#include "rfmu2base.h"
#include "rfmu2_error.h"
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QMetaObject>
#include <cmath>
#include <cstring>
#include <QMetaType>
#include <QtEndian>

static const int _rfmu2_error_metatype_id =
    qRegisterMetaType<Rfmu2Error>("Rfmu2Error");

// ---------------- private helpers ----------------
[[nodiscard]] bool Rfmu2Base::fail(Rfmu2Err code, QStringView msg) noexcept
{
    qWarning() << "[Rfmu2Base]" << msg;
    emit errorOccurred(Rfmu2Error{code, msg.toString()});
    return false;
}

// ---------------- ctor ----------------
Rfmu2Base::Rfmu2Base(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket)
{}

// ---------------- send/echo helper ----------------
bool Rfmu2Base::sendAndEcho(const QByteArray &cmd, int timeoutMs)
{
    if (!sendCommand(cmd))
        return false; // fail() already emitted

    QByteArray echo = receiveResponse(timeoutMs);
    if (echo != cmd)
        return fail(Rfmu2Err::Protocol, QStringLiteral("Unexpected echo frame"));

    return true;
}

// ---------------- sendCommand ----------------
bool Rfmu2Base::sendCommand(const QByteArray &cmd)
{
    if (cmd.isEmpty())
        return fail(Rfmu2Err::InternalLogic, QStringLiteral("Command is empty"));

    if (!m_socket)
        return fail(Rfmu2Err::InternalLogic, QStringLiteral("Socket pointer null"));

    if (m_socket->state() != QAbstractSocket::ConnectedState)
        return fail(Rfmu2Err::TcpWriteFail, QStringLiteral("Socket not connected"));

    if (!m_socket->isWritable())
        return fail(Rfmu2Err::TcpWriteFail, QStringLiteral("Socket not writable"));

    const qint64 totalBytes = cmd.size();
    qint64 bytesSent = 0;
    const char *dataPtr = cmd.constData();

    while (bytesSent < totalBytes) {
        qint64 written = m_socket->write(dataPtr + bytesSent, totalBytes - bytesSent);
        if (written < 0)
            return fail(Rfmu2Err::TcpWriteFail, QStringLiteral("write() returned <0"));

        if (written == 0) {
            if (!m_socket->waitForBytesWritten(m_timeoutMs))
                return fail(Rfmu2Err::Timeout, QStringLiteral("waitForBytesWritten timeout"));
        } else {
            bytesSent += written;
            if (!m_socket->waitForBytesWritten(m_timeoutMs))
                return fail(Rfmu2Err::Timeout, QStringLiteral("flush timeout after partial write"));
        }
    }
    qDebug() << "command sent:" << cmd.toHex(' ');
    return true;
}

// ---------------- receive ----------------
QByteArray Rfmu2Base::receiveResponse(int timeoutMs)
{
    if (timeoutMs < 0) timeoutMs = m_timeoutMs;
    return readOneFrame(timeoutMs);
}

// ---------------- readOneFrame ----------------
QByteArray Rfmu2Base::readOneFrame(int timeoutMs)
{
    QByteArray completeFrame;

    // Maybe we already have leftover data from a previous call.
    completeFrame = tryExtractFrameFromBuffer();
    if (!completeFrame.isEmpty()) {
        return completeFrame;
    }

    // If no complete frame yet, use an event loop + timer
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(timeoutMs);

    QMetaObject::Connection readConn = connect(m_socket, &QTcpSocket::readyRead,
                                               &loop, [&]()
                                               {
                                                   // Append all newly available data
                                                   m_incomingBuffer.append(m_socket->readAll());

                                                   // Attempt to parse a complete frame from the updated buffer
                                                   QByteArray candidate = tryExtractFrameFromBuffer();
                                                   if (!candidate.isEmpty()) {
                                                       completeFrame = candidate;
                                                       loop.quit();
                                                   }
                                               });

    QMetaObject::Connection timerConn = connect(&timer, &QTimer::timeout,
                                                &loop, &QEventLoop::quit);

    // Block until we get a frame or the timer expires
    loop.exec();

    disconnect(readConn);
    disconnect(timerConn);

    // If we never parsed a complete frame, signal a timeout error
    if (completeFrame.isEmpty())
        fail(Rfmu2Err::Timeout,
             QStringLiteral("Timed out waiting for complete frame"));

    return completeFrame;
}

