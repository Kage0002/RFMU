#pragma once
#include "rfmu2signalgenerator.h"
#include "rfmu2spectrumanalyzer.h"
#include "rfmu2networkanalyzer.h"
#include "rfmu2systemcontrol.h"

#include <QObject>
#include <QTcpSocket>

class Rfmu2Tool : public QObject
{
    Q_OBJECT
public:
    explicit Rfmu2Tool(QObject *parent = nullptr);
    ~Rfmu2Tool() override;

    Rfmu2Tool(const Rfmu2Tool&)            = delete;
    Rfmu2Tool& operator=(const Rfmu2Tool&) = delete;

    bool isConnected() const noexcept;
    bool connectToHost(const QString &address, int port);
    void disconnectFromHost();

    /* module accessors */
    Rfmu2SignalGenerator   *signalGenerator()   const noexcept { return mSignalGenerator; }
    Rfmu2SpectrumAnalyzer  *spectrumAnalyzer()  const noexcept { return mSpectrumAnalyzer; }
    Rfmu2NetworkAnalyzer   *networkAnalyzer()   const noexcept { return mNetworkAnalyzer; }
    Rfmu2SystemControl     *systemControl()     const noexcept { return mSystemControl; }

signals:
    void connectionStateChanged(bool connected);
    void errorOccurred(const Rfmu2Error &error);    // bubbled-up from sub-modules

private slots:
    void onSocketStateChanged(QAbstractSocket::SocketState state);

private:
    QTcpSocket            *mSocket          = nullptr;
    Rfmu2SignalGenerator  *mSignalGenerator = nullptr;
    Rfmu2SpectrumAnalyzer *mSpectrumAnalyzer= nullptr;
    Rfmu2NetworkAnalyzer  *mNetworkAnalyzer = nullptr;
    Rfmu2SystemControl    *mSystemControl   = nullptr;
};
