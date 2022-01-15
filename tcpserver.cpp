#include "tcpserver.h"

TcpSocket::TcpSocket(qintptr handle)
{
   _handle = handle;
   this->setSocketDescriptor(_handle);
   connect(this,SIGNAL(disconnected()),this,SLOT(SlotDisConnected()));
   connect(this,SIGNAL(readyRead()),this,SLOT(SlotReadData()));
}

TcpSocket::~TcpSocket()
{
    _handle = 0;
}

void TcpSocket::SlotDisConnected()
{
    emit SignalDisConnect(_handle);
}

void TcpSocket::SlotReadData()
{
    emit SignalReadData(_handle,readAll());
}

TcpServer::TcpServer()
{
    _timer = new QTimer(this);
    connect(_timer,SIGNAL(timeout()),this,SLOT(SlotTimeout()));

}

TcpServer::~TcpServer()
{
    if(_timer){
        _timer->stop();
        delete _timer;
        _timer = nullptr;
    }
    _clients.clear();
}

void TcpServer::incomingConnection (qintptr handle)
{
    TcpSocket *socket = new TcpSocket(handle);

    connect(socket,SIGNAL(SignalDisConnect(qintptr)),this,SLOT(SlotClientDisConnected(qintptr)));
    connect(socket,SIGNAL(SignalReadData(qintptr,const QByteArray&)),this,SLOT(SlotReadData(qintptr,const QByteArray&)));

    _clients.insert(handle,socket);
    _timer->setInterval(1000);
    if(!_timer->isActive()){
        _timer->start();
    }
    emit SignalClientConnected(handle,socket);
    QString currentTime = "time:" + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss.zzz");
    socket->write( currentTime.toLatin1());
}

void TcpServer::SlotClientDisConnected(qintptr handle)
{
    _clients.remove(handle);
    emit SignalClientDisConnected(handle);
}

void TcpServer::SlotReadData(qintptr handle, const QByteArray &data)
{
    emit SignalReadData(handle,data);
}

void TcpServer::SlotTimeout()
{
    foreach (TcpSocket* socket,_clients.values()) {
        QString currentTime = "time:" + QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss.zzz");
        socket->write( currentTime.toLatin1());
    }
}

