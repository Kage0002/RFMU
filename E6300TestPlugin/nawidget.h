#ifndef NAWIDGET_H
#define NAWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QKeyEvent>
#include <QMap>
#include <QVector>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QThread>
#include "include/qcustomplot.h"
#include "include/frequencyspinbox.h"
#include "marker.h"
#include "colorpickerwidget.h"
#include "collapsiblegroupbox.h"
#include "include/rfmu2/rfmu2tool.h"

// Small worker that runs in its own QThread and performs the
// blocking network-analyser call so the GUI thread stays responsive.
class NAWorker : public QObject
{
    Q_OBJECT
public:
    explicit NAWorker(Rfmu2Tool *tool = nullptr, QObject *parent = nullptr)
        : QObject(parent), m_tool(tool) {}

public slots:
    void setTool(Rfmu2Tool *tool) { m_tool = tool; }

    void measureSinglePortAsync(Rfmu2NetworkAnalyzer::ResultType type)
    {
        if (!m_tool || !m_tool->networkAnalyzer()) return;
        QVector<double> raw = m_tool->networkAnalyzer()->measureSinglePort(type);
        emit singlePortReady(type, raw);
    }

    void measureDualPortAsync(Rfmu2NetworkAnalyzer::ResultType type)
    {
        if (!m_tool || !m_tool->networkAnalyzer()) return;
        QVector<double> raw = m_tool->networkAnalyzer()->measureDualPort(type);
        emit dualPortReady(type, raw); // queued back to GUI thread
    }

signals:
    void singlePortReady(Rfmu2NetworkAnalyzer::ResultType, const QVector<double> &);
    void dualPortReady  (Rfmu2NetworkAnalyzer::ResultType, const QVector<double> &);

private:
    Rfmu2Tool *m_tool {nullptr};
};

class NAWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NAWidget(QWidget *parent = nullptr);
    ~NAWidget();

    enum Mode { SingleMode, AutoMode };
    enum TraceType { Off, ClearWrite, MaxHold, MinHold, MinMaxHold, Average };

    void setTool(Rfmu2Tool *tool);

public slots:
    void stopAutoSweep() { setMode(SingleMode); }

private:
    struct TraceData {
        QVector<double> freqs;       // Current frequencies for this trace
        QVector<double> amps;        // Current amplitudes for this trace
        QVector<double> minAmps;     // For Min/Max Hold

        bool updateEnabled;          // If this trace updates on each sweep
        bool hide;                   // If this trace is hidden
        TraceType type;              // Current trace type (Off, ClearWrite, etc.)
        QColor color;                // Current trace color
        int avgCount;                // For Average, how many sweeps to average

        // For Average mode:
        QVector<double> sumAmps;          // Holds the sum of the last N sweeps
        QList<QVector<double>> lastSweeps; // Stores the last N sweeps for rolling average
    };

    struct PeakInfo {
        int index;
        double amplitude;
    };

    static inline const QStringList portLabels = {
        "01A", "02A", "03A", "04A",
        "01B", "02B", "03B", "04B",
        "01C", "02C", "03C", "04C",
        "01D", "02D", "03D", "04D"
    };

private:
    // Store the frequency axis for the last acquisition
    QVector<double> m_freqs;

    // Single-port or dual-port amplitude/phase (or real/imag) data:
    QVector<double> m_s11_ampI;
    QVector<double> m_s21_ampI;
    QVector<double> m_s12_ampI;
    QVector<double> m_s22_ampI;

    QVector<double> m_s11_phaseQ;
    QVector<double> m_s21_phaseQ;
    QVector<double> m_s12_phaseQ;
    QVector<double> m_s22_phaseQ;

signals:
    void naSinglePortCali(const QString &msg);
    void naDualPortCali(const QString &msg);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updatePlot();
    void setMode(NAWidget::Mode mode);
    void placeMarkerAtClick(QMouseEvent *event);
    void peakSearch();
    void onMarkerActiveStateChanged(int state);
    void onCurrentMarkerChanged(const QString &markerName);
    void onDeltaButtonClicked();
    void onValueOfSetFreqChanged(double frequency);
    void onPkTrackingStateChanged(int state);
    void onExportTraceData();
    void onTraceUpdateStateChanged(int state);
    void onTraceHideStateChanged(int state);
    void onPlotColorChanged(const QColor &color);
    void onCurrentTraceChanged(int index);
    void onMarkerPlaceOnChanged(int newTraceIndex);
    void onTraceTypeChanged(int index);
    void onTraceClearClicked();
    void onAvgCountChanged(int avgCount);
    void onCopyToIndexChanged(int index);
    void onMinPeakClicked();
    void onMarkerToCenterClicked();
    void onMarkerToRefLevelClicked();
    void onPeakLeftClicked();
    void onPeakRightClicked();
    void onNextPeakClicked();
    void onRefLevelChanged(double newRefLevel);
    void onDivChanged(double newDiv);
    void onFreqSweepClicked();
    void onLevelSweepClicked();
    void onPointsPortsClicked();

    // Single-Port Cali
    void onSingleCaliOpenClicked();
    void onSingleCaliShortClicked();
    void onSingleCaliLoadStepClicked();
    void onSingleCaliFinishClicked();
    void onSingleCaliSaveFileClicked();
    void onSingleCaliLoadFileClicked();

    // Dual-Port Cali
    void onDualCaliOpen1Clicked();
    void onDualCaliShort1Clicked();
    void onDualCaliLoad1StepClicked();
    void onDualCaliThrough1Clicked();

    void onDualCaliOpen2Clicked();
    void onDualCaliShort2Clicked();
    void onDualCaliLoad2StepClicked();
    void onDualCaliThrough2Clicked();
    void onDualCaliFinishClicked();

    void onDualCaliSaveFileClicked();
    void onDualCaliLoadFileClicked();

    void onMeasTypeChanged();

