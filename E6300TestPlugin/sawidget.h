#ifndef SAWIDGET_H
#define SAWIDGET_H

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
#include "include/qcustomplot.h"
#include "include/frequencyspinbox.h"
#include "marker.h"
#include "colorpickerwidget.h"
#include "collapsiblegroupbox.h"
#include "include/rfmu2/rfmu2tool.h"

class SAWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SAWidget(QWidget *parent = nullptr);
    ~SAWidget();

    enum Mode {
        SingleMode,
        AutoMode
    };

    enum TraceType {
        Off,
        ClearWrite,
        MaxHold,
        MinHold,
        MinMaxHold,
        Average
    };

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

signals:

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updatePlot();
    void updateFrequencyRange();
    void setMode(SAWidget::Mode mode);
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

    void onFreqPeakMeasureClicked();

private:
    void updateMarker(Marker *marker);
    void updateMarkerLabel();

    // Data acquisition function that always returns a fresh sweep
    void acquireSweepData(QVector<double> &outFreqs, QVector<double> &outAmps);

    // Helper functions for each type
    void applyClearWrite(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);
    void applyMaxHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);
    void applyMinHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);
    void applyMinMaxHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);
    void applyAverage(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps);

    void copyTraceData(int srcIndex, int destIndex);
    void revertCopyToComboBox();

    void computeMeanAndStdev(const QVector<double> &amps, double &mean, double &stdev);
    QVector<SAWidget::PeakInfo> findAllPeaksWithPlateaus(const QVector<double> &amps);
    bool checkExcursion(const QVector<double> &amps, int peakIndex, double pkExcurs);

    QCustomPlot *customPlot;
    QTimer *dataTimer;

    double pkThreshold; // For user-defined min amplitude
    double pkExcurs;    // For required amplitude drop to identify a distinct peak

    double startFrequency;
    double stopFrequency;

    Mode currentMode;

    FrequencySpinBox *spinBox_Frequency_Center;
    FrequencySpinBox *spinBox_Frequency_Start;
    FrequencySpinBox *spinBox_Frequency_Stop;
    QDoubleSpinBox *spinBox_Level;
    QLineEdit *lineEdit_Channel;
    QSpinBox *spinBox_receiveChannel;
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

    QMap<QString, Marker*> markers;  // Map marker names to Marker objects
    QString currentMarkerName;       // Name of the currently selected marker
    QLabel *markerLabel;

    static const int MAX_TRACES = 6;
    int currentTraceIndex; // Index of the currently selected trace (0-based)

    TraceData traces[MAX_TRACES];

    bool frequencyRangeChanged; // Flag to indicate frequency range was changed

    Rfmu2Tool *hardwareTool;

    QTextBrowser *browser_SA;
};

#endif // SAWIDGET_H
