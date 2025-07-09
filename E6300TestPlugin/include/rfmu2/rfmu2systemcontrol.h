#pragma once
#include "rfmu2base.h"
#include <QTcpSocket>
#include <QVector>

class Rfmu2SystemControl : public Rfmu2Base
{
    Q_OBJECT
public:
    explicit Rfmu2SystemControl(QTcpSocket *socket, QObject *parent = nullptr);
    ~Rfmu2SystemControl() override = default;

    Rfmu2SystemControl(const Rfmu2SystemControl&)            = delete;
    Rfmu2SystemControl& operator=(const Rfmu2SystemControl&) = delete;

    bool setReferenceClockMode(bool useInternal);        // true = internal
    QVector<double> readVoltagesAndTemperature();        // 8V + 1T
    bool sendRRSUCalibration(const QByteArray &data, const QString &channel);
};
