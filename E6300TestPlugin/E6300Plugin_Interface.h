#ifndef E6300PLUGIN_INTERFACE_H
#define E6300PLUGIN_INTERFACE_H

#include <QMainWindow>
#include <QtPlugin>

#define E6300Plugin_InterfaceIID "MatrixSystemSoftwareInterface/E6300InstrumentMaster/V1.0.0"

#ifndef E6300PluginLogCallback
typedef void (*E6300PluginLogCallback)(const QString&);
#endif

enum class E6300InstrumentStat
{
    Disconnected,
    Actionable,
    Busy,
    Error
};


class E6300Plugin_Interface
{
public:
    /*************************************************************************/
    /******************************* Sys Default******************************/
    /*************************************************************************/
    virtual ~E6300Plugin_Interface() = default;

    /*************************************************************************/
    /*************************** API About Plugin ****************************/
    /*************************************************************************/
    // 获取插件的唯一标识符（后期用不同IID可用于区别不同的插件？）
    virtual QString getPluginID() = 0;

    // 获取插件的动态许可信息，以验证插件的合法性
    virtual QString getPluginDynamicLicense()= 0;

    // 获取插件的基本信息，名称、描述、版本等
    virtual QJsonObject getPluginInfo()= 0;

    // 检查插件是否可以被加载(如检查依赖性等)
    virtual bool checkPluginCompatibility(const QString envPath)= 0;

    // 获取插件的依赖项列表，用于加载插件时拷贝走插件的依赖项目
    virtual QStringList getPluginDependencyList()= 0;

    /*************************************************************************/
    /**************************** QMainWindow API ****************************/
    /*************************************************************************/
    // 创建并获取插件的主窗口, 输出参数 result     操作结果信息
    virtual QMainWindow* createAndGetPluginWin(const QString paraNIC, int slot, QString& result) = 0;


    /*************************************************************************/
    /************************* Instrument Funcstion **************************/
    /*************************************************************************/
    /*********************Need board/instrument stay connected****************/
    /*************************************************************************/
    // 设置日志打印回调函数，参数类型为QString
    virtual bool setLogCallback(E6300PluginLogCallback callback) = 0;


    // 检查设备是否正常连接
    virtual E6300InstrumentStat getInstrumentCurrentStatus()  = 0;

    // 获取连接到的设备的基本信息，名称、描述、版本等
    virtual QJsonObject getInstrumentInfo()  = 0;

};


// 声明接口，并关联到唯一的IID
Q_DECLARE_INTERFACE(E6300Plugin_Interface, E6300Plugin_InterfaceIID)



#endif // E6300PLUGIN_INTERFACE_H
