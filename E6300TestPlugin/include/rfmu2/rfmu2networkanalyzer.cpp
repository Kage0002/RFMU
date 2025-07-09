#include "rfmu2networkanalyzer.h"
#include <QDebug>

/*--------------------------------------------------------------------
 * ctor
 *------------------------------------------------------------------*/
Rfmu2NetworkAnalyzer::Rfmu2NetworkAnalyzer(QTcpSocket *sock, QObject *parent)
    : Rfmu2Base(sock, parent)
{
    m_timeoutMs = 20'000;          // network sweeps can be lengthy
}

/*--------------------------------------------------------------------
 *  sweep configuration helpers
 *------------------------------------------------------------------*/
bool Rfmu2NetworkAnalyzer::configureFrequencySweep(int startKHz, int stopKHz)
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07))         // function
        .append(char(0x03))         // mode
        .append(char(0x01))         // sub
        .append(int24ToBytes(startKHz))
        .append(int24ToBytes(stopKHz))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));               // length byte
    return sendAndEcho(cmd);
}

bool Rfmu2NetworkAnalyzer::configurePowerSweep(double startDb, double stopDb)
{
    auto s = splitDoubleAtDecimal(startDb);
    auto e = splitDoubleAtDecimal(stopDb);

    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07))
        .append(char(0x03))
        .append(char(0x02))
        .append(toByte(s.first)).append(toByte(s.second))
        .append(toByte(e.first)).append(toByte(e.second))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}

bool Rfmu2NetworkAnalyzer::configurePointsAndPorts(int points,
                                                   const QString &p1,
                                                   const QString &p2)
{
    if (points <= 0 || points > 401)
        return fail(Rfmu2Err::InternalLogic,
                    QStringLiteral("Invalid sweep-point count"));

    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07))
        .append(char(0x03))
        .append(char(0x03))
        .append(int16ToBytes(points))
        .append(toByte(channelForRfPort(p1)))
        .append(toByte(channelForRfPort(p2)))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}

/*--------------------------------------------------------------------
 *  measurement helpers
 *------------------------------------------------------------------*/
QVector<double> Rfmu2NetworkAnalyzer::measureSinglePort(ResultType type)
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07)).append(char(0x02)).append(char(0x02))
        .append(char(0x00))
        .append(static_cast<char>(type))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));

    if (!sendCommand(cmd))
        return {};

    QByteArray resp = receiveResponse();
    if (resp.isEmpty())
        return {};                             // fail() already fired inside

    QByteArray payload = extractPayloadFromPackage(resp, 2);
    if (payload.isEmpty())
        return (fail(Rfmu2Err::Protocol, QStringLiteral("empty payload")), QVector<double>{});

    return bytesToDoubleVector(payload);
}

QVector<double> Rfmu2NetworkAnalyzer::measureDualPort(ResultType type)
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07)).append(char(0x01)).append(char(0x02))
        .append(char(0x00))
        .append(static_cast<char>(type))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));

    if (!sendCommand(cmd))
        return {};

    QByteArray resp = receiveResponse();
    if (resp.isEmpty())
        return {};

    QByteArray payload = extractPayloadFromPackage(resp, 2);
    qDebug() << " ---Hex--- \n" << payload.toHex();
    if (payload.isEmpty())
        return (fail(Rfmu2Err::Protocol, QStringLiteral("empty payload")), QVector<double>{});

    return bytesToDoubleVector(payload);
}

/*--------------------------------------------------------------------
 *  common calibration helper  (mode: 0x02 = single-port, 0x01 = dual-port)
 *------------------------------------------------------------------*/
bool Rfmu2NetworkAnalyzer::sendCal(quint8 mode, quint8 data)
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07))          // function
        .append(char(mode))          // mode byte
        .append(char(0x01))          // sub  (fixed for all cal steps)
        .append(char(data))          // data (open/short/...)
        .append(char(0x00))          // reserved
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}

/*------------- wrapper functions -----------------*/
// single-port (mode = 0x02)
bool Rfmu2NetworkAnalyzer::calibrateSinglePortOpen() { return sendCal(0x02, 0x01); }
bool Rfmu2NetworkAnalyzer::calibrateSinglePortShort() { return sendCal(0x02, 0x02); }
bool Rfmu2NetworkAnalyzer::calibrateSinglePortLoad() { return sendCal(0x02, 0x03); }
bool Rfmu2NetworkAnalyzer::finishSinglePortCalibration() { return sendCal(0x02, 0x04); }

bool Rfmu2NetworkAnalyzer::saveSinglePortCalibrationState(int n)
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07)).append(char(0x02)).append(char(0x03))
        .append(toByte(n)).append(char(0x00))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}

SinglePortCaliData Rfmu2NetworkAnalyzer::loadSinglePortCalibrationState(int n, bool* ok)
{
    if (ok) *ok = false;
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07)).append(char(0x02)).append(char(0x04))
        .append(toByte(n)).append(char(0x00))
        .append(int24ToBytes(FrameTailValue));
    cmd.insert(3, char(cmd.size() + 1));

    SinglePortCaliData res {};
    if (!sendCommand(cmd))
        return res;

    QByteArray resp = receiveResponse();
    if (resp.isEmpty()) return res;

    QByteArray pl = extractPayloadFromPackage(resp, 1);
    bool okParse  = false;
    res = parseSinglePortCaliData(pl, okParse);
    if (ok) *ok = okParse;
    return res;
}

