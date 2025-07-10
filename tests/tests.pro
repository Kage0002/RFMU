QT += testlib network core
CONFIG += console c++17 testcase

INCLUDEPATH += ../E6300TestPlugin/include
HEADERS += ../E6300TestPlugin/include/rfmu2/rfmu2base.h
SOURCES += rfmu2basetests.cpp \
           ../E6300TestPlugin/include/rfmu2/rfmu2base.cpp
