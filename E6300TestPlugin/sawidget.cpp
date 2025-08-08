#include "sawidget.h"
#include "logging.h"

SAWidget::SAWidget(QWidget *parent)
    : QWidget{parent},
    customPlot(new QCustomPlot(this)),
    dataTimer(new QTimer(this)),
    pkThreshold(-100),
    pkExcurs(6),
    startFrequency(2.9995e9),
    stopFrequency(3.0005e9),
    currentMode(SingleMode),
    markerLabel(new QLabel(customPlot)),
    currentTraceIndex(0),
    frequencyRangeChanged(false),
    hardwareTool(nullptr)
{
    setMinimumWidth(1500);
    setMinimumHeight(800);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // Top horizontal bar
    QHBoxLayout *horiBar = new QHBoxLayout;

    horiBar->addStretch(1);

    QPushButton *pushButton_Single = new QPushButton("Single");
    pushButton_Single->setIcon(QIcon(":/images/icons/single.png"));
    connect(pushButton_Single, &QPushButton::clicked, this, [=]() {
        logger::log(browser_SA, QStringLiteral("[Mode] Switched to Single sweep mode."));
        setMode(SingleMode);
        updatePlot();
    });
    horiBar->addWidget(pushButton_Single);

    QPushButton *pushButton_Auto = new QPushButton("Auto");
    pushButton_Auto->setIcon(QIcon(":/images/icons/auto.png"));
    connect(pushButton_Auto, &QPushButton::clicked, this, [=]() {
        logger::log(browser_SA, QStringLiteral("[Mode] Switched to Automatic continuous sweep mode."));
        setMode(AutoMode);
        updatePlot();
    });
    horiBar->addWidget(pushButton_Auto);

    QPushButton *pushButton_Stop = new QPushButton("Stop");
    pushButton_Stop->setIcon(QIcon(":/images/icons/stop.png"));
    connect(pushButton_Stop, &QPushButton::clicked, this, [=]() {
        logger::log(browser_SA, QStringLiteral("[Mode] Sweep stopped by user."));
        dataTimer->stop();
    });
    horiBar->addWidget(pushButton_Stop);

    mainLayout->addLayout(horiBar);

    QSplitter *splitter_mainHorizontalLayout = new QSplitter(Qt::Horizontal, this);
    splitter_mainHorizontalLayout->setChildrenCollapsible(false);

    QGroupBox *groupBox_Left = new QGroupBox("", splitter_mainHorizontalLayout);
    QVBoxLayout *verticalLayout_Left = new QVBoxLayout;

    QLabel *label_Measurements = new QLabel("Measurements", groupBox_Left);
    label_Measurements->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label_Measurements->setStyleSheet("background-color: #2B3548; color: white; padding: 4px 10px;");
    verticalLayout_Left->addWidget(label_Measurements);

    // Traces group
    CollapsibleGroupBox *groupBox_Traces = new CollapsibleGroupBox("Traces", groupBox_Left);
    QVBoxLayout *vbox_Traces = new QVBoxLayout;
    QFormLayout *formLayout_Traces = new QFormLayout;

    QComboBox *comboBox_Traces_Number = new QComboBox;
    comboBox_Traces_Number->addItems({"One", "Two", "Three", "Four", "Five", "Six"});
    connect(comboBox_Traces_Number, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SAWidget::onCurrentTraceChanged);
    formLayout_Traces->addRow("Trace", comboBox_Traces_Number);

    comboBox_Traces_Type = new QComboBox;
    comboBox_Traces_Type->addItems({"Off", "Clear & Write", "Max Hold", "Min Hold", "Min/Max Hold", "Average"});
    comboBox_Traces_Type->setCurrentIndex(1);
    connect(comboBox_Traces_Type, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SAWidget::onTraceTypeChanged);
    formLayout_Traces->addRow("Type", comboBox_Traces_Type);

    spinBox_Traces_AvgCount = new QSpinBox;
    spinBox_Traces_AvgCount->setRange(2, 1000);
    spinBox_Traces_AvgCount->setValue(10);
    connect(spinBox_Traces_AvgCount,  QOverload<int>::of(&QSpinBox::valueChanged), this, &SAWidget::onAvgCountChanged);
    formLayout_Traces->addRow("Avg Count", spinBox_Traces_AvgCount);

    label_CurrAvg = new QLabel("CurrAvg: N/A");
    formLayout_Traces->addRow(label_CurrAvg);

    colorPicker = new ColorPickerWidget;
    connect(colorPicker, &ColorPickerWidget::colorChanged, this, &SAWidget::onPlotColorChanged);
    formLayout_Traces->addRow("Color", colorPicker);

    comboBox_Traces_CopyTo = new QComboBox;
    comboBox_Traces_CopyTo->addItems({"--", "Trace 1", "Trace 2", "Trace 3", "Trace 4", "Trace 5", "Trace 6"});
    connect(comboBox_Traces_CopyTo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SAWidget::onCopyToIndexChanged);
    formLayout_Traces->addRow("Copy To", comboBox_Traces_CopyTo);

    checkBox_Traces_Update = new QCheckBox("");
    checkBox_Traces_Update->setChecked(true);
    connect(checkBox_Traces_Update, &QCheckBox::stateChanged, this, &SAWidget::onTraceUpdateStateChanged);
    formLayout_Traces->addRow("Update", checkBox_Traces_Update);

    checkBox_Traces_Hide = new QCheckBox("");
    connect(checkBox_Traces_Hide, &QCheckBox::stateChanged, this, &SAWidget::onTraceHideStateChanged);
    formLayout_Traces->addRow("Hide", checkBox_Traces_Hide);

    vbox_Traces->addLayout(formLayout_Traces);
    QHBoxLayout *hbox_Traces = new QHBoxLayout;

    QPushButton *pushButton_Traces_Export = new QPushButton("Export");
    connect(pushButton_Traces_Export, &QPushButton::clicked, this, &SAWidget::onExportTraceData);
    hbox_Traces->addWidget(pushButton_Traces_Export);

    QPushButton *pushButton_Traces_Clear = new QPushButton("Clear");
    connect(pushButton_Traces_Clear, &QPushButton::clicked, this, &SAWidget::onTraceClearClicked);
    hbox_Traces->addWidget(pushButton_Traces_Clear);

    vbox_Traces->addLayout(hbox_Traces);
    groupBox_Traces->setContentLayout(vbox_Traces);
    verticalLayout_Left->addWidget(groupBox_Traces);

    // Markers group
    CollapsibleGroupBox *groupBox_Markers = new CollapsibleGroupBox("Markers", groupBox_Left);
    QVBoxLayout *vbox_Markers = new QVBoxLayout;
    QFormLayout *formLayout_Markers = new QFormLayout;

    QComboBox *comboBox_Markers_Marker = new QComboBox;
    comboBox_Markers_Marker->addItems({"Marker 1", "Marker 2", "Marker 3", "Marker 4", "Marker 5", "Marker 6", "Marker 7", "Marker 8", "Marker 9"});
    connect(comboBox_Markers_Marker, &QComboBox::currentTextChanged, this, &SAWidget::onCurrentMarkerChanged);
    formLayout_Markers->addRow("Marker", comboBox_Markers_Marker);

    QComboBox *comboBox_Markers_Type = new QComboBox;
    // comboBox_Markers_Type->addItems({"Normal", "Noise", "Channel Power", "N dB"});
    comboBox_Markers_Type->addItems({"Normal"});
    formLayout_Markers->addRow("Type", comboBox_Markers_Type);

    comboBox_Markers_PlaceOn = new QComboBox;
    comboBox_Markers_PlaceOn->addItems({"Trace One", "Trace Two", "Trace Three", "Trace Four", "Trace Five", "Trace Six"});
    connect(comboBox_Markers_PlaceOn, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SAWidget::onMarkerPlaceOnChanged);
    formLayout_Markers->addRow("Place On", comboBox_Markers_PlaceOn);

    FrequencySpinBox *spinBox_Markers_SetFreq = new FrequencySpinBox;
    spinBox_Markers_SetFreq->setFrequency(1e9);
    connect(spinBox_Markers_SetFreq, &FrequencySpinBox::frequencyChanged, this, &SAWidget::onValueOfSetFreqChanged);
    formLayout_Markers->addRow("Set Freq", spinBox_Markers_SetFreq);

    // QCheckBox *checkBox_Markers_Update = new QCheckBox("");
    // formLayout_Markers->addRow("Update", checkBox_Markers_Update);

    checkBox_Markers_Active = new QCheckBox("");
    checkBox_Markers_Active->setChecked(false);
    connect(checkBox_Markers_Active, &QCheckBox::stateChanged, this, &SAWidget::onMarkerActiveStateChanged);
    formLayout_Markers->addRow("Active", checkBox_Markers_Active);

    checkBox_Markers_PkTracking = new QCheckBox("");
    connect(checkBox_Markers_PkTracking, &QCheckBox::stateChanged, this, &SAWidget::onPkTrackingStateChanged);
    formLayout_Markers->addRow("Pk Tracking", checkBox_Markers_PkTracking);

    QDoubleSpinBox *spinBox_Markers_PkThreshold = new QDoubleSpinBox;
    spinBox_Markers_PkThreshold->setRange(-10000, 10000);
    spinBox_Markers_PkThreshold->setDecimals(3);
    spinBox_Markers_PkThreshold->setSuffix(" dBm");
    spinBox_Markers_PkThreshold->setValue(-100);
    connect(spinBox_Markers_PkThreshold, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) { pkThreshold = val; });
    formLayout_Markers->addRow("Pk Threshold", spinBox_Markers_PkThreshold);

    QDoubleSpinBox *spinBox_PkExcurs = new QDoubleSpinBox;
    spinBox_PkExcurs->setRange(0.1, 100);
    spinBox_PkExcurs->setDecimals(2);
    spinBox_PkExcurs->setSuffix(" dB");
    spinBox_PkExcurs->setValue(6);
    connect(spinBox_PkExcurs, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) { pkExcurs = val; });
    formLayout_Markers->addRow("Pk Excurs.", spinBox_PkExcurs);

    vbox_Markers->addLayout(formLayout_Markers);
    QGridLayout *gridLayout_Markers = new QGridLayout;

    QPushButton *pushButton_Markers_PeakSearch = new QPushButton("Peak Search");
    connect(pushButton_Markers_PeakSearch, &QPushButton::clicked, this, &SAWidget::peakSearch);

    pushButton_Markers_Delta = new QPushButton("Delta");
    pushButton_Markers_Delta->setCheckable(true);
    pushButton_Markers_Delta->setStyleSheet("QPushButton { background-color : #EFEFF2; }"
                                            "QPushButton:checked { background-color : #E5C365 }");
    pushButton_Markers_Delta->setChecked(false);
    connect(pushButton_Markers_Delta, &QPushButton::clicked, this, &SAWidget::onDeltaButtonClicked);

    QPushButton *pushButton_Markers_ToCenter = new QPushButton("To Center");
    connect(pushButton_Markers_ToCenter, &QPushButton::clicked, this, &SAWidget::onMarkerToCenterClicked);
    QPushButton *pushButton_Markers_ToRef = new QPushButton("To Ref");
    connect(pushButton_Markers_ToRef, &QPushButton::clicked, this, &SAWidget::onMarkerToRefLevelClicked);
    QPushButton *pushButton_Markers_PeakLeft = new QPushButton("Peak Left");
    connect(pushButton_Markers_PeakLeft, &QPushButton::clicked, this, &SAWidget::onPeakLeftClicked);
    QPushButton *pushButton_Markers_PeakRight = new QPushButton("Peak Right");
    connect(pushButton_Markers_PeakRight, &QPushButton::clicked, this, &SAWidget::onPeakRightClicked);
    QPushButton *pushButton_Markers_MinPeak = new QPushButton("Min Peak");
    connect(pushButton_Markers_MinPeak, &QPushButton::clicked, this, &SAWidget::onMinPeakClicked);
    QPushButton *pushButton_Markers_NextPeak = new QPushButton("Next Peak");
    connect(pushButton_Markers_NextPeak, &QPushButton::clicked, this, &SAWidget::onNextPeakClicked);
    QPushButton *pushButton_Markers_DisableAll = new QPushButton("Disable All");
    connect(pushButton_Markers_DisableAll, &QPushButton::clicked, this, [this]() {
        for (auto &marker : markers) {
            marker->active = false;
            marker->setVisible(false);
        }
        bool wasBlocked = checkBox_Markers_Active->blockSignals(true);
        checkBox_Markers_Active->setChecked(false);
        checkBox_Markers_Active->blockSignals(wasBlocked);
        updateMarkerLabel();
        customPlot->replot();
    });

    gridLayout_Markers->addWidget(pushButton_Markers_PeakSearch, 0, 0);
    gridLayout_Markers->addWidget(pushButton_Markers_Delta, 0, 1);
    gridLayout_Markers->addWidget(pushButton_Markers_ToCenter, 1, 0);
    gridLayout_Markers->addWidget(pushButton_Markers_ToRef, 1, 1);
    gridLayout_Markers->addWidget(pushButton_Markers_PeakLeft, 2, 0);
    gridLayout_Markers->addWidget(pushButton_Markers_PeakRight, 2, 1);
    gridLayout_Markers->addWidget(pushButton_Markers_MinPeak, 3, 0);
    gridLayout_Markers->addWidget(pushButton_Markers_NextPeak, 3, 1);
    gridLayout_Markers->addWidget(pushButton_Markers_DisableAll, 4, 0, 1, 2);
    vbox_Markers->addLayout(gridLayout_Markers);
    groupBox_Markers->setContentLayout(vbox_Markers);
    verticalLayout_Left->addWidget(groupBox_Markers);

    verticalLayout_Left->addStretch();
    groupBox_Left->setLayout(verticalLayout_Left);
    splitter_mainHorizontalLayout->addWidget(groupBox_Left);

    QSplitter *splitter_Middle = new QSplitter(Qt::Vertical, splitter_mainHorizontalLayout);
    splitter_Middle->setChildrenCollapsible(false);

    // Setup multiple graphs for multiple traces
    for (int i = 0; i < MAX_TRACES; ++i) {
        customPlot->addGraph();
        traces[i].updateEnabled = true;
        traces[i].hide = false;
        traces[i].type = (i == 0) ? ClearWrite : Off;
        traces[i].color = Qt::darkBlue;
        traces[i].avgCount = 10;

        customPlot->graph(i)->setPen(QPen(traces[i].color));
    }

    // Add the min line graphs for Min/Max hold
    for (int i = 0; i < MAX_TRACES; ++i) {
        customPlot->addGraph(); // min line = graph(i + MAX_TRACES)
        QColor minColor = traces[i].color;
        customPlot->graph(i + MAX_TRACES)->setPen(QPen(minColor));
        customPlot->graph(i + MAX_TRACES)->setVisible(false);
    }

    customPlot->xAxis->setLabel("Frequency (Hz)");
    customPlot->yAxis->setLabel("Amplitude (dBm)");
    // Initial plot range
    customPlot->xAxis->setRange(startFrequency, stopFrequency);
    customPlot->yAxis->setRange(-120, -20);
    splitter_Middle->addWidget(customPlot);

    QTabWidget *tabWidget = new QTabWidget(splitter_Middle);
    tabWidget->setStyleSheet(
        "QTabBar::tab { background: #CCCEDB; color: #000000; }"
        "QTabBar::tab:selected { background: #2B3548; color: #FFFFFF; }"
        "QTabBar::tab:hover { background: #FFF4BF; color: #000000; }"
        "QWidget { background-color: #EFEFF2; }"
        );
    tabWidget->setMovable(true);
    tabWidget->setTabPosition(QTabWidget::South);

    QWidget *tab_logArea = new QWidget(tabWidget);
    QVBoxLayout *verticalLayout_SA_logArea = new QVBoxLayout;
    browser_SA = new QTextBrowser;
    browser_SA->setStyleSheet("QTextBrowser { border: none; }");
    browser_SA->document()->setMaximumBlockCount(20);
    verticalLayout_SA_logArea->addWidget(browser_SA);
    tab_logArea->setLayout(verticalLayout_SA_logArea);
    tabWidget->addTab(tab_logArea, "General Messages");

    splitter_Middle->addWidget(tabWidget);

    splitter_mainHorizontalLayout->addWidget(splitter_Middle);

    QGroupBox *groupBox_Right = new QGroupBox("", splitter_mainHorizontalLayout);
    QVBoxLayout *verticalLayout_Right = new QVBoxLayout;
    QLabel *label_SweepSettings = new QLabel("Sweep Settings");
    label_SweepSettings->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label_SweepSettings->setStyleSheet("background-color: #2B3548; color: white; padding: 4px 10px;");
    verticalLayout_Right->addWidget(label_SweepSettings);

    // Frequency controls
    CollapsibleGroupBox *groupBox_Frequency = new CollapsibleGroupBox("Frequency", groupBox_Right);
    QVBoxLayout *vbox_Frequency = new QVBoxLayout;
    QFormLayout *formLayout_Frequency = new QFormLayout;

    spinBox_Frequency_Center = new FrequencySpinBox;
    spinBox_Frequency_Center->setFrequency(3e9);
    connect(spinBox_Frequency_Center, &FrequencySpinBox::frequencyChanged, this, &SAWidget::updateFrequencyRange);
    formLayout_Frequency->addRow("Center", spinBox_Frequency_Center);

    spinBox_Level = new QDoubleSpinBox;
    spinBox_Level->setRange(-100, 10);
    spinBox_Level->setDecimals(1);
    spinBox_Level->setSuffix(" dBm");
    spinBox_Level->setValue(-5);
    formLayout_Frequency->addRow("Level", spinBox_Level);

    FrequencySpinBox *spinBox_Frequency_Span = new FrequencySpinBox;
    spinBox_Frequency_Span->setFrequency(1e6);
    spinBox_Frequency_Span->setEnabled(false);
    formLayout_Frequency->addRow("Span", spinBox_Frequency_Span);

    spinBox_Frequency_Start = new FrequencySpinBox;
    spinBox_Frequency_Start->setFrequency(startFrequency);
    connect(spinBox_Frequency_Start, &FrequencySpinBox::frequencyChanged, this, &SAWidget::updateFrequencyRange);
    // formLayout_Frequency->addRow("Start", spinBox_Frequency_Start);

    spinBox_Frequency_Stop = new FrequencySpinBox;
    spinBox_Frequency_Stop->setFrequency(stopFrequency);
    connect(spinBox_Frequency_Stop, &FrequencySpinBox::frequencyChanged, this, &SAWidget::updateFrequencyRange);
    // formLayout_Frequency->addRow("Stop", spinBox_Frequency_Stop);

    FrequencySpinBox *spinBox_Frequency_Step = new FrequencySpinBox;
    spinBox_Frequency_Step->setFrequency(20e6);
    // formLayout_Frequency->addRow("Step", spinBox_Frequency_Step);

    spinBox_receiveChannel = new QSpinBox;
    spinBox_receiveChannel->setRange(1, 4);
    spinBox_receiveChannel->setValue(3);
    formLayout_Frequency->addRow("Receive(RX)", spinBox_receiveChannel);

    comboBox_Channel = new QComboBox;
    const QStringList chans = {
                               "01A","01B","01C","01D",
                               "02A","02B","02C","02D",
                               "03A","03B","03C","03D",
                               "04A","04B","04C","04D"};
    comboBox_Channel->addItems(chans);
    comboBox_Channel->setCurrentText("02A");
    formLayout_Frequency->addRow("Channel", comboBox_Channel);

    QPushButton *pushButton_Frequency_PeakMeasure = new QPushButton("Peak Measure");
    connect(pushButton_Frequency_PeakMeasure, &QPushButton::clicked, this, &SAWidget::onFreqPeakMeasureClicked);
    formLayout_Frequency->addRow(pushButton_Frequency_PeakMeasure);

    vbox_Frequency->addLayout(formLayout_Frequency);
    QHBoxLayout *hbox_Frequency = new QHBoxLayout;
    // QPushButton *pushButton_Frequency_FullSpan = new QPushButton("Full Span");
    // QPushButton *pushButton_Frequency_ZeroSpan = new QPushButton("Zero Span");
    // hbox_Frequency->addWidget(pushButton_Frequency_FullSpan);
    // hbox_Frequency->addWidget(pushButton_Frequency_ZeroSpan);
    // QPushButton *pushButton_Frequency_PeakMeasure = new QPushButton("Peak Measure");
    // hbox_Frequency->addWidget(pushButton_Frequency_PeakMeasure);
    vbox_Frequency->addLayout(hbox_Frequency);
    groupBox_Frequency->setContentLayout(vbox_Frequency);
    verticalLayout_Right->addWidget(groupBox_Frequency);

    // Amplitude
    CollapsibleGroupBox *groupBox_Amplitude = new CollapsibleGroupBox("Amplitude", groupBox_Right);
    QFormLayout *formLayout_Amplitude = new QFormLayout;

    spinBox_Amplitude_RefLevel = new QDoubleSpinBox;
    spinBox_Amplitude_RefLevel->setRange(-130, 100);
    spinBox_Amplitude_RefLevel->setDecimals(3);
    spinBox_Amplitude_RefLevel->setSuffix(" dBm");
    spinBox_Amplitude_RefLevel->setValue(-20);
    connect(spinBox_Amplitude_RefLevel, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SAWidget::onRefLevelChanged);
    formLayout_Amplitude->addRow("Ref Level", spinBox_Amplitude_RefLevel);

    spinBox_Amplitude_Div = new QDoubleSpinBox;
    spinBox_Amplitude_Div->setRange(0.1, 30);
    spinBox_Amplitude_Div->setDecimals(1);
    spinBox_Amplitude_Div->setSuffix(" dB");
    spinBox_Amplitude_Div->setValue(10);
    connect(spinBox_Amplitude_Div, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SAWidget::onDivChanged);
    formLayout_Amplitude->addRow("Div", spinBox_Amplitude_Div);

    spinBox_Amplitude_RefLevel->setSingleStep(spinBox_Amplitude_Div->value());

    QComboBox *comboBox_Amplitude_Gain = new QComboBox;
    comboBox_Amplitude_Gain->addItems({"Auto Gain"});
    // formLayout_Amplitude->addRow("Gain", comboBox_Amplitude_Gain);
    QComboBox *comboBox_Amplitude_Atten = new QComboBox;
    comboBox_Amplitude_Atten->addItems({"Auto Atten"});
    // formLayout_Amplitude->addRow("Atten", comboBox_Amplitude_Atten);
    QComboBox *comboBox_Amplitude_Preamp = new QComboBox;
    comboBox_Amplitude_Preamp->addItems({"Auto"});
    // formLayout_Amplitude->addRow("Preamp", comboBox_Amplitude_Preamp);
    groupBox_Amplitude->setContentLayout(formLayout_Amplitude);
    verticalLayout_Right->addWidget(groupBox_Amplitude);

    // Bandwidth
    CollapsibleGroupBox *groupBox_Bandwidth = new CollapsibleGroupBox("Bandwidth", groupBox_Right);
    QFormLayout *formLayout_Bandwidth = new QFormLayout;
    QComboBox *comboBox_Bandwidth_RBWShape = new QComboBox;
    comboBox_Bandwidth_RBWShape->addItems({"Flat Top", "Nutall", "CISPR(Gaussian)"});
    formLayout_Bandwidth->addRow("RBW Shape", comboBox_Bandwidth_RBWShape);
    QSpinBox *spinBox_Bandwidth_RBW = new QSpinBox;
    spinBox_Bandwidth_RBW->setRange(1, 3e8);
    spinBox_Bandwidth_RBW->setSuffix(" Hz");
    spinBox_Bandwidth_RBW->setValue(3e5);
    formLayout_Bandwidth->addRow("RBW", spinBox_Bandwidth_RBW);
    QSpinBox *spinBox_Bandwidth_VBW = new QSpinBox;
    spinBox_Bandwidth_VBW->setRange(1, 3e8);
    spinBox_Bandwidth_VBW->setSuffix(" Hz");
    spinBox_Bandwidth_VBW->setValue(3e5);
    formLayout_Bandwidth->addRow("VBW", spinBox_Bandwidth_VBW);
    QCheckBox *checkBox_Bandwidth_AutoRBW = new QCheckBox("");
    formLayout_Bandwidth->addRow("Auto RBW", checkBox_Bandwidth_AutoRBW);
    QCheckBox *checkBox_Bandwidth_AutoVBW = new QCheckBox("");
    formLayout_Bandwidth->addRow("Auto VBW", checkBox_Bandwidth_AutoVBW);
    groupBox_Bandwidth->setContentLayout(formLayout_Bandwidth);
    // verticalLayout_Right->addWidget(groupBox_Bandwidth);

    // Acquisition
    CollapsibleGroupBox *groupBox_Acquisition = new CollapsibleGroupBox("Acquisition", groupBox_Right);
    QFormLayout *formLayout_Acquisition = new QFormLayout;

    QComboBox *comboBox_Acquisition_VideoUnits = new QComboBox;
    comboBox_Acquisition_VideoUnits->addItems({"Log", "Voltage", "Power", "Sample"});
    // formLayout_Acquisition->addRow("Video Units", comboBox_Acquisition_VideoUnits);

    QComboBox *comboBox_Acquisition_Detector = new QComboBox;
    comboBox_Acquisition_Detector->addItems({"Min/Max", "Average", "Max", "Min"});
    // formLayout_Acquisition->addRow("Detector", comboBox_Acquisition_Detector);

    QSpinBox *spinBox_Acquisition_SwpTime = new QSpinBox;
    spinBox_Acquisition_SwpTime->setRange(1, 1e3);
    spinBox_Acquisition_SwpTime->setSuffix(" ms");
    spinBox_Acquisition_SwpTime->setValue(1);
    // formLayout_Acquisition->addRow("Swp Time", spinBox_Acquisition_SwpTime);

    spinBox_Acquisition_SwpInterval = new QSpinBox;
    spinBox_Acquisition_SwpInterval->setRange(1, 1e6);
    spinBox_Acquisition_SwpInterval->setSuffix(" ms");
    spinBox_Acquisition_SwpInterval->setValue(50);
    connect(spinBox_Acquisition_SwpInterval, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) { dataTimer->setInterval(value); });
    formLayout_Acquisition->addRow("Swp Interval", spinBox_Acquisition_SwpInterval);

    groupBox_Acquisition->setContentLayout(formLayout_Acquisition);
    verticalLayout_Right->addWidget(groupBox_Acquisition);

    verticalLayout_Right->addStretch();
    groupBox_Right->setLayout(verticalLayout_Right);
    splitter_mainHorizontalLayout->addWidget(groupBox_Right);

    mainLayout->addWidget(splitter_mainHorizontalLayout);

    splitter_Middle->setSizes({300, 100});
    splitter_mainHorizontalLayout->setSizes({100, 800, 100});

    setLayout(mainLayout);

    // Initialize markers
    for (int i = 1; i <= 9; ++i) {
        QString markerName = QString("Marker %1").arg(i);
        markers[markerName] = new Marker(customPlot, markerName);
    }

    markerLabel->setStyleSheet("QLabel { background-color : transparent; color : black; }");
    markerLabel->setVisible(false);  // Initially hidden

    // Default current marker
    currentMarkerName = "Marker 1";

    // Event filter for mouse clicks
    customPlot->setInteraction(QCP::iSelectPlottables, true);
    customPlot->installEventFilter(this);

    // Setup timer for auto mode updates
    dataTimer->setInterval(spinBox_Acquisition_SwpInterval->value());
    connect(dataTimer, &QTimer::timeout, this, &SAWidget::updatePlot);

    // setMode(AutoMode); // Set initial mode to AutoMode
}

