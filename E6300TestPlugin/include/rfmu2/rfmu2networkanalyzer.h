#pragma once
#include "rfmu2base.h"
#include <QTcpSocket>
#include <QVector>

struct SinglePortCaliData {
    quint8   fileNumber = 0;
    quint8   portNumber = 0;
    qint8    powerInt   = 0;
    qint8    powerFrac  = 0;
    int      startFreqKHz = 0;
    int      stopFreqKHz  = 0;
    quint16  sweepPoints  = 0;
};

struct DualPortCaliData {
    quint8   fileNumber   = 0;
    quint8   port1Number  = 0;
    quint8   port2Number  = 0;
    qint8    powerInt     = 0;
    qint8    powerFrac    = 0;
    int      startFreqKHz = 0;
    int      stopFreqKHz  = 0;
    quint16  sweepPoints  = 0;
};

class Rfmu2NetworkAnalyzer : public Rfmu2Base
{
    Q_OBJECT
public:
    enum class ResultType : unsigned char {
        Complex      = 0x01,
        LogAmp       = 0x02,
        Phase        = 0x03,
        LogAmpPhase  = 0x04
    };
    Q_ENUM(ResultType)
    explicit Rfmu2NetworkAnalyzer(QTcpSocket *socket, QObject *parent = nullptr);
    ~Rfmu2NetworkAnalyzer() override = default;

    Rfmu2NetworkAnalyzer(const Rfmu2NetworkAnalyzer&)            = delete;
    Rfmu2NetworkAnalyzer& operator=(const Rfmu2NetworkAnalyzer&) = delete;

    /* sweep configuration */
    bool configureFrequencySweep(int startKHz, int stopKHz);
    bool configurePowerSweep(double startDb, double stopDb);
    bool configurePointsAndPorts(int points,
                                 const QString &rfPort1,
                                 const QString &rfPort2);

    /* measurements */
    QVector<double> measureSinglePort(ResultType retType = ResultType::LogAmp);
    QVector<double> measureDualPort  (ResultType retType = ResultType::LogAmp);

    /* single-port calibration */
    bool calibrateSinglePortOpen();
    bool calibrateSinglePortShort();
    bool calibrateSinglePortLoad();
    bool finishSinglePortCalibration();
    bool saveSinglePortCalibrationState(int stateNumber);
    SinglePortCaliData loadSinglePortCalibrationState(int stateNumber,
                                                      bool *ok = nullptr);

    /* dual-port calibration */
    bool calibrateDualPortOpen1();
    bool calibrateDualPortShort1();
    bool calibrateDualPortLoad1();
    bool calibrateDualPortThrough1();
    bool calibrateDualPortOpen2();
    bool calibrateDualPortShort2();
    bool calibrateDualPortLoad2();
    bool calibrateDualPortThrough2();
    bool finishDualPortCalibration();
    bool saveDualPortCalibrationState(int stateNumber);
    DualPortCaliData loadDualPortCalibrationState(int stateNumber,
                                                  bool *ok = nullptr);

private:
    /* parsing helpers */
    SinglePortCaliData parseSinglePortCaliData(const QByteArray &payload,
                                               bool &ok);
    DualPortCaliData   parseDualPortCaliData  (const QByteArray &payload,
                                           bool &ok);

    /* small wrapper that builds, echoes and returns true on success */
    bool sendCal(quint8 mode, quint8 data);
};
