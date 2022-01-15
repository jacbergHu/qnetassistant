#include "networkwidget.h"
#include "ui_networkwidget.h"
NetworkWidget::NetworkWidget(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::NetworkWidget) {
    ui->setupUi(this);
    this->layout()->setSpacing(0);
    this->layout()->setMargin(0);
    qRegisterMetaType<QByteArray>("QByteArray&");
    _nIsConnected = false;
    _nIsConnectBtnStatus = false;
    _udpSocket = nullptr;
    _server = nullptr;
    _client = nullptr;
    _saveFile = nullptr;
    _sourceFile = nullptr;
    _lastSocket = nullptr;
    _timer = new QTimer(this);
    _timer->start(100);
    connect(_timer, SIGNAL(timeout()), this, SLOT(SlotUpdateStatus()));
    initIP();
    InitNetwork();
    InitUdpConnect();
    InitTCPServerConnect();
}

NetworkWidget::~NetworkWidget() {
    if(_udpSocket) {
        _udpSocket->close();
        delete _udpSocket;
        _udpSocket = nullptr;
    }
    if(_server) {
        _server->destroyed();
        _server->close();
        delete _server;
        _server = nullptr;
    }
    if(_client) {
        _client->disconnectFromHost();
        delete _client;
        _client = nullptr;
    }
    if(_udpWidget) {
        _udpWidget->deleteLater();
    }
    if(_serverWidget) {
        _serverWidget->deleteLater();
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

void NetworkWidget::InitNetwork() {
//    ui->NReceiveText->setReadOnly(true);
    ui->NSendText->setText("hello!");
    ui->NShowinHex->setEnabled(false);
    ui->NSendinHex->setEnabled(false);
    ui->ProtocolType->setCurrentIndex(1);
    ui->NReceiveText->setWordWrapMode(QTextOption::WrapAnywhere);
    ui->NConnectBtn->setText(tr("开始监听"));
    ui->NConnectBtn->setIconSize(QSize(22, 22));

#ifdef Q_OS_LINUX
    ui-> NConnectBtn->setIcon(QIcon(":/images/images/DarkConnect.jpg"));
#endif
#ifdef Q_OS_WIN
    ui-> NConnectBtn->setIcon(QIcon(":/images/images/connect.jpg"));
#endif

    _cycleSendTimer = new QTimer(this);
    connect(_cycleSendTimer, SIGNAL(timeout()), this, SLOT(SlotSendBtnClicked()));
    connect(ui->NloopSendData, SIGNAL(clicked(bool)), this, SLOT(SlotChangeTimer()));

    connect(ui->NShowinAscii, SIGNAL(clicked(bool)), this, SLOT(SlotNShowModeChange()));
    connect(ui->NShowinHex, SIGNAL(clicked(bool)), this, SLOT(SlotNShowModeChange()));
    connect(ui->NSendinAscii, SIGNAL(clicked(bool)), this, SLOT(SlotNSendModeChange()));
    connect(ui->NSendinHex, SIGNAL(clicked(bool)), this, SLOT(SlotNSendModeChange()));

    connect(ui->NReceivetoFile, SIGNAL(clicked(bool)), this, SLOT(SlotNReceivetoFile()));
    connect(ui->NDataFromFile, SIGNAL(clicked(bool)), this, SLOT(SlotDataFromFile()));
    connect(ui->NAutoAddBit, SIGNAL(clicked(bool)), this, SLOT(SlotAutoSendAddBit()));

    connect(ui->NClearReceive, SIGNAL(clicked(bool)), this, SLOT(SlotClearReceiveBtnClicked()));
    connect(ui->NClearInput, SIGNAL(clicked(bool)), this, SLOT(SlotClearInputBtnClicked()));
    connect(ui->NConnectBtn, SIGNAL(clicked(bool)), this, SLOT(SlotNConnectBtnClicked()));
    connect(ui->NResetBtn, SIGNAL(clicked(bool)), this, SLOT(SlotResetBtnClicked()));
    connect(ui->NSendBtn, SIGNAL(clicked(bool)), this, SLOT(SlotSendBtnClicked()));
    connect(ui->NSaveData, SIGNAL(clicked(bool)), this, SLOT(SlotSaveDataBtnClicked()));
    connect(ui->NLoadFile, SIGNAL(clicked(bool)), this, SLOT(SlotFileLoadBtnClicked()));

    connect(ui->PortLine, SIGNAL(editingFinished()), this, SLOT(SlotCheckPort()));
    connect(ui->NPause, SIGNAL(clicked(bool)), this, SLOT(SlotShowDataStop()));

    connect(ui->ProtocolType, SIGNAL(currentIndexChanged(int)), this, SLOT(SlotProtocolTypeChange(int)));
}
void NetworkWidget::initIP() {
    //获取本机所有IP
    QStringList ips;
    QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
    foreach(const QNetworkInterface  &netInterface, netInterfaces) {
        //移除虚拟机和抓包工具的虚拟网卡
        QString humanReadableName = netInterface.humanReadableName().toLower();
        if(humanReadableName.startsWith("vmware network adapter") || humanReadableName.startsWith("npcap loopback adapter")) {
            continue;
        }

        //过滤当前网络接口
        bool flag = (netInterface.flags() == (QNetworkInterface::IsUp | QNetworkInterface::IsRunning | QNetworkInterface::CanBroadcast | QNetworkInterface::CanMulticast));
        if(flag) {
            QList<QNetworkAddressEntry> addrs = netInterface.addressEntries();
            foreach(QNetworkAddressEntry addr, addrs) {
                //只取出IPV4的地址
                if(addr.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QString ip4 = addr.ip().toString();
                    if(ip4 != "127.0.0.1") {
                        ips.append(ip4);
                    }
                }
            }
        }
    }
    ui->IPLine->setText(ips.at(0));
}

void NetworkWidget::InitUdpConnect() {
    _udpWidget = new QWidget(this);
    _udpPortLabel.setText(tr("端口号:"));
    _udpIPLabel.setText(tr("目标主机:"));
    _udpIP.setText("127.0.0.1");
    _udpPort.setText("8000");
    QHBoxLayout* h = new QHBoxLayout();
    h->addWidget(&_udpIPLabel);
    h->addWidget(&_udpIP);
    h->addWidget(&_udpPortLabel);
    h->addWidget(&_udpPort);
    h->setStretch(0, 1);
    h->setStretch(1, 2);
    h->setStretch(2, 1);
    h->setStretch(3, 1);
    h->setSpacing(0);
    h->setMargin(0);
    _udpWidget->setLayout(h);
    _udpWidget->setMaximumHeight(30);
    ui->NVBLayout->addWidget(_udpWidget);
    _udpWidget->hide();
}

void NetworkWidget::InitTCPServerConnect() {
    _serverWidget = new QWidget(this);
    _serverLabel.setText(tr("连接对象:"));
    _clientInfo.addItem(tr("所有连接的客户端"));

    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->addWidget(&_serverLabel);
    hLayout->addWidget(&_clientInfo);
    hLayout->setSpacing(0);
    hLayout->setMargin(0);
    hLayout->setStretch(0, 1);
    hLayout->setStretch(1, 3);
    _serverWidget->setLayout(hLayout);
    _serverWidget->setMaximumHeight(30);
    ui->NVBLayout->addWidget(_serverWidget);
    _serverWidget->hide();
}

void NetworkWidget::TcpClientReadData() {
    QByteArray data =  _client->readAll();
    if(data.length() <= 0) {
        return;
    }
    if(data.left(5) == "time:") {
        data = data.mid(5);
        QDateTime serverTime = QDateTime::fromString(QString(data), "yyyy/MM/dd hh:mm:ss.zzz");
        QDateTime clientTime = QDateTime::currentDateTime();
        uint timeDifference = serverTime.msecsTo(clientTime);
        if(timeDifference > 50) {
            qDebug() << "timeDifference =" << timeDifference;
            QString date = QString("date -s ") + QString(data).split(" ").at(0);
            QString time = QString("date -s ") + QString(data).split(" ").at(1);
            system(date.toLatin1().data());
            system(time.toLatin1().data());
            system("clock -w");
        }
        return ;
    }
    if(data == "disconnected") {
        delete _client;
        _client = nullptr;
        ui->NConnectBtn->setText(tr("连接"));
        _nIsConnectBtnStatus = false;
        ui->ProtocolType->setEnabled(true);
        ui->IPLine->setEnabled(true);
        ui->PortLine->setEnabled(true);
        ui->NInfoLabel->setText(tr("客户端从服务断开连接"));
        _nIsConnected = false;
        return ;
    }
    //data = GlobalFunction::asciiStrToByteArray(QString(data));
    _receiveCount += data.length();
    if(_nStatus.turnToFile) {
        if(_saveFile) {
            if(_saveFile->isOpen()) {
                _saveFile->write(data.data());
            }
        }
    } else {
        if(_nStatus.stopRecvFlag) {
            ui->NReceiveCount->setText(tr("接收：") + QString::number(_receiveCount));
            return;
        }
        QString buffer;
        if(_nStatus.showHexCheck) {
            buffer = GlobalFunction::byteArrayToHexStr(data);
        } else if(_nStatus.showAsciiCheck) {
            //buffer = GlobalFunction::byteArrayToAsciiStr(data);
            buffer = QString(data);
        }
        if(_nStatus.autoEnter) {
            ui->NReceiveText->insertPlainText(buffer + "\n");
        } else {
            ui->NReceiveText->insertPlainText(buffer);
        }
        ui->NReceiveText->moveCursor(QTextCursor::End);
    }
    ui->NReceiveCount->setText(tr("接收：") + QString::number(_receiveCount));
}

void NetworkWidget::UdpReadData() {
    QByteArray datas;
    datas.resize(_udpSocket->pendingDatagramSize());
    QHostAddress sourceIP;
    quint16 sourcePort;
    qint64 datacount;
    while(_udpSocket->hasPendingDatagrams()) {
        datacount = _udpSocket->readDatagram(datas.data(), _udpSocket->pendingDatagramSize(), &sourceIP, &sourcePort);
        _receiveCount += datacount;
        datas = GlobalFunction::asciiStrToByteArray(QString(datas));
        if(_nStatus.turnToFile) {
            if(_saveFile) {
                if(_saveFile->isOpen()) {
                    _saveFile->write(datas.data());
                }
            }
            continue;
        } else {
            if(_nStatus.stopRecvFlag) {
                continue;
            }
            QString receiveData = "[from " + sourceIP.toString() + ":" + QString::number(sourcePort) + "]:  ";
            ui->NReceiveText->insertPlainText(receiveData);
            ui->NReceiveText->moveCursor(QTextCursor::End);
            if(_nStatus.showHexCheck) {
                receiveData = GlobalFunction::byteArrayToHexStr(datas);
            } else if(_nStatus.showAsciiCheck) {
                //receiveData = GlobalFunction::byteArrayToAsciiStr(datas);
                receiveData = datas.data();
            }
            if(_nStatus.autoEnter) {
                ui->NReceiveText->insertPlainText(receiveData + "\n");
            } else {
                ui->NReceiveText->insertPlainText(receiveData);
            }
            ui->NReceiveText->moveCursor(QTextCursor::End);
        }
    }
    ui->NReceiveCount->setText(tr("接收：") + QString::number(_receiveCount));
}

QByteArray NetworkWidget::ChecksumCompute(QString msg) {
    QByteArray msgData = msg.toLatin1();
    QByteArray hexStr;
    quint64 sum = 0;
    if(_nStatus.sendHexCheck) {
        msgData = GlobalFunction::hexStrToByteArray(msg);
    }
    for(int i = 0; i < msgData.length(); i++) {
        sum += msgData.at(i);
    }
    if(_isNegative) {
        sum = -sum;
        hexStr = GlobalFunction::decimalToStrHex(sum).toLatin1();
        hexStr = hexStr.right(2);
    } else {
        hexStr = GlobalFunction::decimalToStrHex(sum).toLatin1();
        hexStr = hexStr.right(2);
    }
    hexStr = GlobalFunction::hexStrToByteArray(QString(hexStr));
    return hexStr;
}

void NetworkWidget::removeAllClient() {
    QList<QTcpSocket*> socketList = _clientList.values();
    foreach(QTcpSocket* socket, socketList) {
        QString address  = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());
        int index = _clientInfo.findText(address);
        _clientInfo.removeItem(index);
        socket->write("disconnected");
    }
    _clientList.clear();
    _clientCount = 0;
    ui->NInfoLabel->setText(QString(tr("服务已停止监听")));
}

void NetworkWidget::removeClient(qintptr handle) {
    QTcpSocket* socket = _clientList.value(handle);
    socket->disconnectFromHost();
    _clientList.remove(handle);
    QString address  = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());
    int index = _clientInfo.findText(address);
    _clientInfo.removeItem(index);
    _clientCount--;
    ui->NInfoLabel->setText(QString(tr("共%1个客户端接入")).arg(_clientCount));
}

