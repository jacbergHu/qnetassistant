#include "serialwidget.h"
#include "ui_serialwidget.h"

SerialWidget::SerialWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::SerialWidget) {
    ui->setupUi(this);
    this->layout()->setSpacing(0);
    this->layout()->setMargin(0);
    _serial = nullptr;
    _saveFile = nullptr;
    _sourceFile = nullptr;
    _cycleTimer = nullptr;
    _nIsConnected = false;
    _receiveCount = 0;
    _sendCount = 0;
    _rIsHex = false;
    _sIsHex = false;
    ui->SshowinHex->setEnabled(false);
    ui->sendinHex->setEnabled(false);
    ui->sendinAscii->setChecked(true);

    _timer = new QTimer(this);
    _timer->start(100);
    connect(_timer, SIGNAL(timeout()), this, SLOT(SlotUpdateState()));
    GetAvailablePorts();
    InitSignalSlot();
}

SerialWidget::~SerialWidget() {
    if(_serial) {
        _serial->close();
        delete _serial;
        _serial = nullptr;
    }
    if(_saveFile) {
        if(_saveFile->isOpen()) {
            _saveFile->close();
        }
        delete _saveFile;
        _saveFile = nullptr;
    }
    if(_sourceFile) {
        if(_sourceFile->isOpen()) {
            _sourceFile->close();
        }
        delete _sourceFile;
        _sourceFile = nullptr;
    }
    delete ui;
}

void SerialWidget::InitSignalSlot() { //初始化信号与槽
    ui->SsendText->setText("hello!");
    ui->SOpenBtn->setIconSize(QSize(22, 22));
#ifdef Q_OS_LINUX
    ui->SOpenBtn->setIcon(QIcon(":/images/images/DarkConnect.jpg"));
#endif
#ifdef Q_OS_WIN
    ui->SOpenBtn->setIcon(QIcon(":/images/images/connect.jpg"));
#endif

    _cycleTimer = new QTimer();
    connect(_cycleTimer, SIGNAL(timeout()), this, SLOT(SlotSendBtnClicked()));
    //初始化按钮信号与槽
    connect(ui->SOpenBtn, SIGNAL(clicked(bool)), this, SLOT(SlotOpenBtnClicked()));
    connect(ui->SResetBtn, SIGNAL(clicked(bool)), this, SLOT(SlotResetBtnClicked()));
    connect(ui->SDataClear, SIGNAL(clicked(bool)), this, SLOT(SlotClearReceiveBtnClicked()));
    connect(ui->SInputClear, SIGNAL(clicked(bool)), this, SLOT(SlotClearInputBtnClicked()));
    connect(ui->SsaveDate, SIGNAL(clicked(bool)), this, SLOT(SlotSaveDataBtnClicked()));
    connect(ui->sLoadFile, SIGNAL(clicked(bool)), this, SLOT(SlotFileLoadBtnClicked()));
    connect(ui->SsendBtn, SIGNAL(clicked(bool)), this, SLOT(SlotSendBtnClicked()));

    connect(ui->SshowinAscii, SIGNAL(clicked(bool)), this, SLOT(SlotShowModeChange()));
    connect(ui->SshowinHex, SIGNAL(clicked(bool)), this, SLOT(SlotShowModeChange()));
    connect(ui->sendinAscii, SIGNAL(clicked(bool)), this, SLOT(SlotSendModeChange()));
    connect(ui->sendinHex, SIGNAL(clicked(bool)), this, SLOT(SlotSendModeChange()));

    connect(ui->SReceivetoFile, SIGNAL(clicked(bool)), this, SLOT(SlotReceivetoFile()));
    connect(ui->SsendDataLoop, SIGNAL(clicked(bool)), this, SLOT(SlotChangeTimer()));
    connect(ui->SAutoAddBit, SIGNAL(clicked(bool)), this, SLOT(SlotAutoSendAddBit()));
    connect(ui->SDatafromFile, SIGNAL(clicked(bool)), this, SLOT(SlotDataFromFile()));

    connect(ui->SstopReceive, SIGNAL(clicked(bool)), this, SLOT(SlotShowDataStop()));
}

QVariantHash SerialWidget::SaveConfig() { //保存配置，返回串口配置
    QVariantHash jsonHash;
    jsonHash.insert("portName", ui->serialComBox->currentIndex());
    jsonHash.insert("baudRate", ui->baudComBox->currentIndex());
    jsonHash.insert("parity", ui->checkComBox->currentIndex());
    jsonHash.insert("data", ui->dataComBox->currentIndex());
    jsonHash.insert("stop", ui->stopComBox->currentIndex());
    jsonHash.insert("stopreceive", _status.stopRecvFlag);
    jsonHash.insert("autoclear", _status.autoSendClear);
    jsonHash.insert("sendinhex", _status.sendHexCheck);
    jsonHash.insert("sendloop", _status.dataRecycle);
    jsonHash.insert("autoenter", _status.autoEnter);
    jsonHash.insert("showinhex", _status.showHexCheck);
    jsonHash.insert("time", ui->Stime->text());
    return jsonHash;
}

