#include "mainwindow.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QDockWidget>
#include <QTabWidget>
#include <QDebug>

#include "sawidget.h"
#include "sgwidget.h"
#include "nawidget.h"
#include "rrsucalibdialog.h"
#include "connectdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_rfmuTool(new Rfmu2Tool(this))
    , m_saWidget(nullptr)
    , m_sgWidget(nullptr)
    , m_naWidget(nullptr)
    , m_tabWidget(nullptr)
{
    // 1. Connect Rfmu2Tool signals to this MainWindow
    connect(m_rfmuTool, &Rfmu2Tool::errorOccurred,
            this, &MainWindow::onHardwareError);
    connect(m_rfmuTool, &Rfmu2Tool::connectionStateChanged,
            this, &MainWindow::onConnectionStateChanged);

    // 2. Create main UI components
    createCentralTabs();    // Tab widget in the center
    createDockWidgets();    // SGWidget on the right side
    createActions();
    createMenus();
    createStatusBar();
    applyStyleSheet();      // optional styling
    applyVisibilitySettings(); // adjust widget visibility based on macro

    // 3. Window properties
    setWindowTitle(tr("RF Module Control"));
    resize(1400, 900);
}

MainWindow::~MainWindow()
{
}

//----------------------------------------
// Create the central tab widget
//----------------------------------------
void MainWindow::createCentralTabs()
{
    // Initialize the tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);
    m_tabWidget->setMovable(true);

    // Create SAWidget (Spectrum Analyzer)
    m_saWidget = new SAWidget(this);
    m_saWidget->setTool(m_rfmuTool);
    m_tabWidget->addTab(m_saWidget, tr("Spectrum Analyzer"));

    // Create NAWidget (Network Analyzer)
    m_naWidget = new NAWidget(this);
    m_naWidget->setTool(m_rfmuTool);
    m_tabWidget->addTab(m_naWidget, tr("Network Analyzer"));

    connect(m_naWidget, &NAWidget::naSinglePortCali,
            this, &MainWindow::onSinglePortCaliResult);
    connect(m_naWidget, &NAWidget::naDualPortCali,
            this, &MainWindow::onDualPortCaliResult);

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // Make the tab widget the central widget
    setCentralWidget(m_tabWidget);
}

//----------------------------------------
// Create Dock Widgets
//----------------------------------------
void MainWindow::createDockWidgets()
{
    // Signal Generator as a dock on the right
    m_sgWidget = new SGWidget(this);
    m_sgWidget->setTool(m_rfmuTool);

    m_dockSG = new QDockWidget(tr("Signal Generator"), this);
    m_dockSG->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_dockSG->setWidget(m_sgWidget);
    addDockWidget(Qt::RightDockWidgetArea, m_dockSG);

    // If you have more side widgets, create more docks similarly
}

