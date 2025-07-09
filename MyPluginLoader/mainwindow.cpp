#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "E6300Plugin_Interface.h"
#include <QPluginLoader>
#include <QDebug>
#include <QVBoxLayout>
#include <QTimer>
#include <QCoreApplication>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , statusLabel(new QLabel(this))  // Add QLabel for status messages
{
    ui->setupUi(this);

    QVBoxLayout* verticalLayout = new QVBoxLayout();
    centralWidget()->setLayout(verticalLayout);

    // Add QLabel to display status messages
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 14px; color: red;");  // Set font style
    verticalLayout->addWidget(statusLabel);

    // Determine plugin path
    QString pluginPath = QCoreApplication::applicationDirPath() + "/plugins/E6300Plugin.dll";
    qDebug() << "Plugin Path:" << pluginPath;

    // Update UI with plugin path information
    statusLabel->setText("Trying to load plugin from: " + pluginPath);

    QPluginLoader loader(pluginPath);
    QObject *pluginInstance = loader.instance();

    if (!pluginInstance) {
        QString errorMsg = "Failed to load plugin:\n" + loader.errorString();
        qWarning() << errorMsg;
        statusLabel->setText(errorMsg);
        return;
    }

    // Attempt to cast the loaded plugin to the E6300Plugin_Interface
    E6300Plugin_Interface* plugin = qobject_cast<E6300Plugin_Interface*>(pluginInstance);
    if (plugin) {
        QString pluginID = plugin->getPluginID();
        qDebug() << "Plugin ID:" << pluginID;
        statusLabel->setText("Plugin loaded successfully!\nID: " + pluginID);

        // Try to create the plugin window
        QString result;
        QMainWindow* pluginWindow = plugin->createAndGetPluginWin("NIC_Param", 1, result);
        if (pluginWindow) {
            pluginWindow->show();
            qDebug() << "Plugin window created successfully: " << result;
            statusLabel->setText(statusLabel->text() + "\nPlugin window launched.");

            // Hide MainWindow after 1 second
            QTimer::singleShot(1000, this, [this]() {
                this->hide();
            });
        } else {
            qWarning() << "Failed to create plugin window: " << result;
            statusLabel->setText(statusLabel->text() + "\nFailed to create plugin window.\n" + result);
        }
    } else {
        QString errorMsg = "Plugin does not implement E6300Plugin_Interface.";
        qWarning() << errorMsg;
        statusLabel->setText(errorMsg);
        return;
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
