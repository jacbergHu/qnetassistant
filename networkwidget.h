#ifndef NETWORKWIDGET_H
#define NETWORKWIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QDebug>
#include <QTimer>
#include <QByteArray>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QNetworkInterface>
#include "tcpserver.h"
#include "globalfunction.h"
using namespace GlobalFunction;

namespace Ui {
class NetworkWidget;
}

class NetworkWidget : public QWidget
{
    Q_OBJECT

public:
    enum ProtocolType{
        UDP = 0,
        TCP_Server,
        TCP_Client
    };
    const ushort MIN_PORT = 1024;
    const ushort MAX_PORT = 65535;
public:
    explicit NetworkWidget(QWidget *parent = 0);
    ~NetworkWidget();
    void InitNetwork();
    void initIP();
    void InitUdpConnect();
    void InitTCPServerConnect();
    void TcpClientReadData();
    void UdpReadData();
    QByteArray ChecksumCompute(QString msg);
    void removeAllClient();
    void removeClient(qintptr handle);
    QVariantHash SaveConfig();
    void LoadConfig(QJsonObject &jsonData);
    void RetranslateUi();

public slots:
    void SlotChangeTimer();
    void SlotNShowModeChange();
    void SlotNSendModeChange();
    void SlotUpdateStatus();
    void SlotCheckPort();
    void SlotNReceivetoFile();
    void SlotDataFromFile();
    void SlotAutoSendAddBit();

    void SlotClearReceiveBtnClicked();
    void SlotNConnectBtnClicked();
    void SlotOnStateChange(QAbstractSocket::SocketState state);
    void SlotResetBtnClicked();
    void SlotSendBtnClicked();
    void SlotClearInputBtnClicked();
    void SlotSaveDataBtnClicked();
    void SlotFileLoadBtnClicked();

    void SlotReadData();
    void SlotNewConnection(qintptr handle, QTcpSocket *socket);
    void SlotClientDisConnected(qintptr handle);
    void SlotServerReadData(qintptr handle, const QByteArray& readData);
    void SlotShowDataStop();

    void SlotProtocolTypeChange(int);

signals:
    void NetworkReceiveData(QByteArray &data);

private:
    Ui::NetworkWidget *ui;
    TcpServer *_server;
    QTcpSocket *_client;
//    QTcpSocket *_serverSocket;
    QTcpSocket *_lastSocket;
    QUdpSocket *_udpSocket;
    struct status _nStatus;
    QMap<qintptr,QTcpSocket *> _clientList;

    bool _nIsConnected;
    QFile *_saveFile;
    QFile *_sourceFile;

    int _receiveCount = 0;
    int _sendCount = 0;
    int _clientCount = 0;

    QTimer *_timer;
    QTimer *_cycleSendTimer;
    bool _rIsHex = false;
    bool _sIsHex = false;
    volatile bool _nIsConnectBtnStatus = false;
    bool _cycleSendStart = false;
    bool _isNegative;

    QWidget *_udpWidget;
    QWidget *_serverWidget;
    QLabel _udpIPLabel;
    QLabel _udpPortLabel;
    QLabel _serverLabel;
    QLineEdit _udpIP;
    QLineEdit _udpPort;
    QComboBox _clientInfo;
};

#endif // NETWORKWIDGET_H
