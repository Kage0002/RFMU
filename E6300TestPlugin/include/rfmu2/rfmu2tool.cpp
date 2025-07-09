#include "rfmu2tool.h"
#include <QDebug>

Rfmu2Tool::Rfmu2Tool(QObject *parent) : QObject(parent)
{
    mSocket = new QTcpSocket(this);
    connect(mSocket, &QAbstractSocket::stateChanged,
            this, &Rfmu2Tool::onSocketStateChanged);

    /* share one socket instance across all functional modules */
    mSignalGenerator   = new Rfmu2SignalGenerator(mSocket, this);
    mSpectrumAnalyzer  = new Rfmu2SpectrumAnalyzer(mSocket, this);
    mNetworkAnalyzer   = new Rfmu2NetworkAnalyzer(mSocket, this);
    mSystemControl     = new Rfmu2SystemControl(mSocket, this);

    for (auto src : { static_cast<Rfmu2Base*>(mSignalGenerator),
                     static_cast<Rfmu2Base*>(mSpectrumAnalyzer),
                     static_cast<Rfmu2Base*>(mNetworkAnalyzer),
                     static_cast<Rfmu2Base*>(mSystemControl) })
    {
        connect(src, &Rfmu2Base::errorOccurred,
                this, &Rfmu2Tool::errorOccurred);
    }
}

Rfmu2Tool::~Rfmu2Tool()
{
    if (mSocket && mSocket->isOpen())
        mSocket->abort();
}

bool Rfmu2Tool::connectToHost(const QString &addr, int port)
{
    if (addr.isEmpty() || port <= 0) {
        emit errorOccurred({Rfmu2Err::InternalLogic,
                            tr("Invalid address or port")});
        return false;
    }

    mSocket->abort();
    mSocket->connectToHost(addr, port);
    if (!mSocket->waitForConnected(2000)) {
        emit errorOccurred({Rfmu2Err::Timeout,
                            tr("Failed to connect to %1:%2").arg(addr).arg(port)});
        return false;
    }
    return true;
}

void Rfmu2Tool::disconnectFromHost()
{
    if (mSocket->state() == QAbstractSocket::ConnectedState)
        mSocket->disconnectFromHost();
}

void Rfmu2Tool::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    emit connectionStateChanged(state == QAbstractSocket::ConnectedState);
}

bool Rfmu2Tool::isConnected() const noexcept
{
    return mSocket && (mSocket->state() == QAbstractSocket::ConnectedState);
}
