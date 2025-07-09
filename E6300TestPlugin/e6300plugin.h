#ifndef E6300PLUGIN_H
#define E6300PLUGIN_H

#include "E6300Plugin_Interface.h"
#include <QObject>
#include <QMainWindow>
#include <QJsonObject>
#include "sgwidget.h"
#include "sawidget.h"

class E6300Plugin : public QObject, public E6300Plugin_Interface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID E6300Plugin_InterfaceIID FILE "E6300Plugin.json")
    Q_INTERFACES(E6300Plugin_Interface)

public:
    E6300Plugin();
    ~E6300Plugin() override;

    // Implementation of E6300Plugin_Interface methods
    QString getPluginID() override;
    QString getPluginDynamicLicense() override;
    QJsonObject getPluginInfo() override;
    bool checkPluginCompatibility(const QString envPath) override;
    QStringList getPluginDependencyList() override;
    QMainWindow* createAndGetPluginWin(const QString paraNIC, int slot, QString& result) override;
    bool setLogCallback(E6300PluginLogCallback callback) override;
    E6300InstrumentStat getInstrumentCurrentStatus() override;
    QJsonObject getInstrumentInfo() override;

private:
    E6300PluginLogCallback m_logCallback = nullptr;
    QMainWindow* m_mainWindow = nullptr;
};

#endif // E6300PLUGIN_H