QVariantHash NetworkWidget::SaveConfig() {
    QVariantHash jsonHash;
    jsonHash.insert("protocoltype", ui->ProtocolType->currentIndex());
    jsonHash.insert("IP", ui->IPLine->text());
    jsonHash.insert("port", ui->PortLine->text());
    jsonHash.insert("autoenter", _nStatus.autoEnter);
    jsonHash.insert("showinhex", _nStatus.showHexCheck);
    jsonHash.insert("stopreceive", _nStatus.stopRecvFlag);
    jsonHash.insert("autoclear", _nStatus.autoSendClear);
    jsonHash.insert("sendinhex", _nStatus.sendHexCheck);
    jsonHash.insert("sendloop", _nStatus.dataRecycle);
    jsonHash.insert("time", ui->Ntime->text());
    return jsonHash;
}

void NetworkWidget::LoadConfig(QJsonObject &jsonData) {
    qDebug() << "network:" << jsonData;
    ui->NloopSendData->setChecked(jsonData.value("sendloop").toBool());
    int protocolType = jsonData.value("protocoltype").toInt();
    ui->ProtocolType->setCurrentIndex(protocolType);
    ui->IPLine->setText(jsonData.value("IP").toString());
    ui->PortLine->setText(jsonData.value("port").toString());
    ui->NAutoBreak->setChecked(jsonData.value("autoenter").toBool());
    bool showinhex = jsonData.value("showinhex").toBool();
    if(showinhex == true) {
        ui->NShowinHex->setEnabled(true);
        ui->NShowinHex->setChecked(true);
        ui->NShowinAscii->setEnabled(false);
        ui->NShowinAscii->setChecked(false);
    } else {
        ui->NShowinHex->setEnabled(false);
        ui->NShowinHex->setChecked(false);
        ui->NShowinAscii->setEnabled(true);
        ui->NShowinAscii->setChecked(true);
    }

    ui->NPause->setChecked(jsonData.value("stopreceive").toBool());
    ui->NAutoSendClear->setChecked(jsonData.value("autoclear").toBool());
    bool sendinhex = jsonData.value("sendinhex").toBool();
    if(sendinhex == true) {
        ui->NSendinHex->setEnabled(true);
        ui->NSendinHex->setChecked(true);
        ui->NSendinAscii->setEnabled(false);
        ui->NSendinAscii->setChecked(false);
    } else {
        ui->NSendinHex->setEnabled(false);
        ui->NSendinHex->setChecked(false);
        ui->NSendinAscii->setEnabled(true);
        ui->NSendinAscii->setChecked(true);
    }
    ui->Ntime->setText(jsonData.value("time").toString());
}