SAWidget::~SAWidget()
{
}

void SAWidget::setMode(SAWidget::Mode mode) {
    currentMode = mode;
    if (currentMode == AutoMode) {
        // Start continuous data updates
        dataTimer->start();
    } else {
        // Stop continuous updates
        dataTimer->stop();
    }
}

void SAWidget::updatePlot()
{
    if (frequencyRangeChanged) {
        for (int i = 0; i < MAX_TRACES; ++i) {
            // Clear arrays to start fresh
            traces[i].freqs.clear();
            traces[i].amps.clear();
            traces[i].minAmps.clear();
            traces[i].sumAmps.clear();
            traces[i].lastSweeps.clear();
        }

        // Reset the flag
        frequencyRangeChanged = false;
    }

    QVector<double> tmpFreqs, tmpAmps;
    bool needData = false;
    for (int i = 0; i < MAX_TRACES; ++i)
    {
        if (traces[i].updateEnabled && traces[i].type != Off) {
            needData = true;
            break;
        }
    }

    if (needData)
    {
        acquireSweepData(tmpFreqs, tmpAmps);
    }

    for (int i = 0; i < MAX_TRACES; ++i) {
        if (traces[i].type == Off) {
            customPlot->graph(i)->setVisible(false);
            customPlot->graph(i + MAX_TRACES)->setVisible(false);
            traces[i].freqs.clear();
            traces[i].amps.clear();
            traces[i].minAmps.clear();
            traces[i].sumAmps.clear();
            traces[i].lastSweeps.clear();
            continue;
        }

        // If this trace updates and is not Off, attempt to acquire new data
        QVector<double> newFreqs, newAmps;
        bool newDataAcquired = false;

        if (traces[i].updateEnabled) {
            // acquireSweepData(newFreqs, newAmps);
            newFreqs = tmpFreqs;
            newAmps = tmpAmps;
            newDataAcquired = true;
        }

        // Apply type-specific logic
        switch (traces[i].type) {
        case ClearWrite:
            if (newDataAcquired)
                applyClearWrite(i, newFreqs, newAmps);
            customPlot->graph(i)->setData(traces[i].freqs, traces[i].amps);
            customPlot->graph(i)->setVisible(!traces[i].hide);
            customPlot->graph(i + MAX_TRACES)->setVisible(false);
            customPlot->graph(i)->setBrush(Qt::NoBrush);
            customPlot->graph(i)->setChannelFillGraph(nullptr);
            break;

        case MaxHold:
            if (newDataAcquired)
                applyMaxHold(i, newFreqs, newAmps);
            customPlot->graph(i)->setData(traces[i].freqs, traces[i].amps);
            customPlot->graph(i)->setVisible(!traces[i].hide);
            customPlot->graph(i + MAX_TRACES)->setVisible(false);
            customPlot->graph(i)->setBrush(Qt::NoBrush);
            customPlot->graph(i)->setChannelFillGraph(nullptr);
            break;

        case MinHold:
            if (newDataAcquired)
                applyMinHold(i, newFreqs, newAmps);
            // For MinHold, display only one line (use main line graph(i))
            customPlot->graph(i)->setData(traces[i].freqs, traces[i].amps);
            customPlot->graph(i)->setVisible(!traces[i].hide);
            customPlot->graph(i + MAX_TRACES)->setVisible(false);
            customPlot->graph(i)->setBrush(Qt::NoBrush);
            customPlot->graph(i)->setChannelFillGraph(nullptr);
            break;

        case MinMaxHold:
        {
            if (newDataAcquired)
                applyMinMaxHold(i, newFreqs, newAmps);
            // For MinMaxHold:
            // graph(i) = max line, graph(i + MAX_TRACES) = min line
            if (traces[i].freqs.isEmpty() || traces[i].amps.isEmpty() || traces[i].minAmps.isEmpty()) {
                // If no data yet, provide empty sets
                customPlot->graph(i)->setData(QVector<double>(), QVector<double>());
                customPlot->graph(i + MAX_TRACES)->setData(QVector<double>(), QVector<double>());
            } else {
                // Use amps for main (max) line, minAmps for min line
                customPlot->graph(i)->setData(traces[i].freqs, traces[i].amps);
                customPlot->graph(i + MAX_TRACES)->setData(traces[i].freqs, traces[i].minAmps);
            }
            customPlot->graph(i)->setVisible(!traces[i].hide);
            customPlot->graph(i + MAX_TRACES)->setVisible(!traces[i].hide);

            QColor fillColor = traces[i].color;
            fillColor.setAlpha(50);
            customPlot->graph(i)->setBrush(fillColor);
            customPlot->graph(i)->setChannelFillGraph(customPlot->graph(i + MAX_TRACES));
            break;
        }

        case Average:
            if (newDataAcquired)
                applyAverage(i, newFreqs, newAmps);

            customPlot->graph(i)->setData(traces[i].freqs, traces[i].amps);
            customPlot->graph(i)->setVisible(!traces[i].hide);
            customPlot->graph(i + MAX_TRACES)->setVisible(false);
            customPlot->graph(i)->setBrush(Qt::NoBrush);
            customPlot->graph(i)->setChannelFillGraph(nullptr);
            break;

        default:
            break;
        }

        if (traces[i].type == Average && i == currentTraceIndex) {
            // If we have lastSweeps, show the current count
            int currCount = traces[i].lastSweeps.size();
            label_CurrAvg->setText(QString("CurrAvg: %1").arg(currCount));
        }
    }

    // Update markers
    for (auto &marker : markers) {
        if (marker->active) {
            int tIndex = marker->traceIndex;
            if (marker->pkTracking && traces[tIndex].updateEnabled && traces[tIndex].type != Off) {
                // For pkTracking, we choose a line to find the max from
                const auto &ampsToCheck = traces[tIndex].amps;
                if (!ampsToCheck.isEmpty()) {
                    auto maxIt = std::max_element(ampsToCheck.begin(), ampsToCheck.end());
                    marker->index = std::distance(ampsToCheck.begin(), maxIt);
                }
            }
            updateMarker(marker);
        }
    }

    updateMarkerLabel();
    customPlot->replot();
}