void SerialWidget::LoadConfig(QJsonObject &jsonData) { //加载串口配置
    qDebug() << "serial:" << jsonData;
    ui->serialComBox->setCurrentIndex(jsonData.value("portName").toInt());
    ui->checkComBox->setCurrentIndex(jsonData.value("parity").toInt());
    ui->baudComBox->setCurrentIndex(jsonData.value("baudRate").toInt());
    ui->dataComBox->setCurrentIndex(jsonData.value("data").toInt());
    ui->stopComBox->setCurrentIndex(jsonData.value("stop").toInt());
    ui->SsendDataLoop->setChecked(jsonData.value("sendloop").toBool());
    ui->SAutoBreak->setChecked(jsonData.value("autoenter").toBool());
    bool showinhex = jsonData.value("showinhex").toBool();
    if(showinhex == true) { //接收是Hex显示
        ui->SshowinHex->setEnabled(true);
        ui->SshowinHex->setChecked(true);
        ui->SshowinAscii->setEnabled(false);
        ui->SshowinAscii->setChecked(false);
    } else {
        ui->SshowinHex->setEnabled(false);
        ui->SshowinHex->setChecked(false);
        ui->SshowinAscii->setEnabled(true);
        ui->SshowinAscii->setChecked(true);
    }

    ui->SstopReceive->setChecked(jsonData.value("stopreceive").toBool());
    ui->SAutoClear->setChecked(jsonData.value("autoclear").toBool());
    bool sendinhex = jsonData.value("sendinhex").toBool();
    if(sendinhex == true) { //发送是Hex模式
        ui->sendinHex->setEnabled(true);
        ui->sendinHex->setChecked(true);
        ui->sendinAscii->setEnabled(false);
        ui->sendinAscii->setChecked(false);
    } else {
        ui->sendinHex->setEnabled(false);
        ui->sendinHex->setChecked(false);
        ui->sendinAscii->setEnabled(true);
        ui->sendinAscii->setChecked(true);
    }
    ui->Stime->setText(jsonData.value("time").toString());
}

void SerialWidget::RetranslateUi() {
    ui->retranslateUi(this);
}

void SerialWidget::SetSerialPort() { //设置串口参数
    if(_serial->isOpen()) { //如果串口已经打开,先给他关闭
        _serial->clear();
        _serial->close();
    }
    _serial->setPortName(ui->serialComBox->currentText());//当前选择的串口名

    if(!_serial->open(QIODevice::ReadWrite)) {
        qDebug() << "打开失败!";
        _nIsConnected = false;
        ui->SInfoLabel->setText(tr("串口打开失败！"));
        ui->SOpenBtn->setText(tr("打开"));
#ifdef Q_OS_LINUX
        ui->SOpenBtn->setIcon(QIcon(":/images/images/DarkConnect.jpg"));
#endif
#ifdef Q_OS_WIN
        ui->SOpenBtn->setIcon(QIcon(":/images/images/connect.jpg"));
#endif
        ui->serialComBox->setEnabled(true);
        ui->baudComBox->setEnabled(true);
        ui->stopComBox->setEnabled(true);
        ui->dataComBox->setEnabled(true);
        ui->checkComBox->setEnabled(true);
        return;
    }
    //设置串口波特率
    _serial->setBaudRate(ui->baudComBox->currentText().toInt(), QSerialPort::AllDirections); //设置波特率
    //设置串口数据位
    int dataBit = ui->dataComBox->currentText().split(" ").at(0).toInt();
    switch(dataBit) {
    case 8:
        _serial->setDataBits(QSerialPort::Data8);//数据位为8位
        break;
    case 7:
        _serial->setDataBits(QSerialPort::Data7);
        break;
    case 6:
        _serial->setDataBits(QSerialPort::Data6);
        break;
    case 5:
        _serial->setDataBits(QSerialPort::Data5);
        break;
    }
    //设置串口校验位
    QString parity = ui->checkComBox->currentText();
    if(parity == "NONE") {
        _serial->setParity(QSerialPort::NoParity);
    } else if(parity == "ODD") {
        _serial->setParity(QSerialPort::OddParity);
    } else if(parity == "EVEN") {
        _serial->setParity(QSerialPort::EvenParity);
    } else if(parity == "MARK") {
        _serial->setParity(QSerialPort::MarkParity);
    } else if(parity == "SPACE") {
        _serial->setParity(QSerialPort::SpaceParity);
    }
    //设置串口停止位
    int stopBit = ui->stopComBox->currentText().split(" ").at(0).toInt();
    switch(stopBit) {
    case 1:
        _serial->setStopBits(QSerialPort::OneStop);
        break;
    case 2:
        _serial->setStopBits(QSerialPort::TwoStop);
        break;
    }
    connect(_serial, SIGNAL(readyRead()), this, SLOT(SlotReadData()));
    _nIsConnected = true;
    ui->SInfoLabel->setText(tr("串口打开成功！"));
    ui->serialComBox->setEnabled(false);
    ui->baudComBox->setEnabled(false);
    ui->stopComBox->setEnabled(false);
    ui->dataComBox->setEnabled(false);
    ui->checkComBox->setEnabled(false);
}

