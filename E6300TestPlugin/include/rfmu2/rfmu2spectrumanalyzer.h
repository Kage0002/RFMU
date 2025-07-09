#pragma once
#include "rfmu2base.h"
#include <QTcpSocket>
#include <QVector>

class Rfmu2SpectrumAnalyzer : public Rfmu2Base
{
    Q_OBJECT
public:
    explicit Rfmu2SpectrumAnalyzer(QTcpSocket *socket, QObject *parent = nullptr);
    ~Rfmu2SpectrumAnalyzer() override = default;

    Rfmu2SpectrumAnalyzer(const Rfmu2SpectrumAnalyzer&)            = delete;
    Rfmu2SpectrumAnalyzer& operator=(const Rfmu2SpectrumAnalyzer&) = delete;

    /* peak-based measurements */
    QVector<double> measurePeakData(int freqKHz, double levelDbm, const QString &rfPath);
    QVector<double> measurePeakData(int freqKHz, double levelDbm,
                                    int receiveChannel, const QString &rfPath);

    /* raw FFT bins */
    QVector<double> measureRawData(int freqKHz, double levelDbm, const QString &rfPath);
    QVector<double> measureRawData(int freqKHz, double levelDbm,
                                   int receiveChannel, const QString &rfPath);

    /* IQ stream (I,Q pairs) */
    QVector<IQ> measureIqData(int freqKHz, double levelDbm, const QString &rfPath);
};
