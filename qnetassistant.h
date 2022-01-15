#ifndef QNETASSISTANT_H
#define QNETASSISTANT_H

#include <QMainWindow>
#include <QMdiSubWindow>
#include <QMdiArea>
#include <QTranslator>
#include <QApplication>
#include "newsubwindow.h"

namespace Ui {
class QNetAssistant;
}

class QNetAssistant : public QMainWindow
{
    Q_OBJECT

public:
    explicit QNetAssistant(QWidget *parent = 0);
    ~QNetAssistant();
    void InitUI();
    void InitSlot();
protected:
    virtual bool eventFilter(QObject *watched,QEvent *event)Q_DECL_OVERRIDE;
public slots:
    void SlotOnNewActTriggered();
    void SlotTabActTriggered();
    void SlotTileActTriggered();
    void SlotLoadActTriggered();
    void SlotSaveActTriggered();
    void SlotLanguageChange();
private:
    Ui::QNetAssistant *ui;
    static int _count;
    QTranslator _tsor;
    volatile bool _isTranslator;
};

#endif // QNETASSISTANT_H