void SerialWidget::GetAvailablePorts() {
    //获取可用串口号
    foreach(const QSerialPortInfo& serialInfo, QSerialPortInfo::availablePorts()) {
        QString portName = serialInfo.portName();
        if(!_serialPortNames.contains(portName)) {
            _serialPortNames.append(portName);
        }
    }
    //ui->serialComBox->clear();
    ui->serialComBox->addItems(_serialPortNames);//将可用串口号名称显示到comboBox中
}

QByteArray SerialWidget::ChecksumCompute(QString msg) { //计算检验位
    QByteArray msgData = msg.toLatin1();
    QByteArray hexStr;
    quint64 sum = 0;
    if(_status.sendHexCheck) {  //把Hex内容先转换成字符数组
        msgData = GlobalFunction::hexStrToByteArray(msg);
    }
    for(int i = 0; i < msgData.length(); i++) { //求正校验，即各个字符之和
        sum += msgData.at(i);
    }
    if(_isNegative) {   //如果需要发送负检验，则计算负校验
        sum = -sum; //负校验:各个字符之和,取反
        hexStr = GlobalFunction::decimalToStrHex(sum).toLatin1();
        hexStr = hexStr.right(2);
    } else {
        hexStr = GlobalFunction::decimalToStrHex(sum).toLatin1();
        hexStr = hexStr.right(2);
    }
    hexStr = GlobalFunction::hexStrToByteArray(QString(hexStr));
    return hexStr;  //返回检验位的值
}

void SerialWidget::SlotShowModeChange() {
    _rIsHex = !_rIsHex;
    if(!_rIsHex) {
        ui->SshowinHex->setChecked(false);
        ui->SshowinHex->setEnabled(false);
        ui->SshowinAscii->setChecked(true);
        ui->SshowinAscii->setEnabled(true);
    } else {
        ui->SshowinHex->setChecked(true);
        ui->SshowinHex->setEnabled(true);
        ui->SshowinAscii->setChecked(false);
        ui->SshowinAscii->setEnabled(false);
    }
}

void SerialWidget::SlotSendModeChange() {
    _sIsHex = !_sIsHex;
    if(_sIsHex) {
        ui->sendinAscii->setEnabled(false);
        ui->sendinHex->setEnabled(true);
        ui->sendinHex->setChecked(true);
        ui->SsendText->setText(GlobalFunction::byteArrayToHexStr(ui->SsendText->toPlainText().toLatin1()));
    } else {
        ui->sendinHex->setEnabled(false);
        ui->sendinAscii->setEnabled(true);
        ui->sendinAscii->setChecked(true);
        ui->SsendText->setText(GlobalFunction::hexStrToByteArray(ui->SsendText->toPlainText()));
    }
}

void SerialWidget::SlotOpenBtnClicked() {
    if(ui->SOpenBtn->text() == QString(tr("打开"))) {
        if(_serial == nullptr) {
            _serial = new QSerialPort();
        }
        ui->SOpenBtn->setBackgroundRole(QPalette::ButtonText);
        ui->SOpenBtn->setText(QString(tr("关闭")));
#ifdef Q_OS_LINUX
        ui->SOpenBtn->setIcon(QIcon(":/images/images/DarkDisconnect.jpg"));
#endif
#ifdef Q_OS_WIN
        ui->SOpenBtn->setIcon(QIcon(":/images/images/disconnect.jpg"));
#endif
        SetSerialPort();
    } else if(ui->SOpenBtn->text() == QString(tr("关闭"))) {
        ui->SOpenBtn->setText(QString(tr("打开")));
#ifdef Q_OS_LINUX
        ui->SOpenBtn->setIcon(QIcon(":/images/images/DarkConnect.jpg"));
#endif
#ifdef Q_OS_WIN
        ui->SOpenBtn->setIcon(QIcon(":/images/images/connect.jpg"));
#endif
        disconnect(_serial, SIGNAL(readyRead()), this, SLOT(SlotReadData()));
        if(_serial) {
            delete _serial;
            _serial = nullptr;
        }
        ui->SInfoLabel->setText(tr("串口已关闭!"));
        ui->serialComBox->setEnabled(true);
        ui->baudComBox->setEnabled(true);
        ui->stopComBox->setEnabled(true);
        ui->dataComBox->setEnabled(true);
        ui->checkComBox->setEnabled(true);
    }
}