void NetworkWidget::RetranslateUi() {
    ui->retranslateUi(this);
}

void NetworkWidget::SlotChangeTimer() {
    _cycleSendTimer->setInterval(_nStatus.time);
    if(ui->NloopSendData->isChecked()) {
        ui->Ntime->setEnabled(false);
        if(!_cycleSendTimer->isActive()) {
            //_cycleSendTimer->start();
        }
    } else {
        ui->Ntime->setEnabled(true);
        if(_cycleSendTimer->isActive()) {
            _cycleSendTimer->stop();

            ui->NSendBtn->setText(tr("发送"));
        }
    }
}

void NetworkWidget::SlotNConnectBtnClicked() {
    _nIsConnectBtnStatus = !_nIsConnectBtnStatus;
    if(_nIsConnectBtnStatus == true) {//建立连接
        ui->ProtocolType->setEnabled(false);
        ui->IPLine->setEnabled(false);
        ui->PortLine->setEnabled(false);
        switch(_nStatus.ProtocolType) {
        case ProtocolType::UDP:
            ui->NConnectBtn->setText(tr("断开"));
#ifdef Q_OS_LINUX
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/DarkDisconnect.jpg"));
#endif
#ifdef Q_OS_WIN
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/disconnect.jpg"));
#endif
            if(_udpSocket == nullptr) {
                _udpSocket = new QUdpSocket();
            }
            if(_udpSocket->bind(QHostAddress(ui->IPLine->text()), ui->PortLine->text().toInt())) {
                ui->NInfoLabel->setText(tr("绑定成功"));
                if(_udpWidget->isHidden()) {
                    _udpWidget->show();
                }
                _nIsConnected = true;
            } else {
                ui->NInfoLabel->setText(tr("绑定失败"));
                _nIsConnected = false;
            }
            connect(_udpSocket, SIGNAL(readyRead()), this, SLOT(SlotReadData()));
            break;
        case ProtocolType::TCP_Server:
            ui->NConnectBtn->setText(tr("停止监听"));
#ifdef Q_OS_LINUX
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/DarkDisconnect.jpg"));
#endif
#ifdef Q_OS_WIN
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/disconnect.jpg"));
#endif
            if(_server == nullptr) {
                _server = new TcpServer();
            }
            if(_server->listen(QHostAddress(ui->IPLine->text()), ui->PortLine->text().toInt())) {
                ui->NInfoLabel->setText(tr("服务开始监听"));
                if(_serverWidget->isHidden()) {
                    _serverWidget->show();
                }
                _nIsConnected = true;
            } else {
                ui->NInfoLabel->setText(tr("建立服务失败"));
                _nIsConnected = false;
            }
            connect(_server, SIGNAL(SignalClientConnected(qintptr, QTcpSocket*)), this, SLOT(SlotNewConnection(qintptr, QTcpSocket*)));
            connect(_server, SIGNAL(SignalClientDisConnected(qintptr)), this, SLOT(SlotClientDisConnected(qintptr)));
            connect(_server, SIGNAL(SignalReadData(qintptr, QByteArray)), this, SLOT(SlotServerReadData(qintptr, const QByteArray &)));
            break;
        case ProtocolType::TCP_Client:
            ui->NConnectBtn->setText(tr("断开"));
#ifdef Q_OS_LINUX
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/DarkDisconnect.jpg"));
#endif
#ifdef Q_OS_WIN
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/disconnect.jpg"));
#endif
            if(_client == nullptr) {
                _client = new QTcpSocket();
            }
            _client->abort();
            _client->connectToHost(ui->IPLine->text(), ui->PortLine->text().toInt(), QAbstractSocket::ReadWrite);
            if(_client->state() == QAbstractSocket::ConnectingState ||
                    _client->state() == QAbstractSocket::ConnectedState) {
                _nIsConnected = true;
                ui->NInfoLabel->setText(tr("客户端连接服务成功"));
            } else {
                _nIsConnected = false;
                ui->NInfoLabel->setText(tr("客户端连接服务失败"));
                SlotNConnectBtnClicked();
            }
            connect(_client, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(SlotNConnectBtnClicked()));
            //connect(_client,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(SlotOnStateChange(QAbstractSocket::SocketState)));
            connect(_client, SIGNAL(readyRead()), this, SLOT(SlotReadData()));
            break;
        }
    } else { //断开连接
        QMessageBox::StandardButton dialogResult =
            QMessageBox::question(this, QString("question?"),
                                  QString(tr("是否确定断开连接?")), QMessageBox::Yes | QMessageBox::No);
        if(dialogResult == QMessageBox::No) {
            _nIsConnectBtnStatus = !_nIsConnectBtnStatus;
            return ;
        }
        ui->ProtocolType->setEnabled(true);
        ui->IPLine->setEnabled(true);
        ui->PortLine->setEnabled(true);
        switch(_nStatus.ProtocolType) {
        case ProtocolType::UDP:
            ui->NConnectBtn->setText(tr("连接"));
#ifdef Q_OS_LINUX
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/DarkConnect.jpg"));
#endif
#ifdef Q_OS_WIN
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/connect.jpg"));
#endif
            if(_udpSocket) {
                _udpSocket->close();
                delete _udpSocket;
                _udpSocket = nullptr;
            }
            _udpWidget->hide();
            ui->NInfoLabel->setText(tr("解除绑定"));
            break;
        case ProtocolType::TCP_Server:
            ui->NConnectBtn->setText(tr("开始监听"));
#ifdef Q_OS_LINUX
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/DarkConnect.jpg"));
#endif
#ifdef Q_OS_WIN
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/connect.jpg"));
#endif
            if(_server) {
                removeAllClient();
                _server->close();
                delete _server;
                _server = nullptr;
            }
            _serverWidget->hide();
            ui->NInfoLabel->setText(tr("服务停止监听"));
            break;
        case ProtocolType::TCP_Client:
            ui->NConnectBtn->setText(tr("连接"));
#ifdef Q_OS_LINUX
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/DarkConnect.jpg"));
#endif
#ifdef Q_OS_WIN
            ui-> NConnectBtn->setIcon(QIcon(":/images/images/connect.jpg"));
#endif
            if(_client) {
                _client->abort();
                delete _client;
                _client = nullptr;
            }
            ui->NInfoLabel->setText(tr("客户端从服务断开连接"));
            break;
        }
        _nIsConnected = false;
    }
}

