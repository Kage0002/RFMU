QT += gui widgets core printsupport network

TEMPLATE = lib
CONFIG += plugin

CONFIG += c++17

# Default mode if RFMU_FEATURE_MODE is not set at runtime.
# Define one of the following macros to pick a mode:
#  DEFAULT_RFMU_FEATURE_MODE_SG
#  DEFAULT_RFMU_FEATURE_MODE_SA
#  DEFAULT_RFMU_FEATURE_MODE_NA
#  DEFAULT_RFMU_FEATURE_MODE_ALL
DEFINES += DEFAULT_RFMU_FEATURE_MODE_ALL

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    E6300Plugin_Interface.h \
    collapsiblegroupbox.h \
    colorpickerwidget.h \
    connectdialog.h \
    e6300plugin.h \
    include/frequencyspinbox.h \
    include/qcustomplot.h \
    include/rfmu2/rfmu2_error.h \
    include/rfmu2/rfmu2base.h \
    include/rfmu2/rfmu2networkanalyzer.h \
    include/rfmu2/rfmu2signalgenerator.h \
    include/rfmu2/rfmu2spectrumanalyzer.h \
    include/rfmu2/rfmu2systemcontrol.h \
    include/rfmu2/rfmu2tool.h \
    logging.h \
    mainwindow.h \
    marker.h \
    nawidget.h \
    rrsucalibdialog.h \
    sawidget.h \
    sgstepworker.h \
    sgwidget.h \
    stepsweepdialog.h

SOURCES += \
    collapsiblegroupbox.cpp \
    colorpickerwidget.cpp \
    connectdialog.cpp \
    e6300plugin.cpp \
    include/frequencyspinbox.cpp \
    include/qcustomplot.cpp \
    include/rfmu2/rfmu2base.cpp \
    include/rfmu2/rfmu2networkanalyzer.cpp \
    include/rfmu2/rfmu2signalgenerator.cpp \
    include/rfmu2/rfmu2spectrumanalyzer.cpp \
    include/rfmu2/rfmu2systemcontrol.cpp \
    include/rfmu2/rfmu2tool.cpp \
    mainwindow.cpp \
    marker.cpp \
    nawidget.cpp \
    rrsucalibdialog.cpp \
    sawidget.cpp \
    sgstepworker.cpp \
    sgwidget.cpp \
    stepsweepdialog.cpp

DISTFILES += E6300Plugin.json

FORMS +=

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

TARGET = E6300Plugin

CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/../MyPluginLoader/debug/plugins
} else {
    DESTDIR = $$PWD/../MyPluginLoader/release/plugins
}

RESOURCES += \
    res.qrc
