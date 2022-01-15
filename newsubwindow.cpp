#include "newsubwindow.h"

NewSubWindow::NewSubWindow(QWidget *parent) : QWidget(parent)
{
    _splitter = new QSplitter(Qt::Horizontal,this);
    _serialWidget = new SerialWidget(_splitter);
    _networkWidget = new NetworkWidget(_splitter);
    _gridLayout = new QGridLayout(this);
    _gridLayout->addWidget(_splitter);
    _splitter->setHandleWidth(2);
    _gridLayout->setMargin(0);
    _gridLayout->setHorizontalSpacing(0);
    _gridLayout->setVerticalSpacing(0);
    this->setLayout(_gridLayout);
    this->show();
}
NewSubWindow::~NewSubWindow()
{
    if( _serialWidget){
        delete _serialWidget;
        _serialWidget = nullptr;
    }
    if( _networkWidget){
        delete _networkWidget;
        _networkWidget = nullptr;
    }
    if( _splitter){
        delete _splitter;
        _splitter = nullptr;
    }
    if( _gridLayout){
        delete _gridLayout;
        _gridLayout = nullptr;
    }
}

void NewSubWindow::SaveConfig()
{
    QString path = QDir::currentPath();
    QString fileName = QFileDialog::getSaveFileName(this,tr("将配置保存为..."),path,tr("Json文件(*.json)"));
    QFile file(fileName);
    if(!file.open(QIODevice::ReadWrite | QIODevice::Text)){
        qDebug()<<"打开文件失败";
        return ;
    }
    QVariantHash json;

    QVariantHash networkSettings = _networkWidget->SaveConfig();//保存网口配置
    QVariantHash serialSetting = _serialWidget->SaveConfig();//保存串口配置
    json.insert("networkSettings",networkSettings);
    json.insert("serialSetting",serialSetting);

    QJsonObject objJson = QJsonObject::fromVariantHash(json);
    QJsonDocument jsondoc;
    jsondoc.setObject(objJson);
    file.write(jsondoc.toJson());

    file.close();
    return ;
}

void NewSubWindow::LoadConfig()
{
    //getOpenFileName(this,"打开文件源",QDir::currentPath(),"文本文件(*.txt)");
    QString configFileName = QFileDialog::getOpenFileName(this,tr("打开配置文件"),QDir::currentPath(),tr("Json文件(*.json)"));
    QFile file(configFileName);
    if( !file.open(QIODevice::ReadOnly |QIODevice::Text)){
        qDebug()<<"打开文件失败";
        return ;
    }
    QByteArray jsonData = file.readAll();
    file.close();
    QJsonParseError jsonError;
    QJsonDocument doc(QJsonDocument::fromJson(jsonData,&jsonError));

    if(jsonError.error != QJsonParseError::NoError){
        qDebug()<<"json error!"<<jsonError.error;
        return ;
    }
    QJsonObject objJson = doc.object();
    if(objJson.contains("networkSettings")){
        QJsonObject networkObj = objJson.value("networkSettings").toObject();
        _networkWidget->LoadConfig(networkObj);
    }

    if(objJson.contains("serialSetting")){
        QJsonObject serialObj = objJson.value("serialSetting").toObject();
        _serialWidget->LoadConfig(serialObj);
    }
}

NetworkWidget *NewSubWindow::GetNetworkWidget()
{
    return _networkWidget;
}

SerialWidget *NewSubWindow::GetSerialWidget()
{
    return _serialWidget;
}
