#include "rfmu2spectrumanalyzer.h"
#include <QDebug>

static inline auto lvlParts(double db) {
    return Rfmu2Base::splitDoubleAtDecimal(db);
}

Rfmu2SpectrumAnalyzer::Rfmu2SpectrumAnalyzer(QTcpSocket *s, QObject *p)
    : Rfmu2Base(s, p)
{
    m_timeoutMs = 5'000;   // typical SA responses are small-ish
}

/*------------------------------------------------------------------
 * Internal helper to build a command (opcode selects 0x05 vs 0x21)
 *----------------------------------------------------------------*/
static QByteArray buildSaCmd(quint8 opcode,
                             int freqKHz,
                             const QPair<qint8,qint8>& level,
                             int recvCh, quint8 measMode,
                             int rfPortChannel)
{
    QByteArray cmd;
    cmd.append(Rfmu2Base::int24ToBytes(Rfmu2Base::FrameHeaderValue))
        .append(char(opcode))
        .append(Rfmu2Base::int24ToBytes(freqKHz))
        .append(Rfmu2Base::toByte(level.first))
        .append(Rfmu2Base::toByte(level.second));

    if (opcode == 0x21)                 // extended variant carries receiver channel
        cmd.append(Rfmu2Base::toByte(recvCh));

    cmd.append(char(measMode))          // 0x01 peak, 0x02 raw
        .append(Rfmu2Base::toByte(rfPortChannel))
        .append(Rfmu2Base::int24ToBytes(Rfmu2Base::FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));
    return cmd;
}

/* ---------------- peak data (two overloads) ---------------- */
QVector<double> Rfmu2SpectrumAnalyzer::measurePeakData(int freqKHz, double lvl,
                                                       const QString &rfPath)
{
    auto lv = lvlParts(lvl);
    QByteArray cmd = buildSaCmd(0x05, freqKHz, lv, 0, 0x01, channelForRfPort(rfPath));

    if (!sendCommand(cmd)) return {};
    QByteArray resp = receiveResponse();
    if (resp.isEmpty())   return {};

    QByteArray payload = extractPayloadFromPackage(resp, 1);
    if (payload.isEmpty()) {
        fail(Rfmu2Err::Protocol, QStringLiteral("Peak payload empty"));
        return {};
    }
    return bytesToDoubleVector(payload);
}

QVector<double> Rfmu2SpectrumAnalyzer::measurePeakData(int freqKHz, double lvl,
                                                       int recvCh,
                                                       const QString &rfPath)
{
    auto lv = lvlParts(lvl);
    QByteArray cmd = buildSaCmd(0x21, freqKHz, lv, recvCh, 0x01, channelForRfPort(rfPath));
    if (!sendCommand(cmd)) return {};
    QByteArray resp = receiveResponse();
    if (resp.isEmpty())   return {};
    QByteArray payload = extractPayloadFromPackage(resp, 1);
    if (payload.isEmpty()) {
        fail(Rfmu2Err::Protocol, QStringLiteral("Peak payload empty"));
        return {};
    }
    return bytesToDoubleVector(payload);
}

/* ---------------- raw data (two overloads) ---------------- */
QVector<double> Rfmu2SpectrumAnalyzer::measureRawData(int freqKHz, double lvl,
                                                      const QString &rfPath)
{
    auto lv = lvlParts(lvl);
    QByteArray cmd = buildSaCmd(0x05, freqKHz, lv, 0, 0x02, channelForRfPort(rfPath));
    if (!sendCommand(cmd)) return {};
    QByteArray resp = receiveResponse();
    if (resp.isEmpty())   return {};
    QByteArray payload = extractPayloadFromPackage(resp, 2);
    if (payload.isEmpty()) {
        fail(Rfmu2Err::Protocol, QStringLiteral("Raw payload empty"));
        return {};
    }
    return bytesToDoubleVector(payload);
}

QVector<double> Rfmu2SpectrumAnalyzer::measureRawData(int freqKHz, double lvl,
                                                      int recvCh,
                                                      const QString &rfPath)
{
    auto lv = lvlParts(lvl);
    QByteArray cmd = buildSaCmd(0x21, freqKHz, lv, recvCh, 0x02, channelForRfPort(rfPath));
    if (!sendCommand(cmd)) return {};
    QByteArray resp = receiveResponse();
    if (resp.isEmpty())   return {};
    QByteArray payload = extractPayloadFromPackage(resp, 2);
    if (payload.isEmpty()) {
        fail(Rfmu2Err::Protocol, QStringLiteral("Raw payload empty"));
        return {};
    }
    return bytesToDoubleVector(payload);
}

/* ---------------- IQ data ---------------- */
QVector<Rfmu2Base::IQ> Rfmu2SpectrumAnalyzer::measureIqData(int freqKHz,
                                                            double lvl,
                                                            const QString &rfPath)
{
    auto lv = lvlParts(lvl);
    int ch  = channelForRfPort(rfPath);

    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x06))
        .append(int24ToBytes(freqKHz))
        .append(toByte(lv.first))
        .append(toByte(lv.second))
        .append(toByte(ch))
        .append(int24ToBytes(FrameTailValue));
    cmd.insert(3, char(cmd.size() + 1));

    if (!sendCommand(cmd)) return {};
    QByteArray resp = receiveResponse();
    if (resp.isEmpty())   return {};

    QByteArray payload = extractPayloadFromPackage(resp, 2);
    if (payload.isEmpty()) {
        fail(Rfmu2Err::Protocol, QStringLiteral("IQ payload empty"));
        return {};
    }

    if (payload.size() % 4 != 0) {
        fail(Rfmu2Err::DataFormat, QStringLiteral("IQ bytes not multiple of 4"));
        return {};
    }

    int sampleCount = payload.size() / 4;
    QVector<IQ> out;
    out.reserve(sampleCount);
    const uchar *raw = reinterpret_cast<const uchar*>(payload.constData());
    for (int i = 0; i < sampleCount; ++i) {
        qint16 iVal = (raw[i*4]   << 8) | raw[i*4+1];
        qint16 qVal = (raw[i*4+2] << 8) | raw[i*4+3];
        out.append(IQ(double(iVal), double(qVal)));
    }
    return out;
}