//----------------------------------------
// Create menu actions
//----------------------------------------
void MainWindow::createActions()
{
    // Connect
    m_connectAction = new QAction(tr("&Connect"), this);
    m_connectAction->setStatusTip(tr("Connect to the hardware"));
    connect(m_connectAction, &QAction::triggered, this, &MainWindow::onConnectTriggered);

    // Disconnect
    m_disconnectAction = new QAction(tr("&Disconnect"), this);
    m_disconnectAction->setStatusTip(tr("Disconnect from the hardware"));
    connect(m_disconnectAction, &QAction::triggered, this, &MainWindow::onDisconnectTriggered);

    // Quit
    m_quitAction = new QAction(tr("&Quit"), this);
    m_quitAction->setShortcuts(QKeySequence::Quit);
    m_quitAction->setStatusTip(tr("Quit the application"));
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::onQuitTriggered);

    // About
    m_aboutAction = new QAction(tr("&About"), this);
    m_aboutAction->setStatusTip(tr("About this application"));
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::onAboutTriggered);

    // Timeout configuration
    m_setTimeoutsAction = new QAction(tr("Set Timeouts..."), this);
    m_setTimeoutsAction->setStatusTip(tr("Configure timeouts for SignalGen, Spectrum, Network"));
    connect(m_setTimeoutsAction, &QAction::triggered,
            this, &MainWindow::onSetTimeoutsTriggered);

    // Create "Use Internal Clock"
    m_useInternalClockAction = new QAction(tr("Use Internal Clock"), this);
    m_useInternalClockAction->setStatusTip(tr("Switch reference to internal clock"));
    connect(m_useInternalClockAction, &QAction::triggered, this, [this]() {
        if (!m_rfmuTool) {
            QMessageBox::warning(this, tr("Error"), tr("Rfmu2Tool is null!"));
            return;
        }
        // Access the systemControl instance
        auto sysCtrl = m_rfmuTool->systemControl();
        if (!sysCtrl) {
            QMessageBox::warning(this, tr("Error"), tr("SystemControl is null!"));
            return;
        }

        // Switch clock
        bool ok = sysCtrl->setReferenceClockMode(true /*useInternal*/);
        if (ok) {
            statusBar()->showMessage(tr("Clock reference set to INTERNAL"), 5000);
        } else {
            statusBar()->showMessage(tr("Failed to set clock reference"), 5000);
        }
    });

    // Create "Use External Clock"
    m_useExternalClockAction = new QAction(tr("Use External Clock"), this);
    m_useExternalClockAction->setStatusTip(tr("Switch reference to external clock"));
    connect(m_useExternalClockAction, &QAction::triggered, this, [this]() {
        if (!m_rfmuTool) {
            QMessageBox::warning(this, tr("Error"), tr("Rfmu2Tool is null!"));
            return;
        }
        auto sysCtrl = m_rfmuTool->systemControl();
        if (!sysCtrl) {
            QMessageBox::warning(this, tr("Error"), tr("SystemControl is null!"));
            return;
        }

        bool ok = sysCtrl->setReferenceClockMode(false /*useInternal*/);
        if (ok) {
            statusBar()->showMessage(tr("Clock reference set to EXTERNAL"), 5000);
        } else {
            statusBar()->showMessage(tr("Failed to set clock reference"), 5000);
        }
    });

    // Read voltages & temperature
    m_readVoltageTempAction = new QAction(tr("Read Voltage/Temp"), this);
    m_readVoltageTempAction->setStatusTip(tr("Read the machine's 8 voltage values and temperature"));
    connect(m_readVoltageTempAction, &QAction::triggered,
            this, &MainWindow::onReadVoltageTempTriggered);

    // RRSU TX Calibration
    m_rrsuCalibAction = new QAction(tr("RRSU TX Calibration"), this);
    m_rrsuCalibAction->setStatusTip(tr("Upload RRSU TX-calibration blob"));
    connect(m_rrsuCalibAction, &QAction::triggered,
            this, &MainWindow::onRRSUCalibrationTriggered);
}

//----------------------------------------
// Create the Menus
//----------------------------------------
void MainWindow::createMenus()
{
    // File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_quitAction);

    // Device menu
    m_deviceMenu = menuBar()->addMenu(tr("&Device"));
    m_deviceMenu->addAction(m_connectAction);
    m_deviceMenu->addAction(m_disconnectAction);
    m_deviceMenu->addSeparator();
    m_deviceMenu->addAction(m_setTimeoutsAction);

    m_deviceMenu->addSeparator();
    m_deviceMenu->addAction(m_readVoltageTempAction);

    // -- Reference Clock Submenu
    m_refClockMenu = new QMenu(tr("Reference Clock"), this);
    m_refClockMenu->addAction(m_useInternalClockAction);
    m_refClockMenu->addAction(m_useExternalClockAction);

    m_deviceMenu->addSeparator();
    m_deviceMenu->addAction(m_rrsuCalibAction);

    // Add the Reference Clock submenu to the device menu
    m_deviceMenu->addMenu(m_refClockMenu);

    // View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    viewSignalGeneratorAction = m_dockSG->toggleViewAction();
    viewSignalGeneratorAction->setText(tr("Signal Generator"));
    m_viewMenu->addAction(viewSignalGeneratorAction);

    // Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_aboutAction);
}

//----------------------------------------
// Create the Status Bar
//----------------------------------------
void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

