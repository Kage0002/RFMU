#include "rfmu2systemcontrol.h"
#include <QDebug>
#include <QtEndian>

namespace {
constexpr quint32 kHdr = 0x5A5A5A;          // 3-byte frame header
constexpr quint32 kTlr = 0xA5A5A5;          // 3-byte frame tail
constexpr quint32 rHdr = 0xAA55AA;
constexpr quint32 rTlr = 0x55AA55;
constexpr quint8  kFuncCalibration = 0x01;  // function code
}

Rfmu2SystemControl::Rfmu2SystemControl(QTcpSocket *s, QObject *p)
    : Rfmu2Base(s, p)
{
    m_timeoutMs = 5'000;    // quick housekeeping commands
}

/*-----------------------------------------------------------
 * Reference clock (function 0x19, mode byte)
 *----------------------------------------------------------*/
bool Rfmu2SystemControl::setReferenceClockMode(bool useInternal)
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x19))                       // function
        .append(useInternal ? char(0x03)          // mode
                            : char(0x00))
        .append(int24ToBytes(FrameTailValue));
    cmd.insert(3, char(cmd.size() + 1));

    return sendAndEcho(cmd);
}

/*-----------------------------------------------------------
 * 8 supply voltages + temperature (function 0x20)
 *----------------------------------------------------------*/
QVector<double> Rfmu2SystemControl::readVoltagesAndTemperature()
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x20))                       // function
        .append(int24ToBytes(FrameTailValue));
    cmd.insert(3, char(cmd.size() + 1));

    if (!sendCommand(cmd))
        return {};

    QByteArray resp = receiveResponse();
    if (resp.isEmpty())
        return {};

    QByteArray payload = extractPayloadFromPackage(resp, 2); // 2-byte length field
    if (payload.size() < 72) {
        fail(Rfmu2Err::Protocol,
             QStringLiteral("Voltage/temp payload too short"));
        return {};
    }

    QVector<double> vals = bytesToDoubleVector(payload);
    if (vals.size() < 9) {
        fail(Rfmu2Err::DataFormat, QStringLiteral("Could not parse 9 doubles"));
        return {};
    }
    vals.resize(9); // trim any extra data
    return vals; // [V1..V8, Temp]
}

/*-----------------------------------------------------------
 * RRSU-TX calibration upload  (function 0x01)
 *----------------------------------------------------------*/
