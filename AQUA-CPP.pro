TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        compiler.cpp \
        executor.cpp \
        main.cpp \
        module.cpp \
        utils.cpp

HEADERS += \
    compiler.h \
    executor.h \
    globals.h \
    module.h \
    utils.h