void SAWidget::acquireSweepData(QVector<double> &outFreqs, QVector<double> &outAmps)
{
    static constexpr int kExpectedPoints = 411; // hardware spec

    if (!hardwareTool || !hardwareTool->spectrumAnalyzer()) {
        logger::log(browser_SA, QStringLiteral("Spectrum Analyzer is null!"));
    }

    int freqKHz  = static_cast<int>(spinBox_Frequency_Center->frequency() / 1000.0);
    double level = spinBox_Level->value();
    int receive = spinBox_receiveChannel->value();
    QString chan = comboBox_Channel->currentText();

    logger::log(browser_SA, QString("[Spectrum] Starting measure with freq=%1 kHz, level=%2 dB, receive=%3, chan=%4")
                                .arg(freqKHz).arg(level).arg(receive).arg(chan));

    auto rawAmps = hardwareTool->spectrumAnalyzer()->measureRawData(freqKHz, level, receive, chan);
    logger::log(browser_SA, QStringLiteral("[Spectrum] measureRawData() returned."));

    if (rawAmps.size() != kExpectedPoints) {
        logger::log(browser_SA,
                    tr("[Spectrum] discarded sweep - expected %1 points, got %2")
                        .arg(kExpectedPoints)
                        .arg(rawAmps.size()));
        outFreqs.clear();
        outAmps.clear();
        return;
    }

    outAmps = rawAmps;
    outFreqs.resize(kExpectedPoints);
    const double step = (stopFrequency - startFrequency) / double(kExpectedPoints - 1);

    for (int i = 0; i < kExpectedPoints; ++i)
        outFreqs[i] = startFrequency + i * step;
}