void NetworkWidget::SlotOnStateChange(QAbstractSocket::SocketState state) {
    qDebug() << state;
}

void NetworkWidget::SlotUpdateStatus() {
    _nStatus.autoSendClear = ui->NAutoSendClear->isChecked();
    _nStatus.dataRecycle = ui->NloopSendData->isChecked();
    _nStatus.sendHexCheck = ui->NSendinHex->isChecked();
    _nStatus.showHexCheck = ui->NShowinHex->isChecked();
    _nStatus.stopRecvFlag = ui->NPause->isChecked();
    _nStatus.time = ui->Ntime->text().toInt();
    _nStatus.turnToFile = ui->NReceivetoFile->isChecked();
    _nStatus.showAsciiCheck = ui->NShowinAscii->isChecked();
    _nStatus.ProtocolType = ui->ProtocolType->currentIndex();
    _nStatus.autoEnter = ui->NAutoBreak->isChecked();
    _nStatus.sendAsciiCheck = ui->NSendinAscii->isChecked();
    _nStatus.sendChecksum = ui->NAutoAddBit->isChecked();
}

void NetworkWidget::SlotCheckPort() {
    if(ui->PortLine->text().toInt() > MAX_PORT) {
        QMessageBox::warning(this, QString(tr("warning")), QString(tr("超出最大端口65535!")), QMessageBox::Ok);
    } else if(ui->PortLine->text().toInt() < MIN_PORT) {
        QMessageBox::warning(this, QString(tr("warning")), QString(tr("低于最小端口1024!")), QMessageBox::Ok);
    }
}

