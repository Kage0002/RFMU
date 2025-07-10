#ifndef SGWIDGET_H
#define SGWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QDoubleSpinBox>
#include "include/rfmu2/rfmu2tool.h"
#include "include/frequencyspinbox.h"
#include "stepsweepdialog.h"
#include "sgstepworker.h"

class SGWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SGWidget(QWidget *parent = nullptr);
    ~SGWidget();

    void setTool(Rfmu2Tool *tool) {
        hardwareTool = tool;
    }

private slots:
    void onSingleChannelClicked();
    void onTwoChannelsClicked();
    void onStopAllOutputsClicked();
    void onStopSingleOutputClicked();

    void onStepSweepClicked();
    void onSweepDone();

private:
    Rfmu2Tool *hardwareTool;

    // Single-channel controls
    FrequencySpinBox *spinBox_SingleFreq;  ///< Frequency input for single channel
    QDoubleSpinBox   *spinBox_SingleLevel; ///< Level in dBm for single channel
    QComboBox *mGenSinglePath;

    // Dual-channel controls
    FrequencySpinBox *spinBox_DualFreq1;   ///< Frequency input for channel 1
    QDoubleSpinBox   *spinBox_DualLevel1;  ///< Level in dBm for channel 1
    FrequencySpinBox *spinBox_DualFreq2;   ///< Frequency input for channel 2
    QDoubleSpinBox   *spinBox_DualLevel2;  ///< Level in dBm for channel 2
    QComboBox *mGenTwoPath;

    // Local log area for debugging feedback
    QTextBrowser     *mLogArea;

    /* ---------- step-sweep infrastructure --------------- */
    QPushButton *m_btnStepSweep {nullptr};
    QThread     *m_sweepThread  {nullptr};
    SGStepWorker*m_sweepWorker  {nullptr};
};

#endif // SGWIDGET_H