void SAWidget::applyClearWrite(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps)
{
    // ClearWrite just replaces old data with new data
    traces[traceIndex].freqs = newFreqs;
    traces[traceIndex].amps = newAmps;
}

void SAWidget::applyMaxHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps)
{
    if (traces[traceIndex].freqs.isEmpty()) {
        // First sweep for MaxHold
        traces[traceIndex].freqs = newFreqs;
        traces[traceIndex].amps = newAmps;
    } else {
        int size = qMin(traces[traceIndex].amps.size(), newAmps.size());
        for (int j = 0; j < size; ++j) {
            if (newAmps[j] > traces[traceIndex].amps[j]) {
                traces[traceIndex].amps[j] = newAmps[j];
            }
        }
    }
}

void SAWidget::applyMinHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps)
{
    if (traces[traceIndex].freqs.isEmpty()) {
        // First sweep for MinHold
        traces[traceIndex].freqs = newFreqs;
        traces[traceIndex].amps = newAmps;
    } else {
        int size = qMin(traces[traceIndex].amps.size(), newAmps.size());
        for (int j = 0; j < size; ++j) {
            if (newAmps[j] < traces[traceIndex].amps[j]) {
                traces[traceIndex].amps[j] = newAmps[j];
            }
        }
    }
}