void NetworkWidget::SlotNReceivetoFile() { //网络接收转向文件
    if(ui->NReceivetoFile->isChecked()) {
        QString path = QDir::currentPath();
        QString file = QFileDialog::getSaveFileName(this, tr("将接收文本保存为..."), path, tr("文本文件(*.txt)"));
        qDebug() << "filePath:" << file;
        if(_saveFile == nullptr) {
            _saveFile = new QFile(file);
            if(!_saveFile->open(QIODevice::ReadWrite | QIODevice::Text)) {
                qDebug() << "文件打开失败!";
                ui->NReceivetoFile->setChecked(false);
                return ;
            }
        }
        ui->NReceiveText->setEnabled(false);
        ui->NReceiveText->clear();
        ui->NReceiveText->insertPlainText(tr("接收存入文件") + file);
    } else {
        ui->NReceiveText->clear();
        ui->NReceiveText->setEnabled(true);
    }
}

void NetworkWidget::SlotDataFromFile() {
    if(ui->NDataFromFile->isChecked()) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("打开文件源"), QDir::currentPath(), tr("文本文件(*.txt)"));
        if(_sourceFile == nullptr) {
            _sourceFile = new QFile(fileName);
            if(!_sourceFile->open(QIODevice::ReadOnly | QIODevice:: Text)) {
                qDebug() << "文件打开失败!";
                return ;
            }
            ui->NSendText->clear();
            QString fileMsg = _sourceFile->readAll();
            if(_sIsHex) {
                fileMsg = GlobalFunction::byteArrayToHexStr(fileMsg.toLatin1());
            }
            ui->NSendText->insertPlainText(fileMsg);
        }
    } else {
        ui->NSendText->clear();
    }
}

