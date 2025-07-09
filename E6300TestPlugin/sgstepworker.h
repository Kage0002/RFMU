#pragma once
#include <QObject>
#include <atomic>
#include <QThread>
#include "include/rfmu2/rfmu2tool.h"

/*! Executes the step-sweep without blocking the GUI.                      */
class SGStepWorker : public QObject
{
    Q_OBJECT
public:
    explicit SGStepWorker(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void stepReady(int freqKHz, double levelDbm, QString rfPath);
    void sweepFinished();

public slots:
    void runSweep(double startFreqGHz, double stopFreqGHz,
                  double startLvl,    double stopLvl,
                  int points, int intervalMs, QString rfPath)
    {
        const bool freqSweep = !qFuzzyCompare(startFreqGHz, stopFreqGHz);
        const double fStepHz = freqSweep ?
                                   (stopFreqGHz - startFreqGHz) * 1e9 / double(points - 1) : 0.0;
        const double lStep = freqSweep ? 0.0 :
                                 (stopLvl - startLvl) / double(points - 1);

        for (int i = 0; i < points; ++i) {
            const double freqHz  = startFreqGHz*1e9 + i*fStepHz;
            const int    freqKHz = static_cast<int>(qRound64(freqHz / 1'000.0));
            const double lvlDbm  = startLvl + i*lStep;

            emit stepReady(freqKHz, lvlDbm, rfPath);      // <- GUI thread does I/O
            QThread::msleep(intervalMs);
        }
        emit sweepFinished();
    }
};
