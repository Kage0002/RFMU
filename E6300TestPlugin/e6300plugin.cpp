#include "e6300plugin.h"
#include "mainwindow.h"

#include <QVBoxLayout>
#include <QCoreApplication>

static char UNIQUE_HEX_CHAR[256] = "yP8)T2KJ|Ge%@}1U~jvb?m5!ow^QSi4`[dF=czYp,q>t\"H\\7NrkEBM9LCnZV*#;R&AL(s_3a:I+W/X$fX0M[>&LBA_N\"3kh'c58VzqlVNO0-W:9T=mx-#iYub.Xc7H1d#GOp3;ymDQnuCV=G&;Rjl6'1F24B9g5[PQhI7sw%zA)vW,xDKM^S0Eqrt*neEroaRUZM-i]1[J)rTGp&gA\"#>DkWxs9tHV,lY7N3_z/L8mCQ+57jf]hbI{v<&0y;4XZ";

#define DPSU_DL_FACTOR0	1
#define DPSU_DL_FACTOR1	2
#define DPSU_DL_FACTOR2	3
#define DPSU_DL_FACTOR3	5
#define DPSU_DL_FACTOR4	8

#define CustomWindowUsed 0

E6300Plugin::E6300Plugin()
    : m_mainWindow(nullptr)
{
}

E6300Plugin::~E6300Plugin()
{
    if (m_mainWindow) {
        if (m_mainWindow->isVisible()) {
            m_mainWindow->close();
        }
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }
}

QString E6300Plugin::getPluginID()
{
    return E6300Plugin_InterfaceIID;
}

QString E6300Plugin::getPluginDynamicLicense()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    int t_year = currentDateTime.date().year();
    int t_month = currentDateTime.date().month();
    int t_day = currentDateTime.date().day();
    int t_hour = currentDateTime.time().hour();
    int t_dayOfWeek = currentDateTime.date().dayOfWeek();

    QString genLicense = "MJDZ-";
    genLicense.append(UNIQUE_HEX_CHAR[uint8_t(t_year * DPSU_DL_FACTOR0 & 0xff)]);
    genLicense.append(UNIQUE_HEX_CHAR[uint8_t(t_month * DPSU_DL_FACTOR1 & 0xff)]);
    genLicense.append(UNIQUE_HEX_CHAR[uint8_t(t_day * DPSU_DL_FACTOR2 & 0xff)]);
    genLicense.append(UNIQUE_HEX_CHAR[uint8_t(t_hour * DPSU_DL_FACTOR3 & 0xff)]);
    genLicense.append(UNIQUE_HEX_CHAR[uint8_t(t_dayOfWeek *DPSU_DL_FACTOR4 & 0xff)]);

    return genLicense;
}

QJsonObject E6300Plugin::getPluginInfo()
{
    QJsonObject info;
    info["Name"] = "E6300 Instrument Plugin";
    info["Description"] = "Implementation of E6300Plugin_Interface";
    info["PluginVersion"] = "1.0.0";
    info["ProtocolVersion"] = "1.0.0";
    info["UpdateTime"] = "2024/10/30";
    return info;
}

bool E6300Plugin::checkPluginCompatibility(const QString envPath)
{
    Q_UNUSED(envPath);
    return true;
}

QStringList E6300Plugin::getPluginDependencyList()
{
    return QStringList();
}