void SAWidget::applyMinMaxHold(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps)
{
    if (traces[traceIndex].freqs.isEmpty()) {
        // First sweep for MinMaxHold: amps = max line, minAmps = min line
        traces[traceIndex].freqs = newFreqs;
        traces[traceIndex].amps = newAmps;     // amps store max line
        traces[traceIndex].minAmps = newAmps;  // minAmps store min line
    } else {
        int size = qMin(traces[traceIndex].freqs.size(), newAmps.size());
        for (int j = 0; j < size; ++j) {
            if (newAmps[j] < traces[traceIndex].minAmps[j]) {
                traces[traceIndex].minAmps[j] = newAmps[j];
            }
            if (newAmps[j] > traces[traceIndex].amps[j]) {
                traces[traceIndex].amps[j] = newAmps[j];
            }
        }
    }
}

void SAWidget::applyAverage(int traceIndex, const QVector<double> &newFreqs, const QVector<double> &newAmps)
{
    TraceData &td = traces[traceIndex];

    // If first sweep in Average mode:
    if (td.freqs.isEmpty()) {
        td.freqs = newFreqs;
        td.amps = newAmps;
        td.sumAmps = newAmps; // sumAmps starts as the first sweep
        td.lastSweeps.clear();
        td.lastSweeps.append(newAmps);
    } else {
        int size = qMin(td.freqs.size(), newAmps.size());

        // If we don't have avgCount sweeps yet:
        if (td.lastSweeps.size() < td.avgCount) {
            // Add the new sweep to sumAmps
            for (int j = 0; j < size; ++j) {
                td.sumAmps[j] += newAmps[j];
            }
            td.lastSweeps.append(newAmps);

            // Compute new average
            int count = td.lastSweeps.size();
            td.amps.resize(size);
            for (int j = 0; j < size; ++j) {
                td.amps[j] = td.sumAmps[j] / count;
            }

        } else {
            // We have avgCount sweeps, remove the oldest and add the new one
            const QVector<double> &oldest = td.lastSweeps.first();
            // Subtract oldest from sumAmps
            for (int j = 0; j < size; ++j) {
                td.sumAmps[j] = td.sumAmps[j] - oldest[j] + newAmps[j];
            }
            td.lastSweeps.pop_front();
            td.lastSweeps.append(newAmps);

            // Compute new average (with avgCount sweeps)
            for (int j = 0; j < size; ++j) {
                td.amps[j] = td.sumAmps[j] / td.avgCount;
            }
        }
    }
}

void SAWidget::updateFrequencyRange() {
    double centerFrequency = spinBox_Frequency_Center->frequency();
    double spanFrequency = 1e6;
    startFrequency = centerFrequency - spanFrequency / 2;
    stopFrequency = centerFrequency + spanFrequency / 2;
    customPlot->xAxis->setRange(startFrequency, stopFrequency);

    frequencyRangeChanged = true;
    updatePlot();
}

bool SAWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == customPlot && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        placeMarkerAtClick(mouseEvent);
        return true;
    }
    return QWidget::eventFilter(obj, event);
}

void SAWidget::placeMarkerAtClick(QMouseEvent *event)
{
    if (!customPlot->viewport().contains(event->pos())) return;

    Marker *marker = markers[currentMarkerName];
    int tIndex = marker->traceIndex;
    auto &freqs = traces[tIndex].freqs;
    if (freqs.isEmpty()) return;

    double x = customPlot->xAxis->pixelToCoord(event->pos().x());
    // Find closest data point index
    auto it = std::lower_bound(freqs.begin(), freqs.end(), x);
    int index = std::distance(freqs.begin(), it);
    if (index >= freqs.size()) index = freqs.size() - 1;

    marker->index = index;
    marker->active = true;
    bool wasBlocked = checkBox_Markers_Active->blockSignals(true);
    checkBox_Markers_Active->setChecked(true);
    checkBox_Markers_Active->blockSignals(wasBlocked);

    updateMarker(marker);
    customPlot->replot();
}

void SAWidget::updateMarker(Marker *marker)
{
    int tIndex = marker->traceIndex;
    int index = marker->index;
    if (!marker->active || tIndex < 0 || tIndex >= MAX_TRACES || index < 0 || index >= traces[tIndex].freqs.size())
        return;

    double freq = traces[tIndex].freqs[index];
    double amp = traces[tIndex].amps[index];

    marker->tracer->setGraph(customPlot->graph(tIndex));
    marker->deltaTracer->setGraph(customPlot->graph(tIndex));
    marker->tracer->setInterpolating(false);
    marker->setVisible(true);
    marker->setPosition(freq, amp);

    updateMarkerLabel();
}

void SAWidget::peakSearch()
{
    Marker *marker = markers[currentMarkerName];
    int tIndex = marker->traceIndex;
    if (traces[tIndex].freqs.isEmpty()) return;
    const auto &ampsToCheck = traces[tIndex].amps;
    if (ampsToCheck.isEmpty()) return;

    auto maxIt = std::max_element(ampsToCheck.begin(), ampsToCheck.end());
    int index = std::distance(ampsToCheck.begin(), maxIt);

    marker->index = index;
    marker->active = true;
    bool wasBlocked = checkBox_Markers_Active->blockSignals(true);
    checkBox_Markers_Active->setChecked(true);
    checkBox_Markers_Active->blockSignals(wasBlocked);
    updateMarker(marker);
    customPlot->replot();
}

void SAWidget::keyPressEvent(QKeyEvent *event)
{
    Marker *marker = markers[currentMarkerName];
    if (marker->active) {
        int tIndex = marker->traceIndex;
        if (tIndex >= 0 && tIndex < MAX_TRACES) {
            const auto &freqs = traces[tIndex].freqs;
            if (freqs.isEmpty()) {
                // No data points, can't move
                QWidget::keyPressEvent(event);
                return;
            }

            if (event->key() == Qt::Key_Left && marker->index > 0) {
                marker->index--;
                updateMarker(marker);
                customPlot->replot();
            } else if (event->key() == Qt::Key_Right && marker->index < freqs.size() - 1) {
                marker->index++;
                updateMarker(marker);
                customPlot->replot();
            }
        }
    }
    QWidget::keyPressEvent(event);
}

void SAWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Update the label position
    markerLabel->move(customPlot->width() - markerLabel->width() - 10, 10);
}

void SAWidget::onCurrentMarkerChanged(const QString &markerName)
{
    currentMarkerName = markerName;
    Marker *marker = markers[currentMarkerName];

    bool wasBlocked = checkBox_Markers_Active->blockSignals(true);
    checkBox_Markers_Active->setChecked(marker->active);
    checkBox_Markers_Active->blockSignals(wasBlocked);

    // Update "Pk Tracking" checkbox
    wasBlocked = checkBox_Markers_PkTracking->blockSignals(true);
    checkBox_Markers_PkTracking->setChecked(marker->pkTracking);
    checkBox_Markers_PkTracking->blockSignals(wasBlocked);

    pushButton_Markers_Delta->setChecked(marker->deltaMode);

    wasBlocked = comboBox_Markers_PlaceOn->blockSignals(true);
    comboBox_Markers_PlaceOn->setCurrentIndex(marker->traceIndex);
    comboBox_Markers_PlaceOn->blockSignals(wasBlocked);
}

