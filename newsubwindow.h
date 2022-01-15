#ifndef NEWSUBWINDOW_H
#define NEWSUBWINDOW_H

#include <QWidget>
#include <QSplitter>
#include <QGridLayout>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QFile>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>
#include <QByteArray>
#include "networkwidget.h"
#include "serialwidget.h"

class NewSubWindow : public QWidget
{
    Q_OBJECT
public:
    explicit NewSubWindow(QWidget *parent = nullptr);
    ~NewSubWindow();
    void SaveConfig();
    void LoadConfig();
    NetworkWidget* GetNetworkWidget();
    SerialWidget *GetSerialWidget();
signals:

public slots:
private:
    QSplitter *_splitter;
    SerialWidget *_serialWidget;
    NetworkWidget *_networkWidget;
    QGridLayout *_gridLayout;
};

#endif // NEWSUBWINDOW_H