void SerialWidget::SlotClearReceiveBtnClicked() { //清空接收区
    ui->SReceiveText->clear();
}

void SerialWidget::SlotResetBtnClicked() { //复位计数
    _receiveCount = 0;
    _sendCount = 0;
    ui->sReceiveCount->setText(tr("接收：0"));
    ui->SsendCount->setText(tr("发送：0"));
}

void SerialWidget::SlotSendBtnClicked() {
    static int flag = 0;
    if(sender() == ui->SsendBtn) {
        if(_status.dataRecycle) {
            if(!_cycleTimer->isActive()) {
                _cycleTimer->start();
            }
            flag++;
        }
    }
    if(flag == 2) {
        if(_cycleTimer->isActive()) {
            _cycleTimer->stop();
        }
        flag = 0;
        ui->SsendBtn->setText(tr("发送"));
        return ;
    }
    if(_status.dataRecycle && flag == 1) {
        ui->SsendBtn->setText(tr("停止发送"));
    }
    if(!_nIsConnected) {
        QMessageBox::warning(this, QString(tr("warning")), QString(tr("没有建立连接!")), QMessageBox::Ok);
        return ;
    }
    QString sendData = ui->SsendText->toPlainText();
    if(sendData.length() <= 0) {
        QMessageBox::warning(this, QString(tr("warning")), QString(tr("发送区内容为空!")), QMessageBox::Ok);
        return ;
    }

    QByteArray buffer;
    if(_status.sendHexCheck) {     //发送是以什么格式发送ASCII/Hex
        if(_status.sendChecksum) { //是否发送校验位
            buffer = GlobalFunction::hexStrToByteArray(sendData);
            buffer = buffer + GlobalFunction::byteArrayToAsciiStr(ChecksumCompute(sendData.toLatin1())).toLatin1();
        } else {
            buffer = GlobalFunction::hexStrToByteArray(sendData);
        }
    } else if(_status.sendAsciiCheck) {
        if(_status.sendChecksum) {
            buffer = GlobalFunction::asciiStrToByteArray(sendData);
            buffer = buffer + ChecksumCompute(sendData.toLatin1());
        } else {
            buffer = GlobalFunction::asciiStrToByteArray(sendData);
        }
    }
    _serial->write(buffer);

    _sendCount += sendData.length();
    ui->SsendCount->setText(tr("发送：") + QString::number(_sendCount));
    if(_status.autoSendClear) {
        ui->SsendText->clear();
    }
}

void SerialWidget::SlotClearInputBtnClicked() { //清空发送区
    ui->SsendText->clear();
}

void SerialWidget::SlotSaveDataBtnClicked() {
    QString msgData = ui->SReceiveText->toPlainText();
    if(msgData.length() <= 0) {
        QMessageBox::warning(this, QString(tr("warning")), QString(tr("没有内容可以保存!")), QMessageBox::Ok);
        return ;
    }
    QString path = QDir::currentPath();
    QString fileName = QFileDialog::getSaveFileName(this, tr("将接收文本保存为..."), path, tr("文本文件(*.txt)"));
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly | QFile::Text)) {
        qDebug() << "文件打开失败!";
        return ;
    }
    file.write(msgData.toLatin1().data());
    file.close();
    SlotClearReceiveBtnClicked();
}

void SerialWidget::SlotFileLoadBtnClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件源"), QDir::currentPath(), tr("文本文件(*.txt)"));
    QFile file(fileName);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "文件打开失败!";
        return ;
    }
    QString fileMsg = file.readAll();
    if(_sIsHex) {
        fileMsg = GlobalFunction::byteArrayToHexStr(fileMsg.toLatin1());
    }
    ui->SsendText->setText(fileMsg);
}