void NetworkWidget::SlotAutoSendAddBit() {
    if(ui->NAutoAddBit->isChecked()) {
        QMessageBox::StandardButton dialogResult =
            QMessageBox::question(this, QString("question?"),
                                  QString(tr("附加发送负校验?")), QMessageBox::Yes | QMessageBox::No);
        if(dialogResult == QMessageBox::Yes) {
            _isNegative = true;
        } else {
            _isNegative = false;
        }
    }
}

void NetworkWidget::SlotClearReceiveBtnClicked() { //清空接收区
    ui->NReceiveText->clear();
}

void NetworkWidget::SlotResetBtnClicked() {
    _receiveCount = 0;
    _sendCount = 0;
    ui->NReceiveCount->setText(tr("接收：0"));
    ui->NSendCount->setText(tr("发送：0"));
}

void NetworkWidget::SlotSendBtnClicked() {
    static int flag = 0;
    if(sender() == ui->NSendBtn) {
        if(_nStatus.dataRecycle) {
            if(!_cycleSendTimer->isActive()) {
                _cycleSendTimer->start();
            }
            flag++;
        }
    }
    if(flag == 2) {
        if(_cycleSendTimer->isActive()) {
            _cycleSendTimer->stop();
        }
        flag = 0;
        ui->NSendBtn->setText(tr("发送"));
        return ;
    }
    if(_nStatus.dataRecycle && flag == 1) {
        ui->NSendBtn->setText(tr("停止发送"));
    }
    if(!_nIsConnected) {
        QMessageBox::warning(this, QString("warning"), QString(tr("没有建立连接!")), QMessageBox::Ok);
        return ;
    }
    QString sendData = ui->NSendText->toPlainText();
    if(sendData.length() <= 0) {
        QMessageBox::warning(this, QString("warning"), QString(tr("发送区内容为空!")), QMessageBox::Ok);
        return ;
    }

    QByteArray buffer;
    if(_nStatus.sendHexCheck) {     //发送是以什么格式发送ASCII/Hex
        if(_nStatus.sendChecksum) { //是否发送校验位
            buffer = GlobalFunction::hexStrToByteArray(sendData);
            buffer = buffer + GlobalFunction::byteArrayToAsciiStr(ChecksumCompute(sendData.toLatin1())).toLatin1();
        } else {
            buffer = GlobalFunction::hexStrToByteArray(sendData);
        }
    } else if(_nStatus.sendAsciiCheck) {
        if(_nStatus.sendChecksum) {
            buffer = GlobalFunction::asciiStrToByteArray(sendData);
            buffer = buffer + ChecksumCompute(sendData.toLatin1());
        } else {
            buffer = GlobalFunction::asciiStrToByteArray(sendData);
        }
    }

    switch(_nStatus.ProtocolType) {
    case ProtocolType::UDP:
        if(_udpSocket) {
            _udpSocket->writeDatagram(buffer, QHostAddress(_udpIP.text()), _udpPort.text().toInt());
        }
        break;
    case ProtocolType::TCP_Server:
        if(_server) {
            if(_clientInfo.currentText() == "所有连接的客户端") { //服务发送消息给所有连接的客户端
                foreach(QTcpSocket* socket, _clientList.values()) {
                    socket->write(buffer);
                }
            } else { //服务发送消息给当前选中的客户端
                QString ip = _clientInfo.currentText().split(":").at(0);
                QString port = _clientInfo.currentText().split(":").at(1);
                foreach(QTcpSocket* socket, _clientList.values()) {
                    if(socket->peerAddress().toString() == ip && port.toInt() == socket->peerPort()) {
                        socket->write(buffer);
                        break;
                    }
                }
            }
        }
        break;
    case ProtocolType::TCP_Client:
        if(_client) {   //客户端发送消息
            _client->write(buffer);
        }
        break;
    }
    _sendCount += sendData.length();
    ui->NSendCount->setText(tr("发送：") + QString::number(_sendCount));
    if(_nStatus.autoSendClear) {
        ui->NSendText->clear();
    }
}

