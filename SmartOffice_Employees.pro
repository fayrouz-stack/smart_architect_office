QT += widgets charts network printsupport
TARGET = SmartOffice_Employees
TEMPLATE = app
SOURCES += main.cpp \
           mainwindow.cpp
HEADERS += mainwindow.h
FORMS += mainwindow.ui
CONFIG += c++11
QT += network core
QT += widgets charts network printsupport core gui
QT += widgets charts network printsupport sql
QT += core gui widgets sql  # Add 'sql' here
