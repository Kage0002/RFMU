#pragma once
/****************************************************************************
**  Rfmu2IoContext – lives *exclusively* in the I/O thread.
**
**  Owns the QTcpSocket and all four low‑level instrument helpers, exposing
**  *slot‑only* façade methods so that the GUI thread can talk to it through
**  queued signal/slot connections.
****************************************************************************/

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <memory>

class Rfmu2NetworkAnalyzer;
class Rfmu2SpectrumAnalyzer;
class Rfmu2SignalGenerator;
class Rfmu2SystemControl;

class Rfmu2IoContext : public QObject
{
    Q_OBJECT
public:
    explicit Rfmu2IoContext(QObject *parent = nullptr);
    ~Rfmu2IoContext() override;

    // Read‑only accessors (no direct mutating calls from outside the thread)
    Rfmu2NetworkAnalyzer *networkAnalyzer()   const { return m_na.get(); }
    Rfmu2SpectrumAnalyzer *spectrumAnalyzer() const { return m_sa.get(); }
    Rfmu2SignalGenerator *signalGenerator()   const { return m_sg.get(); }
    Rfmu2SystemControl  *systemControl()      const { return m_sys.get(); }

signals:
    /* -------- socket‑level state -------- */
    void connected();
    void disconnected();
    void errorOccurred(QAbstractSocket::SocketError code, const QString &text);

    /* -------- high‑level operations --------
       Each long‑running async call will eventually emit its own “finished(…)”
       signal from the relevant instrument helper.  We *re‑emit* them here so
       the GUI only needs to connect to one object (Rfmu2Tool later).          */
    void finished(const QString &tag);

public slots:
    /* ----- façade API: only *slots*, so calls from the GUI are automatically
           queued when IoContext lives on a worker thread.                   */
    void connectToHost(const QHostAddress &address, quint16 port);
    void disconnectFromHost();

    /* Examples of higher‑level requests.  Concrete argument lists will be
       fleshed out in Step 4 when the instrument classes become non‑blocking. */
    void performNetworkSweep(/* sweep parameters TBD */);
    void performSpectrumScan(/* scan parameters TBD */);

private slots:
    /* Internal wiring for the socket */
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError e);

private:
    QTcpSocket m_socket;   ///< lives entirely in this thread
    std::unique_ptr<Rfmu2NetworkAnalyzer>   m_na;
    std::unique_ptr<Rfmu2SpectrumAnalyzer>  m_sa;
    std::unique_ptr<Rfmu2SignalGenerator>   m_sg;
    std::unique_ptr<Rfmu2SystemControl>     m_sys;
};