void SAWidget::onMarkerActiveStateChanged(int state)
{
    Marker *marker = markers[currentMarkerName];
    int tIndex = marker->traceIndex; // Trace the marker currently belongs to

    if (state == Qt::Checked) {
        // Activate the marker
        marker->active = true;

        // Ensure index is within range for this trace
        if (tIndex < 0 || tIndex >= MAX_TRACES || traces[tIndex].freqs.isEmpty()) {
            // If invalid trace or no data, set index = 0
            marker->index = 0;
        } else if (marker->index < 0 || marker->index >= traces[tIndex].freqs.size()) {
            marker->index = traces[tIndex].freqs.size() / 2;
        }

        updateMarker(marker);
        customPlot->replot();
    } else {
        // Deactivate the marker
        marker->active = false;
        marker->setVisible(false);
        updateMarkerLabel();
        customPlot->replot();
    }
}

void SAWidget::updateMarkerLabel()
{
    QString labelText;
    QList<int> markerNumbers;

    // Collect active markers and their numbers
    for (auto it = markers.begin(); it != markers.end(); ++it) {
        Marker *marker = it.value();
        if (marker->active) {
            // Extract the marker number from the name (assuming "Marker X")
            int markerNumber = marker->name.split(' ').last().toInt();
            markerNumbers.append(markerNumber);
        }
    }

    // Sort the marker numbers
    std::sort(markerNumbers.begin(), markerNumbers.end());

    // Build the label text
    for (int number : markerNumbers) {
        QString markerName = QString("Marker %1").arg(number);
        Marker *marker = markers[markerName];

        int tIndex = marker->traceIndex;
        int index = marker->index;
        // Ensure tIndex is valid and the index is in range
        if (tIndex < 0 || tIndex >= MAX_TRACES || index < 0 || index >= traces[tIndex].freqs.size()) {
            // If out of range, skip this marker
            continue;
        }

        double freq = traces[tIndex].freqs[index];
        double amp = traces[tIndex].amps[index];

        if (marker->deltaMode) {
            double deltaFreq = freq - marker->deltaRefFrequency;
            double deltaAmp = amp - marker->deltaRefAmplitude;
            labelText += QString("%1 Delta: %2 Hz, %3 dB\n")
                             .arg(markerName)
                             .arg(deltaFreq, 0, 'f', 0)
                             .arg(deltaAmp, 0, 'f', 3);
        } else {
            labelText += QString("%1: %2 Hz, %3 dBm\n")
            .arg(markerName)
                .arg(freq, 0, 'f', 0)
                .arg(amp, 0, 'f', 3);
        }
    }

    // Update the label
    if (!labelText.isEmpty()) {
        markerLabel->setText(labelText.trimmed());  // Remove the last newline
        markerLabel->adjustSize();
        markerLabel->move(customPlot->width() - markerLabel->width() - 10, 10);
        markerLabel->setVisible(true);
    } else {
        markerLabel->setVisible(false);
    }
}

void SAWidget::onDeltaButtonClicked()
{
    Marker *marker = markers[currentMarkerName];
    if (!marker->active) {
        // Marker not active: revert button state
        bool wasBlocked = pushButton_Markers_Delta->blockSignals(true);
        pushButton_Markers_Delta->setChecked(false);
        pushButton_Markers_Delta->blockSignals(wasBlocked);
        return;
    }

    // If active, proceed as normal
    marker->toggleDeltaMode();
    updateMarker(marker);
    customPlot->replot();
}

void SAWidget::onValueOfSetFreqChanged(double frequency)
{
    Marker *marker = markers[currentMarkerName];
    int tIndex = marker->traceIndex;
    auto &freqs = traces[tIndex].freqs;
    if (freqs.isEmpty()) return;

    // Find the index in frequencies closest to the desired frequency
    auto it = std::lower_bound(freqs.begin(), freqs.end(), frequency);
    int index = std::distance(freqs.begin(), it);
    if (index >= freqs.size()) index = freqs.size() - 1;

    marker->index = index;
    marker->active = true;
    bool wasBlocked = checkBox_Markers_Active->blockSignals(true);
    checkBox_Markers_Active->setChecked(true);
    checkBox_Markers_Active->blockSignals(wasBlocked);
    updateMarker(marker);
    customPlot->replot();
}

void SAWidget::onPkTrackingStateChanged(int state)
{
    Marker *marker = markers[currentMarkerName];
    marker->pkTracking = (state == Qt::Checked);

    if (marker->pkTracking && !marker->active) {
        // Activate the marker
        marker->active = true;
        bool wasBlocked = checkBox_Markers_Active->blockSignals(true);
        checkBox_Markers_Active->setChecked(true);
        checkBox_Markers_Active->blockSignals(wasBlocked);
        updateMarker(marker);
        customPlot->replot();
    }
}

void SAWidget::onExportTraceData()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export Trace Data", QString(), "CSV Files (*.csv);;All Files (*)");

    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not open the file for writing.");
        return;
    }

    QTextStream out(&file);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out.setRealNumberPrecision(9);

    out << "Frequency (Hz),Amplitude (dBm)\n";

    const auto &freqs = traces[currentTraceIndex].freqs;
    const auto &ampsToExport = traces[currentTraceIndex].amps;

    if (!freqs.isEmpty() && !ampsToExport.isEmpty()) {
        for (int i = 0; i < freqs.size(); ++i) {
            double freq = freqs[i];
            double amp = ampsToExport[i];
            out << freq << "," << amp << "\n";
        }
    } else {
        // If no data is available, just warn the user
        QMessageBox::information(this, "No Data", "No data is available for the selected trace.");
    }

    file.close();

    QMessageBox::information(this, "Export Complete", "The trace data has been exported successfully.");
}

void SAWidget::onTraceUpdateStateChanged(int state)
{
    traces[currentTraceIndex].updateEnabled = (state == Qt::Checked);
}

void SAWidget::onTraceHideStateChanged(int state)
{
    traces[currentTraceIndex].hide = (state == Qt::Checked);
    if (traces[currentTraceIndex].type == TraceType::Off)
        return;

    // Always set the visibility for the main line graph
    customPlot->graph(currentTraceIndex)->setVisible(!traces[currentTraceIndex].hide);

    // If it's MinMaxHold, also set the visibility for the min line graph
    if (traces[currentTraceIndex].type == MinMaxHold) {
        customPlot->graph(currentTraceIndex + MAX_TRACES)->setVisible(!traces[currentTraceIndex].hide);
    }

    customPlot->replot();
}

void SAWidget::onPlotColorChanged(const QColor &color)
{
    traces[currentTraceIndex].color = color;
    customPlot->graph(currentTraceIndex)->setPen(QPen(color));
    QColor minColor = color;
    customPlot->graph(currentTraceIndex + MAX_TRACES)->setPen(QPen(minColor));
    customPlot->replot();
}

void SAWidget::onCurrentTraceChanged(int index)
{
    currentTraceIndex = index;

    // Update the UI elements to reflect the currently selected trace
    bool wasBlocked = checkBox_Traces_Update->blockSignals(true);
    checkBox_Traces_Update->setChecked(traces[currentTraceIndex].updateEnabled);
    checkBox_Traces_Update->blockSignals(wasBlocked);

    wasBlocked = checkBox_Traces_Hide->blockSignals(true);
    checkBox_Traces_Hide->setChecked(traces[currentTraceIndex].hide);
    checkBox_Traces_Hide->blockSignals(wasBlocked);

    wasBlocked = comboBox_Traces_Type->blockSignals(true);
    comboBox_Traces_Type->setCurrentIndex(static_cast<int>(traces[currentTraceIndex].type));
    comboBox_Traces_Type->blockSignals(wasBlocked);

    wasBlocked = colorPicker->blockSignals(true);
    colorPicker->setColor(traces[currentTraceIndex].color);
    colorPicker->blockSignals(wasBlocked);

    wasBlocked = spinBox_Traces_AvgCount->blockSignals(true);
    spinBox_Traces_AvgCount->setValue(traces[currentTraceIndex].avgCount);
    spinBox_Traces_AvgCount->blockSignals(wasBlocked);

    if (traces[currentTraceIndex].type == Average) {
        int currentAverageCount = traces[currentTraceIndex].lastSweeps.size();
        label_CurrAvg->setText(QString("CurrAvg: %1").arg(currentAverageCount));
    } else {
        label_CurrAvg->setText("CurrAvg: N/A");
    }
}

void SAWidget::onMarkerPlaceOnChanged(int newTraceIndex)
{
    Marker *marker = markers[currentMarkerName];
    int oldIndex = marker->index;

    // Update marker traceIndex to new trace
    marker->traceIndex = newTraceIndex;

    // Ensure index is in range for the new trace
    // If new trace has fewer data points, clamp index
    if (oldIndex >= traces[newTraceIndex].freqs.size()) {
        marker->index = traces[newTraceIndex].freqs.size() - 1;
        if (marker->index < 0) marker->index = 0;
    }

    updateMarker(marker);
    customPlot->replot();
}