// ---------------- tryExtractFrameFromBuffer ----------------
QByteArray Rfmu2Base::tryExtractFrameFromBuffer()
{
    static const QByteArray HEADER  = QByteArray::fromHex("AA55AA");
    static const QByteArray TAIL    = QByteArray::fromHex("55AA55");
    const int HEADER_SIZE = 3;
    const int TAIL_SIZE   = 3;

    // 1) Search for the header 0xAA55AA in m_incomingBuffer
    int headerIndex = m_incomingBuffer.indexOf(HEADER);
    if (headerIndex < 0) {
        // No header found at all -> no frame
        return QByteArray();
    }

    // If the header isn't at position 0, discard everything before it
    // to keep the buffer in sync.
    if (headerIndex > 0) {
        m_incomingBuffer.remove(0, headerIndex);
    }

    // Now the buffer starts with the header. We need at least:
    //   3 bytes (header) + 1 byte (length min) + 3 bytes (tail) = 7 bytes total
    if (m_incomingBuffer.size() < (HEADER_SIZE + 1 + TAIL_SIZE)) {
        // Not enough to even hold the smallest valid frame
        return QByteArray();
    }

    // 2) Check the length field. We try:
    //    - 1-byte length at offset 3
    //    - if that doesn't parse validly, try 2-byte length at offset 3..4

    // The offset to the first length byte
    int lengthFieldOffset = HEADER_SIZE; // = 3

    // We'll define a helper lambda to validate and extract the frame
    auto parseWithLengthField = [&](int lengthFieldSize) -> QByteArray
    {
        if (m_incomingBuffer.size() < HEADER_SIZE + lengthFieldSize + TAIL_SIZE) {
            // Not enough data to read the length field + tail
            return QByteArray();
        }

        // Read the length field
        int frameLength = 0;
        if (lengthFieldSize == 1) {
            frameLength = static_cast<unsigned char>(m_incomingBuffer[lengthFieldOffset]);
        } else if (lengthFieldSize == 2) {
            // Big-endian example:
            //   first byte is high, second byte is low
            unsigned char high = static_cast<unsigned char>(m_incomingBuffer[lengthFieldOffset]);
            unsigned char low  = static_cast<unsigned char>(m_incomingBuffer[lengthFieldOffset + 1]);
            frameLength = (high << 8) | low;
        }

        // If frameLength is smaller than the minimum possible
        // (header + length field + tail), it's invalid
        int minFrame = HEADER_SIZE + lengthFieldSize + TAIL_SIZE;
        if (frameLength < minFrame) {
            return QByteArray();
        }

        // If we don't have enough bytes yet to hold the entire frame,
        // we can't parse it yet
        if (m_incomingBuffer.size() < frameLength) {
            return QByteArray();
        }

        // Now we can slice out the proposed frame
        QByteArray candidateFrame = m_incomingBuffer.left(frameLength);

        // 3) Check the tail at the end of candidateFrame
        int tailStart = frameLength - TAIL_SIZE;
        if (tailStart < 0) {
            return QByteArray();
        }

        QByteArray tailBytes = candidateFrame.mid(tailStart, TAIL_SIZE);
        if (tailBytes != TAIL) {
            // Not a valid tail
            return QByteArray();
        }

        // 4) If the header is correct, length is plausible, tail is correct,
        //    we have a complete frame. Remove it from the buffer & return it.
        m_incomingBuffer.remove(0, frameLength);
        return candidateFrame;
    };

    // Attempt 1-byte length parse
    QByteArray frame = parseWithLengthField(1);
    if (!frame.isEmpty()) {
        return frame; // success
    }

    // If 1-byte parse fails, try 2-byte length parse
    frame = parseWithLengthField(2);
    if (!frame.isEmpty()) {
        return frame; // success
    }

    // If both failed, either incomplete data or invalid frame.
    // We return empty to indicate "no complete frame"
    return QByteArray();
}

QPair<qint8,qint8> Rfmu2Base::splitDoubleAtDecimal(double value)
{
    // 1) Split into integer & fractional parts (fraction keeps the sign)
    double intPart;
    double fracPart = std::modf(value, &intPart);

    // 2) Convert to tenths-of-dB and round
    int fracTenths = static_cast<int>(std::round(fracPart * 10.0));  // -9 ... +9

    int whole = static_cast<int>(intPart);

    // 3) Handle cases like  +3.96 -> whole=3, fracTenths=10  or  -2.96 -> -3, -10
    if (fracTenths == 10) {
        whole += 1;
        fracTenths = 0;
    } else if (fracTenths == -10) {
        whole -= 1;
        fracTenths = 0;
    }

    // 4) Cast to qint8 (fits -128...127)
    return { static_cast<qint8>(whole),
            static_cast<qint8>(fracTenths) };
}

int Rfmu2Base::channelForRfPort(const QString &port)
{
    if (port.isEmpty())
        return 0;

    // Use the part after '-' if it exists; otherwise, use the whole port string.
    QString key = port.contains('-') ? port.split('-').value(1) : port;

    static const QMap<QString, int> portMap{
        {"01A", 0}, {"02A", 1}, {"03A", 2}, {"04A", 3},
        {"01B", 4}, {"02B", 5}, {"03B", 6}, {"04B", 7},
        {"01C", 8}, {"02C", 9}, {"03C", 10}, {"04C", 11},
        {"01D", 12}, {"02D", 13}, {"03D", 14}, {"04D", 15}
    };

    return portMap.value(key, 0);
}

