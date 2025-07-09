#pragma once
#include <QMetaType>

enum class Rfmu2Err {
    TcpWriteFail,
    Timeout,
    Protocol,
    DeviceNack,
    DataFormat,
    InternalLogic
};

struct Rfmu2Error {
    Rfmu2Err code {};
    QString  text;
};
Q_DECLARE_METATYPE(Rfmu2Error)