void SAWidget::onTraceTypeChanged(int index)
{
    traces[currentTraceIndex].type = static_cast<TraceType>(index);

    // Clear arrays to ensure consistency
    traces[currentTraceIndex].freqs.clear();
    traces[currentTraceIndex].minAmps.clear();
    traces[currentTraceIndex].amps.clear();

    // If switching to Average, initialize sumAmps and lastSweeps
    if (traces[currentTraceIndex].type == Average) {
        traces[currentTraceIndex].sumAmps.clear();
        traces[currentTraceIndex].lastSweeps.clear();
    }

    if (traces[currentTraceIndex].type == Average) {
        label_CurrAvg->setText("CurrAvg: 0");
    } else {
        label_CurrAvg->setText("CurrAvg: N/A");
    }

    updatePlot();
}

void SAWidget::onTraceClearClicked()
{
    int i = currentTraceIndex;
    traces[i].freqs.clear();
    traces[i].amps.clear();
    traces[i].minAmps.clear();

    if (traces[i].type == Average) {
        traces[i].sumAmps.clear();
        traces[i].lastSweeps.clear();
    }

    // Update the graph to show no data
    customPlot->graph(currentTraceIndex)->setData(QVector<double>(), QVector<double>());
    customPlot->graph(i + MAX_TRACES)->setData(QVector<double>(), QVector<double>());

    // If any markers are on this trace, deactivate them since no data is available
    for (auto &marker : markers) {
        if (marker->traceIndex == i) {
            marker->active = false;
            marker->setVisible(false);
        }
    }

    updateMarkerLabel();

    // Update CurrAvg label
    if (traces[i].type == Average) {
        // After clearing, no sweeps are averaged yet
        label_CurrAvg->setText("CurrAvg: 0");
    } else {
        label_CurrAvg->setText("CurrAvg: N/A");
    }

    customPlot->replot();
}

void SAWidget::onAvgCountChanged(int avgCount)
{
    traces[currentTraceIndex].avgCount = avgCount;
    if (traces[currentTraceIndex].type == Average) {
        traces[currentTraceIndex].freqs.clear();
        traces[currentTraceIndex].minAmps.clear();
        traces[currentTraceIndex].amps.clear();
        traces[currentTraceIndex].sumAmps.clear();
        traces[currentTraceIndex].lastSweeps.clear();
        label_CurrAvg->setText("CurrAvg: 0");
        updatePlot();
    }
}

void SAWidget::onCopyToIndexChanged(int index)
{
    if (index == 0) return;

    int destIndex = index - 1;
    // If Trace 1 is selected at index 1, destIndex = 0

    // Validate destination
    if (destIndex < 0 || destIndex >= MAX_TRACES) {
        revertCopyToComboBox();
        return;
    }

    int srcIndex = currentTraceIndex;
    if (srcIndex == destIndex) {
        revertCopyToComboBox();
        return;
    }

    // Perform the copy
    copyTraceData(srcIndex, destIndex);

    // Revert combo box to "--"
    revertCopyToComboBox();

    customPlot->graph(destIndex)->setData(traces[destIndex].freqs, traces[destIndex].amps);
    customPlot->graph(destIndex)->setVisible(!traces[destIndex].hide);

    if (traces[destIndex].type == MinMaxHold) {
        customPlot->graph(destIndex + MAX_TRACES)->setData(traces[destIndex].freqs, traces[destIndex].minAmps);
        customPlot->graph(destIndex + MAX_TRACES)->setVisible(!traces[destIndex].hide);
        QColor fillColor = traces[destIndex].color;
        fillColor.setAlpha(50);
        customPlot->graph(destIndex)->setBrush(fillColor);
        customPlot->graph(destIndex)->setChannelFillGraph(customPlot->graph(destIndex + MAX_TRACES));
    } else {
        customPlot->graph(destIndex + MAX_TRACES)->setVisible(false);
        customPlot->graph(destIndex)->setBrush(Qt::NoBrush);
        customPlot->graph(destIndex)->setChannelFillGraph(nullptr);
    }

    // Update markers
    for (auto &marker : markers) {
        if (marker->active) {
            int tIndex = marker->traceIndex;
            if (marker->pkTracking && traces[tIndex].updateEnabled && traces[tIndex].type != Off) {
                // For pkTracking, we choose a line to find the max from
                const auto &ampsToCheck = traces[tIndex].amps;
                if (!ampsToCheck.isEmpty()) {
                    auto maxIt = std::max_element(ampsToCheck.begin(), ampsToCheck.end());
                    marker->index = std::distance(ampsToCheck.begin(), maxIt);
                }
            }
            updateMarker(marker);
        }
    }

    updateMarkerLabel();

    customPlot->replot();
}

void SAWidget::revertCopyToComboBox()
{
    bool wasBlocked = comboBox_Traces_CopyTo->blockSignals(true);
    comboBox_Traces_CopyTo->setCurrentIndex(0); // revert to "--"
    comboBox_Traces_CopyTo->blockSignals(wasBlocked);
}

void SAWidget::copyTraceData(int srcIndex, int destIndex)
{
    if (srcIndex < 0 || srcIndex >= MAX_TRACES || destIndex < 0 || destIndex >= MAX_TRACES) {
        return; // Invalid indices
    }

    TraceData &src = traces[srcIndex];
    TraceData &dst = traces[destIndex];

    // Overwrite data
    dst.freqs = src.freqs;
    dst.amps = src.amps;
    dst.minAmps = src.minAmps;
    dst.sumAmps = src.sumAmps;
    dst.lastSweeps = src.lastSweeps;

    // If destination was Off, set it to ClearWrite
    if (dst.type == Off) {
        dst.type = ClearWrite;
    }

    // Set destination to update=off and display=on
    dst.updateEnabled = false;
    dst.hide = false;
}

void SAWidget::onMinPeakClicked()
{
    Marker *marker = markers[currentMarkerName];
    int tIndex = marker->traceIndex;
    if (tIndex < 0 || tIndex >= MAX_TRACES) return;

    const auto &freqs = traces[tIndex].freqs;
    const auto &amps = traces[tIndex].amps;

    if (freqs.isEmpty() || amps.isEmpty()) return;

    // Find the index of the minimum amplitude
    auto minIt = std::min_element(amps.begin(), amps.end());
    int index = std::distance(amps.begin(), minIt);

    // Activate the marker if not active
    marker->index = index;
    marker->active = true;
    bool wasBlocked = checkBox_Markers_Active->blockSignals(true);
    checkBox_Markers_Active->setChecked(true);
    checkBox_Markers_Active->blockSignals(wasBlocked);

    updateMarker(marker);
    customPlot->replot();
}

void SAWidget::computeMeanAndStdev(const QVector<double> &amps, double &mean, double &stdev)
{
    if (amps.isEmpty()) {
        mean = 0.0;
        stdev = 0.0;
        return;
    }

    double sum = 0.0;
    for (double a : amps) sum += a;
    mean = sum / amps.size();

    double sqSum = 0.0;
    for (double a : amps) {
        double diff = a - mean;
        sqSum += diff * diff;
    }
    stdev = std::sqrt(sqSum / amps.size());
}

QVector<SAWidget::PeakInfo> SAWidget::findAllPeaksWithPlateaus(const QVector<double> &amps)
{
    QVector<PeakInfo> peaks;
    if (amps.size() < 2) {
        // Not enough data for any kind of peak
        return peaks;
    }

    // Compute mean & std dev
    double mean, stdevVal;
    computeMeanAndStdev(amps, mean, stdevVal);

    // Dynamic threshold (1 std dev above mean) vs. user pkThreshold
    double dynamicThreshold = std::max(mean + stdevVal, pkThreshold);

    // Iterate through amps, looking for plateaus that stand above dynamicThreshold
    int i = 0;
    while (i < amps.size()) {
        // If below threshold, move on
        if (amps[i] < dynamicThreshold) {
            i++;
            continue;
        }

        // Found a value >= dynamicThreshold, see if it forms a plateau
        int start = i;
        int end = i;
        double val = amps[i];

        // Extend plateau while values remain equal to val and within array
        while (end + 1 < amps.size() && qFuzzyCompare(1.0 + amps[end + 1], 1.0 + val)) {
            end++;
        }

        bool leftLower = false, rightLower = false;

        if (start == 0) {
            leftLower = true;
        } else {
            leftLower = (amps[start - 1] < val);
        }

        if (end == amps.size() - 1) {
            rightLower = true;
        } else {
            rightLower = (amps[end + 1] < val);
        }

        // If both leftLower and rightLower, we treat this plateau as a peak
        if (leftLower && rightLower) {
            // pick a single index to represent the plateau
            int plateauMid = (start + end) / 2;
            if (checkExcursion(amps, plateauMid, pkExcurs)) {
                PeakInfo pi;
                pi.index = plateauMid;
                pi.amplitude = val;
                peaks.append(pi);
            }
        }

        i = end + 1;
    }

    return peaks;
}