private:
    void updateMarker(Marker *marker);
    void updateMarkerLabel();

    void acquireSweepData(Rfmu2NetworkAnalyzer::ResultType type, const QVector<double> &rawData);

    // Helper functions for each type
    void applyClearWrite(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);
    void applyMaxHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);
    void applyMinHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);
    void applyMinMaxHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);
    void applyAverage(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);

    void copyTraceData(int srcIndex, int destIndex);
    void revertCopyToComboBox();

    void computeMeanAndStdev(const QVector<double> &amps, double &mean, double &stdev);
    QVector<NAWidget::PeakInfo> findAllPeaksWithPlateaus(const QVector<double> &amps);
    bool checkExcursion(const QVector<double> &amps, int peakIndex, double pkExcurs);

    QCustomPlot *customPlot;
    QTimer *dataTimer;

    double pkThreshold; // For user-defined min amplitude
    double pkExcurs;    // For required amplitude drop to identify a distinct peak

    double startFrequency;
    double stopFrequency;
    double startLevel;
    double stopLevel;
    double startPoint;
    double endPoint;

    Mode currentMode;

    FrequencySpinBox *spinBox_Frequency_Center;
    FrequencySpinBox *spinBox_Frequency_Start;
    FrequencySpinBox *spinBox_Frequency_Stop;
    QDoubleSpinBox *spinBox_Level_Start;
    QDoubleSpinBox *spinBox_Level_Stop;
    QLineEdit *lineEdit_Channel;
    QSpinBox *spinBox_Acquisition_SwpInterval;
    QCheckBox *checkBox_Markers_Active;
    QPushButton *pushButton_Markers_Delta;
    QCheckBox *checkBox_Markers_PkTracking;
    QCheckBox *checkBox_Traces_Update;
    QCheckBox *checkBox_Traces_Hide;
    ColorPickerWidget *colorPicker;
    QComboBox *comboBox_Traces_Type;
    QComboBox *comboBox_Markers_PlaceOn;
    QSpinBox *spinBox_Traces_AvgCount;
    QLabel *label_CurrAvg;
    QComboBox *comboBox_Traces_CopyTo;
    QDoubleSpinBox *spinBox_Amplitude_RefLevel;
    QDoubleSpinBox *spinBox_Amplitude_Div;
    QComboBox *m_comboBoxMeasType;

    QMap<QString, Marker*> markers;  // Map marker names to Marker objects
    QString currentMarkerName;       // Name of the currently selected marker
    QLabel *markerLabel;

    static const int MAX_TRACES = 8;
    int currentTraceIndex; // Index of the currently selected trace (0-based)

    TraceData traces[MAX_TRACES];

    bool frequencyRangeChanged; // Flag to indicate frequency range was changed

    Rfmu2Tool *hardwareTool;

    QTabWidget *tabWidget;
    QWidget *tab_logArea;

    QTextBrowser *browser_NA;

    int dataCount;

    bool isFreqSweep(double epsilon);
    void AdjustSweepRange();

    QSpinBox *mPointsEdit;
    QComboBox *mPort1Edit;
    QComboBox *mPort2Edit;
    QLineEdit *mSingleFileEdit;
    QLineEdit *mDualFileEdit;

    void parseSinglePortData(Rfmu2NetworkAnalyzer::ResultType retType,
                                       const QVector<double> &rawData,
                                       QVector<double> &outS11_ampOrI,
                                       QVector<double> &outS11_phaseOrQ);
    void parseDualPortData(Rfmu2NetworkAnalyzer::ResultType retType,
                                     const QVector<double> &rawData,
                                     QVector<double> &s11_ampOrI,
                                     QVector<double> &s21_ampOrI,
                                     QVector<double> &s12_ampOrI,
                                     QVector<double> &s22_ampOrI,
                                     QVector<double> &s11_phaseOrQ,
                                     QVector<double> &s21_phaseOrQ,
                                     QVector<double> &s12_phaseOrQ,
                                     QVector<double> &s22_phaseOrQ);
    QVector<double>& getDataForTrace(int index);
    void allocateBuffers(int N);

public:
    void setTraceTypeForTrace(int targetTraceIndex, TraceType newType);
    void setColorForTrace(int targetTraceIndex, const QColor &newColor);

private:
    QString xAxisUnit() const;
    QString yAxisUnitForTrace(int traceIndex) const;

private:
    QThread *m_workerThread {nullptr};
    NAWorker *m_worker {nullptr};
    QVector<double> m_pendingRaw; // data handed over from worker
    Rfmu2NetworkAnalyzer::ResultType m_pendingType {};
    bool m_pendingReady {false};

signals:
    void requestSinglePort(Rfmu2NetworkAnalyzer::ResultType type);
    void requestDualPort(Rfmu2NetworkAnalyzer::ResultType type);

private slots:
    void onSinglePortDataReady(Rfmu2NetworkAnalyzer::ResultType type, const QVector<double> &raw);
    void onDualPortDataReady(Rfmu2NetworkAnalyzer::ResultType type, const QVector<double> &raw);
};

#endif // NAWIDGET_H
