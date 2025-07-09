#include "sgwidget.h"
#include "logging.h"

#include <QGroupBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

SGWidget::SGWidget(QWidget *parent)
    : QWidget(parent)
    , hardwareTool(nullptr)
{
    // Top-level layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Optional log area at bottom
    mLogArea = new QTextBrowser(this);
    mLogArea->setFixedHeight(100);
    mLogArea->document()->setMaximumBlockCount(20);
    logger::log(mLogArea, QStringLiteral("Signal generator initialized."));

    // Group box for Signal Generator inputs
    QGroupBox *signalGenGroup = new QGroupBox(tr("Signal Generator"), this);
    QFormLayout *sigLayout = new QFormLayout(signalGenGroup);

    //----------------------------------------
    // Single-channel controls
    //----------------------------------------
    spinBox_SingleFreq = new FrequencySpinBox;
    spinBox_SingleFreq->setRange(1e8, 6e9);
    spinBox_SingleFreq->setFrequency(1e9);  // Default: 1 GHz

    spinBox_SingleLevel = new QDoubleSpinBox;
    spinBox_SingleLevel->setRange(-100, 10);
    spinBox_SingleLevel->setDecimals(1);
    spinBox_SingleLevel->setSuffix(" dBm");
    spinBox_SingleLevel->setValue(-5);

    mGenSinglePath = new QLineEdit("01A"); // Default path
    auto *mGenSingleBtn  = new QPushButton(tr("Set Channel TX1"));

    sigLayout->addRow(tr("TX1 Freq:"), spinBox_SingleFreq);
    sigLayout->addRow(tr("TX1 Level (dB):"), spinBox_SingleLevel);
    sigLayout->addRow(tr("TX1 RfPath:"), mGenSinglePath);
    sigLayout->addRow(mGenSingleBtn);

    // Connect button to slot
    connect(mGenSingleBtn, &QPushButton::clicked,
            this, &SGWidget::onSingleChannelClicked);

    //----------------------------------------
    // Dual-channel controls
    //----------------------------------------
    spinBox_DualFreq1 = new FrequencySpinBox;
    spinBox_DualFreq1->setRange(1e8, 6e9);
    spinBox_DualFreq1->setFrequency(1e9); // Default: 1 GHz

    spinBox_DualLevel1 = new QDoubleSpinBox;
    spinBox_DualLevel1->setRange(-100, 10);
    spinBox_DualLevel1->setDecimals(1);
    spinBox_DualLevel1->setSuffix(" dBm");
    spinBox_DualLevel1->setValue(-5);

    spinBox_DualFreq2 = new FrequencySpinBox;
    spinBox_DualFreq2->setRange(1e8, 6e9);
    spinBox_DualFreq2->setFrequency(2e9); // Default: 2 GHz

    spinBox_DualLevel2 = new QDoubleSpinBox;
    spinBox_DualLevel2->setRange(-100, 10);
    spinBox_DualLevel2->setDecimals(1);
    spinBox_DualLevel2->setSuffix(" dBm");
    spinBox_DualLevel2->setValue(-7);

    mGenTwoPath = new QLineEdit("02B");
    auto *mGenTwoBtn  = new QPushButton(tr("Set Dual Channels"));

    sigLayout->addRow(tr("TX1 Freq:"),      spinBox_DualFreq1);
    sigLayout->addRow(tr("TX1 Level (dB):"),spinBox_DualLevel1);
    sigLayout->addRow(tr("TX2 Freq:"),      spinBox_DualFreq2);
    sigLayout->addRow(tr("TX2 Level (dB):"),spinBox_DualLevel2);
    sigLayout->addRow(tr("RfPath:"), mGenTwoPath);
    sigLayout->addRow(mGenTwoBtn);

    connect(mGenTwoBtn, &QPushButton::clicked,
            this, &SGWidget::onTwoChannelsClicked);

    //----------------------------------------
    // Stop outputs
    //----------------------------------------
    auto *mStopAllBtn = new QPushButton(tr("Stop All Outputs"));
    auto *mStopSingleBtn = new QPushButton(tr("Stop Single Output"));

    sigLayout->addRow(mStopAllBtn);
    sigLayout->addRow(mStopSingleBtn);

    connect(mStopAllBtn,    &QPushButton::clicked,
            this, &SGWidget::onStopAllOutputsClicked);
    connect(mStopSingleBtn, &QPushButton::clicked,
            this, &SGWidget::onStopSingleOutputClicked);

    m_btnStepSweep = new QPushButton(tr("Step Sweep"));
    sigLayout->addRow(m_btnStepSweep);
    connect(m_btnStepSweep, &QPushButton::clicked,
            this, &SGWidget::onStepSweepClicked);

    /* ---------- dedicated sweep thread ----------------- */
    m_sweepThread  = new QThread(this);
    m_sweepWorker = new SGStepWorker;

    connect(m_sweepWorker, &SGStepWorker::stepReady,
            this, [this](int freqKHz, double lvlDbm, const QString &path)
            {
                if (!hardwareTool || !hardwareTool->signalGenerator())
                    return;

                hardwareTool->signalGenerator()
                    ->configureSingleChannel(freqKHz, lvlDbm, path);
            },
            Qt::QueuedConnection);   // ensure it runs in the GUI thread

    m_sweepWorker->moveToThread(m_sweepThread);

    connect(m_sweepWorker, &SGStepWorker::sweepFinished,
            this,          &SGWidget::onSweepDone,
            Qt::QueuedConnection);

    m_sweepThread->start();

    signalGenGroup->setLayout(sigLayout);
    mainLayout->addWidget(signalGenGroup);

    // Add the log area below the group
    mainLayout->addWidget(mLogArea);
    setLayout(mainLayout);
}