bool SAWidget::checkExcursion(const QVector<double> &amps, int peakIndex, double pkExcurs)
{
    if (peakIndex < 0 || peakIndex >= amps.size()) return false;

    double peakVal = amps[peakIndex];

    double leftMinVal = peakVal;
    for (int k = peakIndex - 1; k >= 0; --k) {
        if (amps[k] < leftMinVal) leftMinVal = amps[k];
        if ((peakVal - leftMinVal) >= pkExcurs) break;
    }
    if ((peakVal - leftMinVal) < pkExcurs) return false;

    double rightMinVal = peakVal;
    for (int k = peakIndex + 1; k < amps.size(); ++k) {
        if (amps[k] < rightMinVal) rightMinVal = amps[k];
        if ((peakVal - rightMinVal) >= pkExcurs) break;
    }
    if ((peakVal - rightMinVal) < pkExcurs) return false;

    return true;
}

void SAWidget::onMarkerToCenterClicked()
{
    // 1. Get the current marker
    Marker *marker = markers[currentMarkerName];
    if (!marker->active) {
        return;
    }

    int tIndex = marker->traceIndex;
    if (tIndex < 0 || tIndex >= MAX_TRACES) return;
    if (marker->index < 0 || marker->index >= traces[tIndex].freqs.size()) return;

    // 2. Get that marker's frequency
    double markerFreq = traces[tIndex].freqs[marker->index];

    // 3. Update the center frequency spin box
    //    This will automatically trigger updateFrequencyRange()
    spinBox_Frequency_Center->setFrequency(markerFreq);
}

void SAWidget::onMarkerToRefLevelClicked()
{
    // 1. Get the current marker
    Marker *marker = markers[currentMarkerName];
    if (!marker->active) {
        return;
    }
    int tIndex = marker->traceIndex;
    if (tIndex < 0 || tIndex >= MAX_TRACES) return;
    if (marker->index < 0 || marker->index >= traces[tIndex].amps.size()) return;

    // 2. Get that marker's amplitude
    double markerAmp = traces[tIndex].amps[marker->index];

    // 3. Update the reference level
    //    This triggers onRefLevelChanged(), which re-scales the Y-axis
    spinBox_Amplitude_RefLevel->setValue(markerAmp);
}

void SAWidget::onPeakLeftClicked()
{
    Marker *marker = markers[currentMarkerName];
    if (!marker->active) return;

    int tIndex = marker->traceIndex;
    if (tIndex < 0 || tIndex >= MAX_TRACES) return;

    const auto &amps = traces[tIndex].amps;
    if (amps.isEmpty()) return;

    // Get all peaks (sorted ascending by index)
    QVector<PeakInfo> allPeaks = findAllPeaksWithPlateaus(amps);
    if (allPeaks.isEmpty()) {
        logger::log(browser_SA, QStringLiteral("[PeakLeft] no peaks above threshold."));
        return;
    }

    int currentIdx = marker->index;
    int bestIndex = -1;

    for (auto &p : allPeaks) {
        if (p.index >= currentIdx) {
            break;
        }
        bestIndex = p.index;
    }

    if (bestIndex < 0) {
        // No left peak found
        return;
    }

    // Update marker position
    marker->index = bestIndex;
    updateMarker(marker);
    customPlot->replot();
}

void SAWidget::onPeakRightClicked()
{
    Marker *marker = markers[currentMarkerName];
    if (!marker->active) return;

    int tIndex = marker->traceIndex;
    if (tIndex < 0 || tIndex >= MAX_TRACES) return;

    const auto &amps = traces[tIndex].amps;
    if (amps.isEmpty()) return;

    // Get all peaks (sorted ascending by index)
    QVector<PeakInfo> allPeaks = findAllPeaksWithPlateaus(amps);
    if (allPeaks.isEmpty()) {
        logger::log(browser_SA, QStringLiteral("[PeakRight] no peaks above threshold."));
        return;
    }

    int currentIdx = marker->index;

    for (auto &p : allPeaks) {
        if (p.index > currentIdx) {
            marker->index = p.index;
            updateMarker(marker);
            customPlot->replot();
            return;
        }
    }
}

void SAWidget::onNextPeakClicked()
{
    Marker *marker = markers[currentMarkerName];
    if (!marker->active) return;

    int tIndex = marker->traceIndex;
    if (tIndex < 0 || tIndex >= MAX_TRACES) return;
    const auto &amps = traces[tIndex].amps;
    if (amps.isEmpty()) return;

    QVector<PeakInfo> allPeaks = findAllPeaksWithPlateaus(amps);
    if (allPeaks.isEmpty()) {
        logger::log(browser_SA, QStringLiteral("[NextPeak] no peaks above threshold."));
        return;
    }

    // Sort peaks by amplitude descending
    std::sort(allPeaks.begin(), allPeaks.end(),
              [](const PeakInfo &a, const PeakInfo &b){
                  return a.amplitude > b.amplitude;
              });

    // Find current peak in the list (or best match)
    // If marker->index is not exactly in allPeaks, find the nearest
    int currentIdx = marker->index;
    double currentAmp = amps[currentIdx];

    // Find the peak in allPeaks that matches marker->index (or best guess)
    int posInList = -1;
    for (int i=0; i<allPeaks.size(); ++i) {
        if (allPeaks[i].index == currentIdx) {
            posInList = i;
            break;
        }
    }
    // if not found exactly, find the peak with amplitude closest to currentAmp

    if (posInList < 0) {
        double minDiff = 1e15;
        for (int i=0; i<allPeaks.size(); ++i) {
            double diff = std::fabs(allPeaks[i].amplitude - currentAmp);
            if (diff < minDiff) {
                minDiff = diff;
                posInList = i;
            }
        }
    }

    if (posInList < 0) return; // can't identify current peak

    int nextPos = posInList + 1;
    if (nextPos >= allPeaks.size()) {
        return;
    }

    marker->index = allPeaks[nextPos].index;
    updateMarker(marker);
    customPlot->replot();
}

void SAWidget::onRefLevelChanged(double newRefLevel) {
    // Update the reference level and adjust the y-axis range
    double div = spinBox_Amplitude_Div->value(); // Get current Div value
    double rangeTop = newRefLevel; // Set the top of the y-axis based on the reference level

    // Set the y-axis range considering Div (each div represents a certain number of dB)
    double rangeBottom = rangeTop - 10 * div; // Calculate the bottom of the range (10 divisions below the ref level)

    customPlot->yAxis->setRange(rangeBottom, rangeTop); // Set y-axis range
    customPlot->replot(); // Replot the graph to update the view
}

void SAWidget::onDivChanged(double newDiv) {
    // Make Ref Level spin box step match the Div value
    spinBox_Amplitude_RefLevel->setSingleStep(newDiv);

    // Update Div and adjust the y-axis range
    double refLevel = spinBox_Amplitude_RefLevel->value(); // Get current Ref Level value
    double rangeTop = refLevel; // Top of the y-axis remains the same

    // Calculate the bottom of the y-axis based on Div (10 divisions below the ref level)
    double rangeBottom = rangeTop - 10 * newDiv;

    customPlot->yAxis->setRange(rangeBottom, rangeTop); // Set the new y-axis range
    customPlot->replot(); // Replot the graph to update the view
}

void SAWidget::onFreqPeakMeasureClicked()
{
    if (!hardwareTool || !hardwareTool->spectrumAnalyzer()) {
        logger::log(browser_SA, QStringLiteral("Spectrum Analyzer is null!"));
        return;
    }

    int freqKHz  = static_cast<int>(spinBox_Frequency_Center->frequency() / 1000.0);
    double level = spinBox_Level->value();
    int receive = spinBox_receiveChannel->value();
    QString chan = comboBox_Channel->currentText();

    logger::log(browser_SA, QString("[Spectrum] Starting peak measure with freq=%1, level=%2, receive=%3, chan=%4")
                           .arg(freqKHz).arg(level).arg(receive).arg(chan));

    QVector<double> outAmps = hardwareTool->spectrumAnalyzer()->measurePeakData(freqKHz, level, receive, chan);
    logger::log(browser_SA, QStringLiteral("[Spectrum] measurePeakData() returned."));
    int dataSize = outAmps.size();
    if(dataSize == 0)
    {
        logger::log(browser_SA, QStringLiteral("Spectrum measure returned empty."));
        return;
    }
    logger::log(browser_SA, QString("[Spectrum] Peak value=%1").arg(QString::number(outAmps.first(), 'f', 6)));
}

void SAWidget::setTool(Rfmu2Tool *tool)
{
    hardwareTool = tool;
}