//----------------------------------------
// Optional method for styling
//----------------------------------------
void MainWindow::applyStyleSheet()
{
    // Example: mild dark style, or any style you like
    // (Adjust colors, fonts, etc. to your preference)
    setStyleSheet(
        "QWidget { font: 10pt 'Segoe UI'; } "
        "QMainWindow { background-color: #F0F0F0; } "
        "QTabBar::tab { background: #C8C8C8; padding: 6px; } "
        "QTabBar::tab:selected { background: #606060; color: #FFFFFF; } "
        );
}

//----------------------------------------
// Slots for menu actions
//----------------------------------------
void MainWindow::onConnectTriggered()
{
    ConnectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return; // user cancelled

    const QString addr = dlg.ip();
    const quint16 prt = dlg.port();

    if (addr.isEmpty()) { // (mask guarantees format but not "empty")
        QMessageBox::warning(this, tr("Invalid IP"),
                             tr("Please enter a valid IP address."));
        return;
    }

    if (m_rfmuTool->connectToHost(addr, prt)) {
        statusBar()->showMessage(tr("Connected to %1:%2")
                                     .arg(addr).arg(prt), 5000);
    } else {
        statusBar()->showMessage(tr("Connection failed"), 5000);
    }
}

void MainWindow::onDisconnectTriggered()
{
    m_rfmuTool->disconnectFromHost();
    statusBar()->showMessage(tr("Disconnected"));
}

void MainWindow::onSetTimeoutsTriggered()
{
    if (!m_rfmuTool) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Rfmu2Tool is null, cannot apply timeouts."));
        return;
    }

    // 1) Prompt for Signal Generator Timeout
    bool ok = false;
    int genMs = QInputDialog::getInt(this,
                                     tr("Signal Generator Timeout"),
                                     tr("Timeout (ms):"),
                                     5000, // default
                                     1, 9999999, 1, &ok);
    if (!ok) {
        // User canceled
        return;
    }

    // 2) Prompt for Spectrum Analyzer Timeout
    int specMs = QInputDialog::getInt(this,
                                      tr("Spectrum Analyzer Timeout"),
                                      tr("Timeout (ms):"),
                                      5000, // default
                                      1, 9999999, 1, &ok);
    if (!ok) {
        return;
    }

    // 3) Prompt for Network Analyzer Timeout
    int naMs = QInputDialog::getInt(this,
                                    tr("Network Analyzer Timeout"),
                                    tr("Timeout (ms):"),
                                    20000, // default
                                    1, 9999999, 1, &ok);
    if (!ok) {
        return;
    }

    auto gen  = m_rfmuTool->signalGenerator();
    auto spec = m_rfmuTool->spectrumAnalyzer();
    auto na   = m_rfmuTool->networkAnalyzer();

    if (gen)  gen->setTimeoutMs(genMs);
    if (spec) spec->setTimeoutMs(specMs);
    if (na)   na->setTimeoutMs(naMs);

    // Show a small confirmation or log
    statusBar()->showMessage(
        tr("Timeouts set: SG=%1 ms, SA=%2 ms, NA=%3 ms")
            .arg(genMs).arg(specMs).arg(naMs),
        5000  // message duration
        );
}

void MainWindow::onQuitTriggered()
{
    close();
}

void MainWindow::onAboutTriggered()
{
    QMessageBox::about(this, tr("About RF Module Control"),
                       tr("<b>RF Module Control</b><br>"
                          "An application to control and visualize "
                          "signal generator, spectrum analyzer, and network analyzer."));
}

//----------------------------------------
// Slots for hardware signals
//----------------------------------------
void MainWindow::onHardwareError(const Rfmu2Error &err)
{
    QMessageBox::warning(this, tr("Hardware Error"), err.text);
    statusBar()->showMessage(tr("Hardware Error: ") + err.text);
}

void MainWindow::onConnectionStateChanged(bool connected)
{
    if (connected) {
        statusBar()->showMessage(tr("Hardware connected"));
    } else {
        statusBar()->showMessage(tr("Hardware disconnected"));
        m_saWidget->stopAutoSweep();
        m_naWidget->stopAutoSweep();
    }
}

void MainWindow::onSinglePortCaliResult(const QString &msg)
{
    // Display the message in a message box
    QMessageBox::information(this, tr("Calibration Result"), msg);

    // Show the message in the status bar
    statusBar()->showMessage(tr("Calibration result: ") + msg);
}

