QT += widgets charts network printsupport sql

QT += core widgets charts network printsupport sql
QT += widgets charts network printsupport
TARGET = SmartOffice_Employees
TEMPLATE = app
SOURCES += main.cpp \
           databaseconnection.cpp \
           invoice.cpp \
           mainwindow.cpp
HEADERS += mainwindow.h \
    databaseconnection.h \
    invoice.h
FORMS += mainwindow.ui
CONFIG += c++11