// dual-port (mode = 0x01)
bool Rfmu2NetworkAnalyzer::calibrateDualPortOpen1() { return sendCal(0x01, 0x01); }
bool Rfmu2NetworkAnalyzer::calibrateDualPortShort1() { return sendCal(0x01, 0x02); }
bool Rfmu2NetworkAnalyzer::calibrateDualPortLoad1() { return sendCal(0x01, 0x03); }
bool Rfmu2NetworkAnalyzer::calibrateDualPortThrough1() { return sendCal(0x01, 0x04); }
bool Rfmu2NetworkAnalyzer::calibrateDualPortOpen2() { return sendCal(0x01, 0x05); }
bool Rfmu2NetworkAnalyzer::calibrateDualPortShort2() { return sendCal(0x01, 0x06); }
bool Rfmu2NetworkAnalyzer::calibrateDualPortLoad2() { return sendCal(0x01, 0x07); }
bool Rfmu2NetworkAnalyzer::calibrateDualPortThrough2() { return sendCal(0x01, 0x08); }
bool Rfmu2NetworkAnalyzer::finishDualPortCalibration() { return sendCal(0x01, 0x09); }

bool Rfmu2NetworkAnalyzer::saveDualPortCalibrationState(int n)
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07)).append(char(0x01)).append(char(0x03))
        .append(toByte(n)).append(char(0x00))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}

DualPortCaliData Rfmu2NetworkAnalyzer::loadDualPortCalibrationState(int n, bool* ok)
{
    if (ok) *ok = false;
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x07)).append(char(0x01)).append(char(0x04))
        .append(toByte(n)).append(char(0x00))
        .append(int24ToBytes(FrameTailValue));
    cmd.insert(3, char(cmd.size() + 1));

    DualPortCaliData res {};
    if (!sendCommand(cmd))
        return res;

    QByteArray resp = receiveResponse();
    if (resp.isEmpty()) return res;

    QByteArray pl = extractPayloadFromPackage(resp, 1);
    bool okParse  = false;
    res = parseDualPortCaliData(pl, okParse);
    if (ok) *ok = okParse;
    return res;
}

SinglePortCaliData Rfmu2NetworkAnalyzer::parseSinglePortCaliData(const QByteArray &payload, bool &ok)
{
    // 12 bytes total
    // fileNumber(1), portNumber(1),
    // powerInt(1), powerFrac(1),
    // startFreq(3 bytes, big-endian),
    // stopFreq(3 bytes, big-endian),
    // sweepPoints(2 bytes, big-endian).

    ok = false;
    SinglePortCaliData data{};

    // Check payload size
    if (payload.size() < 12) {
        qWarning() << Q_FUNC_INFO
                   << "Payload too short for single-port cali data. Size="
                   << payload.size();
        return data;
    }

    data.fileNumber = static_cast<quint8>( payload[0] );
    data.portNumber = static_cast<quint8>( payload[1] );
    data.powerInt   = static_cast<qint8>(  payload[2] );
    data.powerFrac = static_cast<qint8>(payload[3]);

    // Next 3 bytes => big-endian startFreq
    int startFreq = ((static_cast<unsigned char>(payload[4]) << 16)
                     | (static_cast<unsigned char>(payload[5]) << 8)
                     |  static_cast<unsigned char>(payload[6]));
    data.startFreqKHz = startFreq;

    // Next 3 bytes => big-endian stopFreq
    int stopFreq = ((static_cast<unsigned char>(payload[7]) << 16)
                    | (static_cast<unsigned char>(payload[8]) << 8)
                    |  static_cast<unsigned char>(payload[9]));
    data.stopFreqKHz = stopFreq;

    // Last 2 bytes => big-endian sweepPoints
    quint16 pts = ((static_cast<unsigned char>(payload[10]) << 8)
                   |  static_cast<unsigned char>(payload[11]));
    data.sweepPoints = pts;

    ok = true;
    return data;
}

DualPortCaliData Rfmu2NetworkAnalyzer::parseDualPortCaliData(const QByteArray &payload, bool &ok)
{
    // Format: file(1), port1(1), port2(1), powerInt(1), powerFrac(1), startFreq(3), stopFreq(3), points(2)
    // total 1+1+1+1+1+3+3+2 = 13 bytes
    ok = false;
    DualPortCaliData data{};

    if(payload.size() < 13) {
        qWarning() << Q_FUNC_INFO << "Payload too short for dual-port cali data. Size=" << payload.size();
        return data;
    }

    data.fileNumber = static_cast<quint8>(payload[0]);
    data.port1Number = static_cast<quint8>(payload[1]);
    data.port2Number = static_cast<quint8>(payload[2]);
    data.powerInt = static_cast<qint8>(payload[3]);
    data.powerFrac = static_cast<qint8>(payload[4]);

    int startFreq = ((static_cast<uchar>(payload[5]) << 16)
                     | (static_cast<uchar>(payload[6]) << 8)
                     |  static_cast<uchar>(payload[7]));
    data.startFreqKHz = startFreq;

    int stopFreq = ((static_cast<uchar>(payload[8]) << 16)
                    | (static_cast<uchar>(payload[9]) << 8)
                    |  static_cast<uchar>(payload[10]));
    data.stopFreqKHz = stopFreq;

    quint16 points = ((static_cast<uchar>(payload[11]) << 8)
                      |  static_cast<uchar>(payload[12]));
    data.sweepPoints = points;

    ok = true;
    return data;
}