bool Rfmu2SystemControl::sendRRSUCalibration(const QByteArray &data, const QString &channelStr)
{
    qDebug() << "[RRSU] === upload start === size" << data.size()
    << "bytes  chan" << channelStr;

    /* -------------- sanity --------------------------------- */
    if (data.isEmpty() || data.size() > 0xFFFF) {
        qDebug() << "[RRSU] blob out of range" << data.size();
        fail(Rfmu2Err::InternalLogic,
             QStringLiteral("calibration blob size out of range"));
        return false;
    }
    const int ch = channelForRfPort(channelStr);
    if (ch < 0 || ch > 0x0F) {
        qDebug() << "[RRSU] bad channel string" << channelStr;
        fail(Rfmu2Err::InternalLogic,
             QStringLiteral("invalid RRSU RF-port string"));
        return false;
    }
    const quint8 channel = quint8(ch);

    /* -------------- constants ------------------------------ */
    constexpr int    kMaxPayload   = 1000;
    constexpr quint8 kFuncCode     = kFuncCalibration;   // 0x01

    /* ========================================================
     * 1. Build *all* packets first (len / remain placeholders)
     * ====================================================== */
    struct Packet { QByteArray bytes; };
    QList<Packet> pkts;

    auto makeFrame = [&](quint8 type, const QByteArray &payload) {
        QByteArray frame;
        frame.append(int24ToBytes(kHdr)); // header (3)
        frame.append("\0\0", 2); // length placeholder (2)
        frame.append(char(kFuncCode))
            .append(char(type))
            .append("\0\0", 2) // remain placeholder (2)
            .append(char(channel))
            .append(payload)
            .append(int24ToBytes(kTlr)); // tail (3)
        const quint16 len = quint16(frame.size());
        qToBigEndian(len, frame.data() + 3); // patch length
        return frame;
    };

    int off = 0;
    while (off < data.size()) {
        const int  chunk = std::min(kMaxPayload, data.size() - off);
        const bool first = (off == 0);
        const bool last  = (off + chunk == data.size());
        const quint8 typ = first ? 0x0A : (last ? 0x0C : 0x0B);
        pkts.append({ makeFrame(typ, data.mid(off, chunk)) });
        off += chunk;
    }

    if (pkts.isEmpty()) {
        qDebug() << "[RRSU] internal error - no packets";
        fail(Rfmu2Err::InternalLogic, QStringLiteral("no packets generated"));
        return false;
    }

    /* ========================================================
     * 2. Patch remain field for every packet
     * ====================================================== */
    qsizetype bytesRemainingAll = 0;
    for (const auto &p : pkts) bytesRemainingAll += p.bytes.size();

    qsizetype bytesProcessed = 0;
    for (auto &p : pkts) {
        const bool isTail = (p.bytes[6] == char(0x0C));   // type at index 6
        quint16 remainVal = isTail ? quint16(p.bytes.size())
                                   : quint16(bytesRemainingAll - bytesProcessed);
        p.bytes[7] = char(remainVal >> 8);
        p.bytes[8] = char(remainVal & 0xFF);
        bytesProcessed += p.bytes.size();
    }

    qDebug() << "[RRSU] prepared" << pkts.size() << "frames, total bytes"
             << bytesRemainingAll;

    /* ========================================================
     * 3. Send packets - ACK for head & tail only
     * ====================================================== */
    for (int i = 0; i < pkts.size(); ++i) {
        const Packet &pkt = pkts[i];
        const quint8  typeByte = quint8(pkt.bytes[6]);
        const quint16 remain   = quint16(quint8(pkt.bytes[7]) << 8 | quint8(pkt.bytes[8]));

        qDebug() << "[RRSU] TX" << i << "type" << QString("0x%1").arg(typeByte,2,16,QChar('0'))
                 << "len" << pkt.bytes.size() << "remain" << remain;
        qDebug().noquote() << "RAW" << pkt.bytes.toHex(' ');

        if (!sendCommand(pkt.bytes)) {
            qDebug() << "[RRSU] sendCommand failed at frame" << i;
            return false;
        }

        if (typeByte == 0x0B) { // middle - no ACK expected
            continue;
        }

        QByteArray ack = receiveResponse();
        qDebug() << "[RRSU] ACK size" << ack.size();
        if (ack.isEmpty()) {
            qDebug() << "[RRSU] no ACK (timeout)";
            return false;
        }

        // framing quick-check
        if (ack.left(3) != int24ToBytes(rHdr) ||
            ack.right(3) != int24ToBytes(rTlr) ||
            ack[4] != char(kFuncCode))
        {
            qDebug() << "[RRSU] malformed ACK frame" << ack.toHex();
            fail(Rfmu2Err::Protocol, QStringLiteral("malformed RRSU ACK"));
            return false;
        }

        if (typeByte == 0x0A) {
            const quint16 echoedLen = qFromBigEndian<quint16>(ack.constData() + 5);
            const quint16 ourTotal  = quint16(bytesRemainingAll);
            qDebug() << "[RRSU] head-ACK length dev:" << echoedLen << " ours:" << ourTotal;
            if (echoedLen != ourTotal) {
                fail(Rfmu2Err::Protocol,
                     QStringLiteral("head ACK length mismatch (dev %1, ours %2)")
                         .arg(echoedLen).arg(ourTotal));
                return false;
            }
        } else { // 0x0C tail
            const char status = ack[ack.size() - 4];
            qDebug() << "[RRSU] tail-ACK status" << QString("0x%1").arg(uchar(status),2,16,QChar('0'));
            if (status != '\x01') {
                fail(Rfmu2Err::DeviceNack,
                     QStringLiteral("device reported CRC/length error"));
                return false;
            }
        }
    }

    qDebug() << "[RRSU] === upload done (OK) ===";
    return true;
}