QByteArray Rfmu2Base::int24ToBytes(int value, bool bigEndian)
{
    QByteArray arr(3, '\0');
    if(bigEndian) {
        arr[0] = static_cast<char>((value >> 16) & 0xFF);
        arr[1] = static_cast<char>((value >> 8) & 0xFF);
        arr[2] = static_cast<char>(value & 0xFF);
    } else {
        arr[2] = static_cast<char>((value >> 16) & 0xFF);
        arr[1] = static_cast<char>((value >> 8) & 0xFF);
        arr[0] = static_cast<char>(value & 0xFF);
    }
    return arr;
}

QByteArray Rfmu2Base::int16ToBytes(int value, bool bigEndian)
{
    QByteArray arr(2, '\0');
    if(bigEndian) {
        arr[0] = static_cast<char>((value >> 8) & 0xFF);
        arr[1] = static_cast<char>(value & 0xFF);
    } else {
        arr[1] = static_cast<char>((value >> 8) & 0xFF);
        arr[0] = static_cast<char>(value & 0xFF);
    }
    return arr;
}

// ---------------- bytesToDoubleVector ----------------
QVector<double> Rfmu2Base::bytesToDoubleVector(const QByteArray &bytes)
{
    QVector<double> result;
    const int sz = bytes.size();
    if (sz % static_cast<int>(sizeof(double)) != 0) {
        qWarning() << Q_FUNC_INFO << "Byte array size" << sz << "is not a multiple of 8";
        return result; // cannot emit from static context
    }

    const int count = sz / static_cast<int>(sizeof(double));
    result.resize(count);
    for (int i = 0; i < count; ++i)
        std::memcpy(&result[i], bytes.constData() + i * 8, 8);

    return result;
}

QVector<double> Rfmu2Base::bytesToDoubleVector_BE(const QByteArray& bytes)
{
    QVector<double> result;
    const int sz = bytes.size();
    if (sz % static_cast<int>(sizeof(double)) != 0) {
        qWarning() << Q_FUNC_INFO << "Byte array size" << sz << "is not a multiple of 8";
        return result;
    }

    const int count = sz / static_cast<int>(sizeof(double));
    result.reserve(count);

    for (int i = 0; i < count; ++i) {
        const char* p = bytes.constData() + i * 8;
        quint64 raw = qFromBigEndian<quint64>(p);
        double value;
        std::memcpy(&value, &raw, 8);
        result.append(value);
    }
    return result;
}

QByteArray Rfmu2Base::extractPayloadFromPackage(const QByteArray &package, int lengthFieldSize)
{
    QByteArray empty;
    int pkgSize = package.size();
    int minSize = HEADER_SIZE + lengthFieldSize + TAIL_SIZE;
    if(pkgSize < minSize) {
        qWarning() << Q_FUNC_INFO << "Package too small! Need >=" << minSize << "bytes, got" << pkgSize;
        return empty;
    }
    if((HEADER_SIZE + lengthFieldSize) > pkgSize) {
        qWarning() << Q_FUNC_INFO << "Not enough data to read length field!";
        return empty;
    }
    QByteArray lenBytes = package.mid(HEADER_SIZE, lengthFieldSize);
    int frameLength = 0;
    if(lengthFieldSize == 1) {
        frameLength = static_cast<unsigned char>(lenBytes[0]);
    } else if(lengthFieldSize == 2) {
        frameLength = ((static_cast<unsigned char>(lenBytes[0]) << 8)
                       | static_cast<unsigned char>(lenBytes[1]));
    } else {
        qWarning() << Q_FUNC_INFO << "Unsupported lengthFieldSize:" << lengthFieldSize;
        return empty;
    }
    if(frameLength > pkgSize) {
        qWarning() << Q_FUNC_INFO << "Frame length" << frameLength
                   << "exceeds package size" << pkgSize;
        return empty;
    }
    int payloadStart = HEADER_SIZE + lengthFieldSize;
    int payloadEnd = frameLength - TAIL_SIZE;
    if(payloadEnd < payloadStart) {
        qWarning() << Q_FUNC_INFO << "Invalid payload range: start=" << payloadStart
                   << "end=" << payloadEnd << "frameLength=" << frameLength;
        return empty;
    }
    int payloadSize = payloadEnd - payloadStart;
    if(payloadSize < 0) {
        qWarning() << Q_FUNC_INFO << "Negative payload size!";
        return empty;
    }
    return package.mid(payloadStart, payloadSize);
}
