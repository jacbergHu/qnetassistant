#include "qnetassistant.h"
#include "ui_qnetassistant.h"

int QNetAssistant::_count = 0;
QNetAssistant::QNetAssistant(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QNetAssistant)
{
    ui->setupUi(this);
    _isTranslator = false;
    InitUI();
    InitSlot();
    setMinimumSize(860,720);
    ui->mdiArea->setTabsMovable(true);
    ui->mdiArea->setTabsClosable(true);
    ui->mdiArea->setViewMode(QMdiArea::TabbedView);
    ui->mdiArea->setTabShape(QTabWidget::Rounded);
    ui->mdiArea->setBackground(Qt::NoBrush);
    Q_FOREACH(QTabBar *tab,ui->mdiArea->findChildren<QTabBar*>()){
        tab->setDrawBase(false);
        tab->setExpanding(false);
    }
    setWindowTitle("QNetAssistant");
    setWindowIcon(QIcon(QString(":/images/images/netassistant.png")));
    this->installEventFilter(this);
}

QNetAssistant::~QNetAssistant()
{
    delete ui;
}
void QNetAssistant::InitUI()
{
    this->setCentralWidget( ui->mdiArea);
    //this->setWindowState(Qt::WindowMaximized); //窗口最大化显示
    ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    ui->mdiArea->setTabsMovable(true);
    ui->mdiArea->setTabsClosable(true);
    ui->mdiArea->setViewMode(QMdiArea::TabbedView);
    ui->mdiArea->setTabShape(QTabWidget::Rounded);
    ui->mdiArea->setBackground(Qt::NoBrush);
}

void QNetAssistant::InitSlot()
{
    connect(ui->newAct,SIGNAL(triggered(bool)),this,SLOT(SlotOnNewActTriggered()));
    connect(ui->tabModeAct,SIGNAL(triggered(bool)),this,SLOT(SlotTabActTriggered()));
    connect(ui->tileAct,SIGNAL(triggered(bool)),this,SLOT(SlotTileActTriggered()));
    connect(ui->loadAct,SIGNAL(triggered(bool)),this,SLOT(SlotLoadActTriggered()));
    connect(ui->saveAct,SIGNAL(triggered(bool)),this,SLOT(SlotSaveActTriggered()));
    connect(ui->languageAct,SIGNAL(triggered(bool)),this,SLOT(SlotLanguageChange()));
}

bool QNetAssistant::eventFilter(QObject *watched, QEvent *event)
{
        QMdiSubWindow* subWidget = qobject_cast<QMdiSubWindow*>(watched);
        if(subWidget &&  event->type() == QEvent::Close){
            QMessageBox::StandardButton dialogResult =
                    QMessageBox::question(this,QString("question?"),
                                          QString("do you want to close this widget?"),
                                          QMessageBox::Yes|QMessageBox::Cancel);
            if(dialogResult == QMessageBox::Yes){
                int index = 1;
                foreach (QMdiSubWindow *subWindow,ui->mdiArea->subWindowList()) {
                    if( subWidget == subWindow){
                        continue;
                    }
                    subWindow->widget()->setWindowTitle(tr("串口网口工具")+QString::number(index++));
                }
                _count--;
                QWidget* widget = subWidget->widget();
                delete widget;

            }else{
                event->ignore();
                return true;
            }
        }else{
            QNetAssistant* widget = qobject_cast<QNetAssistant*>(watched);
            if(widget && event->type() == QEvent::Close){
                QMessageBox::StandardButton dialogResult =
                        QMessageBox::question(this,QString("question?"),
                                              QString("do you want to close this widget?"),
                                              QMessageBox::Yes|QMessageBox::Cancel);
                if(dialogResult == QMessageBox::Cancel){
                    event->ignore();
                    return true;
                }else{
                    return QMainWindow::eventFilter(watched,event);
                }
            }
        }
        return QMainWindow::eventFilter(watched,event);
}

void QNetAssistant::SlotOnNewActTriggered()
{
    QWidget *w = new NewSubWindow();
    w->setWindowTitle(tr("串口网口工具")+QString::number(++_count));
    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(w);
    w->setContentsMargins(QMargins(10,10,10,10));
    w->showMaximized();
    subWindow->installEventFilter(this);
    ui->mdiArea->setActiveSubWindow(subWindow);
}

void QNetAssistant::SlotTabActTriggered()
{
    if(ui->mdiArea->viewMode() == QMdiArea::SubWindowView){
        ui->mdiArea->setViewMode(QMdiArea::TabbedView);
        Q_FOREACH(QTabBar *tab,ui->mdiArea->findChildren<QTabBar*>()){
            tab->setDrawBase(false);
            tab->setExpanding(false);
        }
    }
}

void QNetAssistant::SlotTileActTriggered()
{
    if(ui->mdiArea->viewMode() == QMdiArea::TabbedView){
        ui->mdiArea->setViewMode(QMdiArea::SubWindowView);
        ui->mdiArea->tileSubWindows();
    }
}

void QNetAssistant::SlotLoadActTriggered()
{
    if(_count < 0){
        return ;
    }
    NewSubWindow* subWidget = qobject_cast<NewSubWindow*>(ui->mdiArea->currentSubWindow()->widget());
    subWidget->LoadConfig();
}

void QNetAssistant::SlotSaveActTriggered()
{
    if(_count < 0){
        return ;
    }
    NewSubWindow* subWidget = qobject_cast<NewSubWindow*>(ui->mdiArea->currentSubWindow()->widget());
    subWidget->SaveConfig();
}

void QNetAssistant::SlotLanguageChange()
{
    if( !_isTranslator){
        _tsor.load("../../QNetAssistant/zh_hans.qm");
        qApp->installTranslator(&_tsor);
        int index = 1;
        foreach(QMdiSubWindow *widget,ui->mdiArea->subWindowList()) {
            NewSubWindow * sub = qobject_cast<NewSubWindow*>(widget->widget());
            sub->GetNetworkWidget()->RetranslateUi();
            sub->GetSerialWidget()->RetranslateUi();
            widget->widget()->setWindowTitle(tr("Debugging Tool ") + QString::number(index++));
        }
        ui->retranslateUi(this);
    }else{
        qApp->removeTranslator(&_tsor);
        int index = 1;
        foreach(QMdiSubWindow *widget,ui->mdiArea->subWindowList()) {
            NewSubWindow * sub = qobject_cast<NewSubWindow*>(widget->widget());
            sub->GetNetworkWidget()->RetranslateUi();
            sub->GetSerialWidget()->RetranslateUi();
            widget->widget()->setWindowTitle(tr("串口网口工具") + QString::number(index++));
        }
        ui->retranslateUi(this);
    }
    _isTranslator = !_isTranslator;
    this->setWindowTitle("QNetAssistant");
}