void MainWindow::onDualPortCaliResult(const QString &msg)
{
    // Display the message in a warning box
    QMessageBox::information(this, tr("Calibration Result"), msg);

    // Show the message in the status bar
    statusBar()->showMessage(tr("Calibration result: ") + msg);
}

void MainWindow::onReadVoltageTempTriggered()
{
    // 1) Check if we have a valid tool
    if(!m_rfmuTool) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Rfmu2Tool is null; cannot read voltage/temp."));
        return;
    }
    // 2) Grab the systemControl submodule
    auto sysCtrl = m_rfmuTool->systemControl();
    if(!sysCtrl) {
        QMessageBox::warning(this, tr("Error"),
                             tr("SystemControl is null; cannot read voltage/temp."));
        return;
    }

    // 3) Call the new function
    QVector<double> results = sysCtrl->readVoltagesAndTemperature();
    if (results.isEmpty()) {
        // Something went wrong, or user was informed via errorOccurred
        statusBar()->showMessage("Failed to read voltage/temp", 5000);
        return;
    }
    // results[0..7] => voltages, results[8] => temperature
    QString msg;
    msg += "Voltages:\n";
    for (int i=0; i<8; i++) {
        msg += QString(" V%1 = %2\n").arg(i+1).arg(results[i], 0, 'f', 3);
    }
    msg += QString("\nTemperature = %1(Celsius)").arg(results[8], 0, 'f', 3);

    // 4) Display to user
    QMessageBox::information(this, tr("Voltage/Temp"), msg);
    statusBar()->showMessage("Read voltage/temp successful", 5000);
}

void MainWindow::onTabChanged(int index)
{
    QWidget *current = m_tabWidget->widget(index);

    if (current == m_naWidget) {
        m_dockSG->hide();
        m_saWidget->stopAutoSweep();
    }
    else if (current == m_saWidget) {
        m_dockSG->show();
        m_naWidget->stopAutoSweep();
    }
    else {
        m_dockSG->show();
    }
}

void MainWindow::onRRSUCalibrationTriggered()
{
    if (!m_rfmuTool || !m_rfmuTool->systemControl()) {
        QMessageBox::warning(this, tr("Error"),
                             tr("SystemControl not available"));
        return;
    }
    RRSUCalibDialog dlg(m_rfmuTool->systemControl(), this);
    dlg.exec(); // modal
}

//----------------------------------------
// Apply visibility settings based on macro
//----------------------------------------
void MainWindow::applyVisibilitySettings()
{
#if WIDGET_VISIBILITY_MODE == DISPLAY_SG
    // Show only SG widget
    if (m_tabWidget) {
        m_tabWidget->hide();
    }
    if (m_saWidget)  m_saWidget->hide();
    if (m_naWidget)  m_naWidget->hide();
    if (m_dockSG)    m_dockSG->show();
#elif WIDGET_VISIBILITY_MODE == DISPLAY_SA
    if (m_tabWidget) {
        m_tabWidget->setCurrentWidget(m_saWidget);
        m_tabWidget->tabBar()->hide();
    }
    if (m_naWidget)  m_naWidget->hide();
    if (m_sgWidget)  m_sgWidget->hide();
    if (m_dockSG)    m_dockSG->hide();
    if (viewSignalGeneratorAction) viewSignalGeneratorAction->setVisible(false);
#elif WIDGET_VISIBILITY_MODE == DISPLAY_NA
    if (m_tabWidget) {
        m_tabWidget->setCurrentWidget(m_naWidget);
        m_tabWidget->tabBar()->hide();
    }
    if (m_saWidget)  m_saWidget->hide();
    if (m_sgWidget)  m_sgWidget->hide();
    if (m_dockSG)    m_dockSG->hide();
    if (viewSignalGeneratorAction) viewSignalGeneratorAction->setVisible(false);
#else // DISPLAY_ALL
    if (m_tabWidget) m_tabWidget->show();
    if (m_saWidget)  m_saWidget->show();
    if (m_naWidget)  m_naWidget->show();
    if (m_dockSG)    m_dockSG->show();
    if (viewSignalGeneratorAction) viewSignalGeneratorAction->setVisible(true);
#endif
}
