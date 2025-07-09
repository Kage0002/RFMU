#include "rfmu2signalgenerator.h"
#include <QDebug>

static const QByteArray FRAME_TAIL = QByteArray::fromHex("55AA55");

Rfmu2SignalGenerator::Rfmu2SignalGenerator(QTcpSocket *sock, QObject *parent)
    : Rfmu2Base(sock, parent)
{
    m_timeoutMs = 5'000;   // default for quick SG commands
}

// helper: build level as int,int (dB, 0.1 dB)
static inline auto levelParts(double db)
{
    return Rfmu2Base::splitDoubleAtDecimal(db);
}

bool Rfmu2SignalGenerator::configureSingleChannel(int freqKHz,
                                                  double levelDbm,
                                                  const QString &rfPort)
{
    if (freqKHz <= 0)
        return fail(Rfmu2Err::InternalLogic,
                    QStringLiteral("Frequency must be >0"));

    auto parts = levelParts(levelDbm);
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x01))                       // function: single-tone
        .append(int24ToBytes(freqKHz))
        .append(toByte(parts.first))
        .append(toByte(parts.second))
        .append(toByte(channelForRfPort(rfPort)))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}

bool Rfmu2SignalGenerator::configureTwoChannels(int f1KHz, double l1Dbm,
                                                int f2KHz, double l2Dbm,
                                                const QString &rfPort)
{
    if (f1KHz <= 0 || f2KHz <= 0)
        return fail(Rfmu2Err::InternalLogic,
                    QStringLiteral("Frequencies must be >0"));

    auto p1 = levelParts(l1Dbm);
    auto p2 = levelParts(l2Dbm);

    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x03))                       // function: two-tone
        .append(int24ToBytes(f1KHz))
        .append(toByte(p1.first))
        .append(toByte(p1.second))
        .append(int24ToBytes(f2KHz))
        .append(toByte(p2.first))
        .append(toByte(p2.second))
        .append(toByte(channelForRfPort(rfPort)))
        .append(int24ToBytes(FrameTailValue));

    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}

bool Rfmu2SignalGenerator::stopAllOutputs()
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x08))
        .append(int24ToBytes(FrameTailValue));
    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}

bool Rfmu2SignalGenerator::stopSingleOutput()
{
    QByteArray cmd;
    cmd.append(int24ToBytes(FrameHeaderValue))
        .append(char(0x09))
        .append(int24ToBytes(FrameTailValue));
    cmd.insert(3, char(cmd.size() + 1));
    return sendAndEcho(cmd);
}