void NetworkWidget::SlotClearInputBtnClicked() { //清空输入
    ui->NSendText->clear();
}

void NetworkWidget::SlotSaveDataBtnClicked() {
    QString msgData = ui->NReceiveText->toPlainText();
    if(msgData.length() <= 0) {
        QMessageBox::warning(this, QString("warning"), QString(tr("没有内容可以保存!")), QMessageBox::Ok);
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

void NetworkWidget::SlotFileLoadBtnClicked() {
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
    ui->NSendText->setText(fileMsg);
}

void NetworkWidget::SlotReadData() {
    switch(_nStatus.ProtocolType) {
    case ProtocolType::UDP:
        UdpReadData();
        break;
    case ProtocolType::TCP_Client:
        TcpClientReadData();
        break;
    }
}

void NetworkWidget::SlotNewConnection(qintptr handle, QTcpSocket* socket) {
    _clientList.insert(handle, socket);
    _clientInfo.insertItem(_clientCount, socket->peerAddress().toString() + ":" + QString::number(socket->peerPort()));
    _clientInfo.setCurrentIndex(_clientCount);
    _clientCount++;
    ui->NInfoLabel->setText(QString(tr("共%1个客户端接入")).arg(_clientCount));
}

void NetworkWidget::SlotClientDisConnected(qintptr handle) {
    removeClient(handle);
    ui->NInfoLabel->setText(QString(tr("共%1个客户端接入")).arg(_clientCount));
}
void NetworkWidget::SlotServerReadData(qintptr handle, const QByteArray &readData) {
    if(readData.length() <= 0) {
        return;
    }
    _receiveCount += readData.length();
    if(_nStatus.turnToFile) {
        if(_saveFile) {
            if(_saveFile->isOpen()) {
                _saveFile->write(readData.data());
            }
        }
    } else {
        if(_nStatus.stopRecvFlag) {
            ui->NReceiveCount->setText(tr("接收：") + QString::number(_receiveCount));
            return;
        }
        QTcpSocket* socket = _clientList.value(handle);
        if(_lastSocket != socket) {
            _lastSocket = socket;
            ui->NReceiveText->insertPlainText("\r[from " + socket->peerAddress().toString()
                                              + ":" + QString::number(socket->peerPort()) + "] ");
        }
        QString buffer;
        if(_nStatus.showHexCheck) {
            buffer = GlobalFunction::byteArrayToHexStr(readData);
        } else if(_nStatus.showAsciiCheck) {
            buffer = GlobalFunction::byteArrayToAsciiStr(readData);
        }
        if(_nStatus.autoEnter) {
            ui->NReceiveText->insertPlainText(buffer + "\n");
        } else {
            ui->NReceiveText->insertPlainText(buffer);
        }
        ui->NReceiveText->moveCursor(QTextCursor::End);
    }
    ui->NReceiveCount->setText(tr("接收：") + QString::number(_receiveCount));
}

void NetworkWidget::SlotShowDataStop() {
    if(ui->NPause->isChecked()) {
        ui->NReceiveText->setEnabled(false);
    } else {
        ui->NReceiveText->setEnabled(true);
    }
}

void NetworkWidget::SlotProtocolTypeChange(int) {
    switch(ui->ProtocolType->currentIndex()) {
    case 0:
        ui->NConnectBtn->setText(tr("连接"));
        ui->IPLabel->setText(tr("本地IP地址"));
        ui->PortLabel->setText("本地端口号");
        break;
    case 1:
        ui->NConnectBtn->setText(tr("开始监听"));
        ui->IPLabel->setText(tr("本地IP地址"));
        ui->PortLabel->setText(tr("本地端口号"));
        break;
    case 2:
        ui->NConnectBtn->setText(tr("连接"));
        ui->IPLabel->setText(tr("服务端IP地址"));
        ui->PortLabel->setText(tr("服务端端口号"));
        break;
    }
}

void NetworkWidget::SlotNShowModeChange() {
    _rIsHex = !_rIsHex;
    if(_rIsHex) {
        ui->NShowinAscii->setEnabled(false);
        ui->NShowinHex->setEnabled(true);
        ui->NShowinHex->setChecked(true);
    } else {
        ui->NShowinHex->setEnabled(false);
        ui->NShowinAscii->setEnabled(true);
        ui->NShowinAscii->setChecked(true);
    }
}
void NetworkWidget::SlotNSendModeChange() {
    _sIsHex = !_sIsHex;
    if(_sIsHex) {
        ui->NSendinAscii->setEnabled(false);
        ui->NSendinHex->setEnabled(true);
        ui->NSendinHex->setChecked(true);
        ui->NSendText->setText(GlobalFunction::byteArrayToHexStr(ui->NSendText->toPlainText().toLatin1()));
    } else {
        ui->NSendinHex->setEnabled(false);
        ui->NSendinAscii->setEnabled(true);
        ui->NSendinAscii->setChecked(true);
        ui->NSendText->setText(GlobalFunction::hexStrToByteArray(ui->NSendText->toPlainText()));
    }
}
