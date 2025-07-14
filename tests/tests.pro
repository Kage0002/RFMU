QT += testlib widgets gui core network
CONFIG += console c++17 testcase

INCLUDEPATH += ../E6300TestPlugin/include \
               ../E6300TestPlugin

HEADERS += ../E6300TestPlugin/sgwidget.h \
           ../E6300TestPlugin/sgstepworker.h \
           ../E6300TestPlugin/stepsweepdialog.h \
           ../E6300TestPlugin/include/frequencyspinbox.h \
           ../E6300TestPlugin/include/rfmu2/rfmu2signalgenerator.h \
           ../E6300TestPlugin/include/rfmu2/rfmu2base.h

SOURCES += sgwidgettests.cpp \
           ../E6300TestPlugin/sgwidget.cpp \
           ../E6300TestPlugin/sgstepworker.cpp \
           ../E6300TestPlugin/stepsweepdialog.cpp \
           ../E6300TestPlugin/include/frequencyspinbox.cpp \
           ../E6300TestPlugin/include/rfmu2/rfmu2signalgenerator.cpp \
           ../E6300TestPlugin/include/rfmu2/rfmu2base.cpp
