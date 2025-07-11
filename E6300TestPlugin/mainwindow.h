#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QTabWidget>
#include "include/rfmu2/rfmu2tool.h"
#include "include/rfmu2/rfmu2_error.h"

// Forward declarations
class Rfmu2Tool;
class SAWidget;
class SGWidget;
class NAWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum class DisplayMode {
        ShowAll,
        SignalSourceOnly,
        SpectrumAnalyzerOnly,
        NetworkAnalyzerOnly
    };

    explicit MainWindow(DisplayMode mode = DisplayMode::ShowAll,
                        QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // Menu actions
    void onConnectTriggered();
    void onDisconnectTriggered();
    void onQuitTriggered();
    void onAboutTriggered();
    void onReadVoltageTempTriggered();
    void onRRSUCalibrationTriggered();

    // Hardware signals
    void onHardwareError(const Rfmu2Error &error);
    void onConnectionStateChanged(bool connected);
    void onSinglePortCaliResult(const QString &msg);
    void onDualPortCaliResult(const QString &msg);

    void onSetTimeoutsTriggered();
    void onTabChanged(int index);

private:
    void createCentralTabs();
    void createDockWidgets();
    void createActions();
    void createMenus();
    void createStatusBar();
    void applyStyleSheet(); // optional for styling

private:
    // The hardware tool managing the TCP connection & submodules
    Rfmu2Tool *m_rfmuTool;

    // Our main widgets
    SAWidget  *m_saWidget;  // Spectrum Analyzer
    SGWidget  *m_sgWidget;  // Signal Generator (to be expanded)
    NAWidget  *m_naWidget;  // Network Analyzer (placeholder)

    // Central tab widget
    QTabWidget *m_tabWidget;

    // Menu & actions
    QMenu   *m_fileMenu;
    QMenu   *m_deviceMenu;
    QMenu   *m_viewMenu;
    QMenu   *m_helpMenu;
    QMenu   *m_refClockMenu;

    QAction *m_connectAction;
    QAction *m_disconnectAction;
    QAction *m_quitAction;
    QAction *m_aboutAction;
    QAction *m_setTimeoutsAction;
    QAction *m_useInternalClockAction;
    QAction *m_useExternalClockAction;
    QAction* m_readVoltageTempAction;
    QAction *m_rrsuCalibAction;

    QDockWidget *m_dockSG;

    DisplayMode m_displayMode;
};

#endif // MAINWINDOW_H
