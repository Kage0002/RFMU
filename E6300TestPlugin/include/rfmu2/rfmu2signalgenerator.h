#pragma once
#include "rfmu2base.h"
#include <QTcpSocket>

class Rfmu2SignalGenerator : public Rfmu2Base
{
    Q_OBJECT
public:
    explicit Rfmu2SignalGenerator(QTcpSocket *socket, QObject *parent = nullptr);
    ~Rfmu2SignalGenerator() override = default;

    Rfmu2SignalGenerator(const Rfmu2SignalGenerator&)            = delete;
    Rfmu2SignalGenerator& operator=(const Rfmu2SignalGenerator&) = delete;

    /* channel configuration */
    bool configureSingleChannel(int freqKHz, double levelDbm, const QString &rfPort);
    bool configureTwoChannels(int f1KHz, double l1Dbm,
                              int f2KHz, double l2Dbm,
                              const QString &rfPort);

    /* control */
    bool stopAllOutputs();
    bool stopSingleOutput();
};