void SerialWidget::SlotReadData() {
    QByteArray msg = _serial->readAll();
    _receiveCount += msg.length();
    if(_status.turnToFile) {
        if(_saveFile) {
            if(_saveFile->isOpen()) {
                _saveFile->write(msg.data());
            }
        }
    } else {
        if(_status.stopRecvFlag) {
            ui->sReceiveCount->setText(tr("接收：") + QString::number(_receiveCount));
            return;
        }
        QString buffer;
        if(_status.showHexCheck) {
            buffer = GlobalFunction::byteArrayToHexStr(msg);
        } else if(_status.showAsciiCheck) {
            //buffer = GlobalFunction::byteArrayToAsciiStr(data);
            buffer = QString(msg);
        }
        if(_status.autoEnter) {
            ui->SReceiveText->insertPlainText(buffer + "\n");
        } else {
            ui->SReceiveText->insertPlainText(buffer);
        }
        ui->SReceiveText->moveCursor(QTextCursor::End);
    }
    ui->sReceiveCount->setText(tr("接收:") + QString::number(_receiveCount));
}

void SerialWidget::SlotUpdateState() {
    _status.autoEnter = ui->SAutoBreak->isChecked();
    _status.autoSendClear = ui->SAutoClear->isChecked();
    _status.dataRecycle = ui->SsendDataLoop->isChecked();
    _status.showHexCheck = ui->SshowinHex->isChecked();
    _status.sendHexCheck = ui->sendinHex->isChecked();
    _status.sendAsciiCheck = ui->sendinAscii->isChecked();
    _status.stopRecvFlag = ui->SstopReceive->isChecked();
    _status.time = ui->Stime->text().toInt();
    _status.turnToFile = ui->SReceivetoFile->isChecked();
    _status.showAsciiCheck = ui->SshowinAscii->isChecked();
    _status.sendChecksum = ui->SAutoAddBit->isChecked();
}

void SerialWidget::SlotReceivetoFile() {
    if(ui->SReceivetoFile->isChecked()) {
        QString path = QDir::currentPath();
        QString file = QFileDialog::getSaveFileName(this, tr("将接收文本保存为..."), path, tr("文本文件(*.txt)"));
        qDebug() << "filePath:" << file;
        if(_saveFile == nullptr) {
            _saveFile = new QFile(file);
            if(!_saveFile->open(QIODevice::ReadWrite | QIODevice::Text)) {
                qDebug() << tr("文件打开失败!");
                ui->SReceivetoFile->setChecked(false);
                return ;
            }
        }
        ui->SReceiveText->clear();
        ui->SReceiveText->insertPlainText(tr("接收存入文件") + file);
        ui->SReceiveText->setEnabled(false);
    } else {
        ui->SReceiveText->clear();
        ui->SReceiveText->setEnabled(true);
    }
}

void SerialWidget::SlotDataFromFile() {
    if(ui->SDatafromFile->isChecked()) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件源"), QDir::currentPath(), tr("文本文件(*.txt)"));
        if(_sourceFile == nullptr) {
            _sourceFile = new QFile(fileName);
            if(!_sourceFile->open(QIODevice::ReadOnly | QIODevice:: Text)) {
                qDebug() << "文件打开失败!";
                return ;
            }
            ui->SsendText->clear();
            QString fileMsg = _sourceFile->readAll();
            if(_sIsHex) {
                fileMsg = GlobalFunction::byteArrayToHexStr(fileMsg.toLatin1());
            }
            ui->SsendText->insertPlainText(fileMsg);
        }
    } else {
        ui->SsendText->clear();
    }
}

void SerialWidget::SlotAutoSendAddBit() {
    if(ui->SAutoAddBit->isChecked()) {
        QMessageBox::StandardButton dialogResult =
            QMessageBox::question(this, QString(tr("question?")),
                                  QString(tr("附加发送负校验?")), QMessageBox::Yes | QMessageBox::No);
        if(dialogResult == QMessageBox::Yes) {
            _isNegative = true;
        } else {
            _isNegative = false;
        }
    }
}

void SerialWidget::SlotChangeTimer() {
    _cycleTimer->setInterval(_status.time);
    if(ui->SsendDataLoop->isChecked()) {
        ui->Stime->setEnabled(false);
        if(!_cycleTimer->isActive()) {
            //connect(_cycleTimer,SIGNAL(timeout()),this,SLOT(SlotSendBtnClicked()));
        }
    } else {
        ui->Stime->setEnabled(true);
        if(_cycleTimer->isActive()) {
            _cycleTimer->stop();
            ui->SsendBtn->setText(tr("发送"));
        }
    }
}

void SerialWidget::SlotShowDataStop() {
    if(ui->SstopReceive->isChecked()) {
        ui->SReceiveText->setEnabled(false);
    } else {
        ui->SReceiveText->setEnabled(true);
    }
}
