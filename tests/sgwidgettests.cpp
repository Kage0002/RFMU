#include <QtTest>
#include <QSignalSpy>

#define private public
#include "../E6300TestPlugin/sgwidget.h"
#undef private

class SGWidgetTests : public QObject
{
    Q_OBJECT
private slots:
    void sweepWorkerDeleted();
};

void SGWidgetTests::sweepWorkerDeleted()
{
    SGWidget *w = new SGWidget;
    SGStepWorker *worker = w->m_sweepWorker;
    QSignalSpy spy(worker, &QObject::destroyed);

    delete w;
    // process deferred deletion events posted by deleteLater
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();

    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(SGWidgetTests)
#include "sgwidgettests.moc"