QMainWindow* E6300Plugin::createAndGetPluginWin(const QString paraNIC, int slot, QString& result)
{
#if CustomWindowUsed
    Q_UNUSED(paraNIC);
    Q_UNUSED(slot);
    // Check if m_mainWindow is already created
    if (!m_mainWindow) {
        // Create the main window for the plugin
        m_mainWindow = new PluginTest_MainWindow(paraNIC, slot);
        result = "Plugin window created successfully.";
    } else {
        result = "Plugin window already exists.";
    }

    return m_mainWindow;
#else
    Q_UNUSED(paraNIC);
    Q_UNUSED(slot);

    if (!m_mainWindow) {
        // m_mainWindow = new QMainWindow();
        // QWidget *myCentralWidget = new QWidget();
        // QVBoxLayout *layout = new QVBoxLayout();

        // SGWidget *mysgwidget = new SGWidget();
        // mysgwidget->setAttribute(Qt::WA_StyledBackground, true);
        // mysgwidget->setStyleSheet("background-color: #EFEFF2; font-size: 12px; font-family: Arial;");
        // layout->addWidget(mysgwidget);

        // SAWidget *mysawidget = new SAWidget();
        // mysawidget->setAttribute(Qt::WA_StyledBackground, true);
        // mysawidget->setStyleSheet("background-color: #EFEFF2; font-size: 12px; font-family: Arial;");
        // layout->addWidget(mysawidget);

        // myCentralWidget->setLayout(layout);
        // m_mainWindow->setCentralWidget(myCentralWidget);

        QString modeStr = qEnvironmentVariable("RFMU_FEATURE_MODE");
        if (modeStr.isEmpty()) {
#if defined(DEFAULT_RFMU_FEATURE_MODE_SG)
            modeStr = QStringLiteral("SG");
#elif defined(DEFAULT_RFMU_FEATURE_MODE_SA)
            modeStr = QStringLiteral("SA");
#elif defined(DEFAULT_RFMU_FEATURE_MODE_NA)
            modeStr = QStringLiteral("NA");
#else
            modeStr = QStringLiteral("ALL");
#endif
        }
        MainWindow::DisplayMode mode = MainWindow::DisplayMode::ShowAll;
        if (modeStr.compare("SG", Qt::CaseInsensitive) == 0) {
            mode = MainWindow::DisplayMode::SignalSourceOnly;
        } else if (modeStr.compare("SA", Qt::CaseInsensitive) == 0) {
            mode = MainWindow::DisplayMode::SpectrumAnalyzerOnly;
        } else if (modeStr.compare("NA", Qt::CaseInsensitive) == 0) {
            mode = MainWindow::DisplayMode::NetworkAnalyzerOnly;
        }

        m_mainWindow = new MainWindow(mode);

        result = "Plugin window created successfully.";
    } else {
        result = "Plugin window already exists.";
    }

    return m_mainWindow;
#endif
}

bool E6300Plugin::setLogCallback(E6300PluginLogCallback callback)
{
#if CustomWindowUsed
    if(!m_mainWindow)
        return false;

    PluginTest_MainWindow* mainWindow = qobject_cast<PluginTest_MainWindow*>(m_mainWindow);
    if (mainWindow) {
        mainWindow->setExtCallback(callback);
        return true;
    }

    return false;
#else
    m_logCallback = callback;
    if (m_logCallback) {
        m_logCallback("Log callback set successfully.");
        return true;
    }
    return false;
#endif
}

E6300InstrumentStat E6300Plugin::getInstrumentCurrentStatus()
{
#if CustomWindowUsed
    if(!m_mainWindow)
        return E6300InstrumentStat::Error;

    PluginTest_MainWindow* mainWin = qobject_cast<PluginTest_MainWindow*>(m_mainWindow);
    if (mainWin) {
        return mainWin->getStatus();
    }

    return E6300InstrumentStat::Error;
#else
    return E6300InstrumentStat::Actionable;
#endif
}

QJsonObject E6300Plugin::getInstrumentInfo()
{
#if CustomWindowUsed
    QJsonObject errInfo;

    if(!m_mainWindow)
    {
        errInfo["Error detail"] = "Plugin init error.";
        return errInfo;
    }

    PluginTest_MainWindow* mainWin = qobject_cast<PluginTest_MainWindow*>(m_mainWindow);
    if (mainWin) {
        return mainWin->getInfo();
    }

    errInfo["Error detail"] = "Plugin malfunction.";
    return errInfo;
#else
    QJsonObject info;
    info["Test info"] = "An instrument info.";
    return info;
#endif
}
