#include "Rfmu2IoContext.h"

/*  ---- forward declarations for the low‑level helpers  ----
    Real headers will be included once those classes are converted to the new
    non‑blocking API in Steps 3‑4.                                            */
#include "rfmu2networkanalyzer.h"
#include "rfmu2spectrumanalyzer.h"
#include "rfmu2signalgenerator.h"
#include "rfmu2systemcontrol.h"

// ────────────────────────────────────────────────────────────────────────────
Rfmu2IoContext::Rfmu2IoContext(QObject *parent)
    : QObject(parent)
{
    /* 1) Wire socket signals to self so we can forward them to the GUI. */
    connect(&m_socket, &QTcpSocket::connected,
            this,        &Rfmu2IoContext::onSocketConnected);
    connect(&m_socket, &QTcpSocket::disconnected,
            this,        &Rfmu2IoContext::onSocketDisconnected);
    connect(&m_socket, &QTcpSocket::errorOccurred,
            this,        &Rfmu2IoContext::onSocketError);

    /* 2) Instantiate helpers – they all share the same QTcpSocket. */
    m_na  = std::make_unique<Rfmu2NetworkAnalyzer>(&m_socket, this);
    m_sa  = std::make_unique<Rfmu2SpectrumAnalyzer>(&m_socket, this);
    m_sg  = std::make_unique<Rfmu2SignalGenerator>(&m_socket, this);
    m_sys = std::make_unique<Rfmu2SystemControl>(&m_socket, this);

    /* 3) Re‑emit their finished/error signals so the upper layer can listen
          to only one object (this).  We’ll tighten the overloads later.     */
    // Example – real signal names will be adjusted in Step 4
    connect(m_na.get(),  &Rfmu2NetworkAnalyzer::finished,
            this,        &Rfmu2IoContext::finished);
    connect(m_sa.get(),  &Rfmu2SpectrumAnalyzer::finished,
            this,        &Rfmu2IoContext::finished);
    connect(m_sg.get(),  &Rfmu2SignalGenerator::finished,
            this,        &Rfmu2IoContext::finished);
    connect(m_sys.get(), &Rfmu2SystemControl::finished,
            this,        &Rfmu2IoContext::finished);
}

Rfmu2IoContext::~Rfmu2IoContext() = default;

// ───────────────────────── façade slots ─────────────────────────────────────
void Rfmu2IoContext::connectToHost(const QHostAddress &address, quint16 port)
{
    if (m_socket.state() != QAbstractSocket::UnconnectedState)
        m_socket.abort();                     // cancel previous attempt

    m_socket.connectToHost(address, port);
}

void Rfmu2IoContext::disconnectFromHost()
{
    m_socket.disconnectFromHost();
}

// Placeholder high‑level commands – will be expanded in Step 4
void Rfmu2IoContext::performNetworkSweep()
{
    if (!m_na)
        return;
    /* TODO: schedule sweep parameters once non‑blocking NA API exists. */
}

void Rfmu2IoContext::performSpectrumScan()
{
    if (!m_sa)
        return;
    /* TODO: schedule scan parameters once non‑blocking SA API exists.  */
}

// ────────────────────── socket signal handlers ─────────────────────────────
void Rfmu2IoContext::onSocketConnected()
{
    emit connected();
}

void Rfmu2IoContext::onSocketDisconnected()
{
    emit disconnected();
}

void Rfmu2IoContext::onSocketError(QAbstractSocket::SocketError e)
{
    emit errorOccurred(e, m_socket.errorString());
}