SGWidget::~SGWidget()
{
    if (m_sweepThread) {
        m_sweepThread->quit();
        m_sweepThread->wait();
    }
}

//----------------------------------------
// Single Channel
//----------------------------------------
void SGWidget::onSingleChannelClicked()
{
    if (!hardwareTool || !hardwareTool->signalGenerator()) {
        logger::log(mLogArea, QStringLiteral("SignalGenerator is null!"));
        return;
    }

    // Convert frequency from Hz to kHz
    int freqKHz  = static_cast<int>(spinBox_SingleFreq->frequency() / 1000.0);
    double level = spinBox_SingleLevel->value();
    QString path = mGenSinglePath->text();

    logger::log(mLogArea, QString("[SignalGen] Configure single freq=%1 kHz, level=%2 dBm, path=%3").arg(freqKHz).arg(level).arg(path));

    bool ok = hardwareTool->signalGenerator()
                  ->configureSingleChannel(freqKHz, level, path);
    if (!ok) {
        logger::log(mLogArea, QStringLiteral("Failed to configure single channel."));
    } else {
        logger::log(mLogArea, QStringLiteral("Single channel configured successfully."));
    }
}

//----------------------------------------
// Two Channels
//----------------------------------------
void SGWidget::onTwoChannelsClicked()
{
    if (!hardwareTool || !hardwareTool->signalGenerator()) {
        logger::log(mLogArea, QStringLiteral("SignalGenerator is null!"));
        return;
    }

    int f1KHz = static_cast<int>(spinBox_DualFreq1->frequency() / 1000.0);
    double l1 = spinBox_DualLevel1->value();
    int f2KHz = static_cast<int>(spinBox_DualFreq2->frequency() / 1000.0);
    double l2 = spinBox_DualLevel2->value();
    QString path = mGenTwoPath->text();

    logger::log(mLogArea,
        QString("[SignalGen] Two-ch freq1=%1 kHz, lev1=%2 dBm, freq2=%3 kHz, lev2=%4 dBm, path=%5")
            .arg(f1KHz).arg(l1).arg(f2KHz).arg(l2).arg(path)
        );

    bool ok = hardwareTool->signalGenerator()
                  ->configureTwoChannels(f1KHz, l1, f2KHz, l2, path);
    if (!ok) {
        logger::log(mLogArea, QStringLiteral("Failed to configure two channels."));
    } else {
        logger::log(mLogArea, QStringLiteral("Two channels configured successfully."));
    }
}

//----------------------------------------
// Stop All Outputs
//----------------------------------------
void SGWidget::onStopAllOutputsClicked()
{
    if (!hardwareTool || !hardwareTool->signalGenerator()) {
        logger::log(mLogArea, QStringLiteral("SignalGenerator is null!"));
        return;
    }

    logger::log(mLogArea, QStringLiteral("[SignalGen] Stopping ALL outputs."));

    bool ok = hardwareTool->signalGenerator()->stopAllOutputs();
    if (!ok) {
        logger::log(mLogArea, QStringLiteral("Failed to stop all outputs."));
    } else {
        logger::log(mLogArea, QStringLiteral("All outputs stopped successfully."));
    }
}

//----------------------------------------
// Stop Single Output
//----------------------------------------
void SGWidget::onStopSingleOutputClicked()
{
    if (!hardwareTool || !hardwareTool->signalGenerator()) {
        logger::log(mLogArea, QStringLiteral("SignalGenerator is null!"));
        return;
    }

    logger::log(mLogArea, QStringLiteral("[SignalGen] Stopping SINGLE output."));

    bool ok = hardwareTool->signalGenerator()->stopSingleOutput();
    if (!ok) {
        logger::log(mLogArea, QStringLiteral("Failed to stop single output."));
    } else {
        logger::log(mLogArea, QStringLiteral("Single output stopped successfully."));
    }
}

void SGWidget::onStepSweepClicked()
{
    StepSweepDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    /* hand the job to the worker -- one simple queued call ----------- */
    QMetaObject::invokeMethod(m_sweepWorker, "runSweep",
                              Qt::QueuedConnection,
                              Q_ARG(double, dlg.startFreqGHz()),
                              Q_ARG(double, dlg.stopFreqGHz()),
                              Q_ARG(double, dlg.startLvlDbm()),
                              Q_ARG(double, dlg.stopLvlDbm()),
                              Q_ARG(int,    dlg.points()),
                              Q_ARG(int,    dlg.intervalMs()),
                              Q_ARG(QString,dlg.rfPath()));
}

void SGWidget::onSweepDone()
{
    logger::log(mLogArea, tr("Step-sweep finished."));
}

