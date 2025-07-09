#pragma once

#include <QObject>
#include <QByteArray>
#include <QVector>
#include <QPair>
#include <QString>
#include <QTcpSocket>
#include <QMap>
#include <QStringView>
#include <QElapsedTimer>
#include "rfmu2_error.h"

class Rfmu2Base : public QObject
{
    Q_OBJECT
public:
    struct IQ {
        double I = 0.0;
        double Q = 0.0;
        IQ(double iVal = 0.0, double qVal = 0.0) : I(iVal), Q(qVal) {}
    };

    static constexpr int HEADER_SIZE       = 3;
    static constexpr int TAIL_SIZE         = 3;
    static constexpr quint32 FrameHeaderValue = 0xAA55AAu;
    static constexpr quint32 FrameTailValue   = 0x55AA55u;

    explicit Rfmu2Base(QTcpSocket* socket, QObject* parent = nullptr);
    ~Rfmu2Base() override = default;

    // high-level helpers
    [[nodiscard]] bool sendAndEcho(const QByteArray& cmd, int timeoutMs = -1);

    void setTimeoutMs(int ms) { m_timeoutMs = ms; }

    // Static helper functions
    static QPair<qint8,qint8> splitDoubleAtDecimal(double value);
    static int channelForRfPort(const QString &port);
    static QByteArray int24ToBytes(int value, bool bigEndian = true);
    static QByteArray int16ToBytes(int value, bool bigEndian = true);
    static QVector<double> bytesToDoubleVector(const QByteArray &bytes);
    QVector<double> bytesToDoubleVector_BE(const QByteArray& bytes);
    static QByteArray extractPayloadFromPackage(const QByteArray &package, int lengthFieldSize);

    static inline char toByte(qint8 v) noexcept
    {
        return static_cast<char>(v);
    }

signals:
    void errorOccurred(const Rfmu2Error &error);

protected:
    // unified failure helper
    [[nodiscard]] bool fail(Rfmu2Err code, QStringView msg) noexcept;

    bool sendCommand(const QByteArray &frame);            // internal send
    QByteArray receiveResponse(int timeoutMs = -1);

    QByteArray readOneFrame(int timeoutMs);
    QTcpSocket* m_socket = nullptr;
    int m_timeoutMs      = 5000;

    QByteArray m_incomingBuffer; // persistent buffer for partial data
    QByteArray tryExtractFrameFromBuffer();
};
