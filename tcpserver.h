#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QDateTime>
#include <QTime>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QObject>
class TcpSocket : public QTcpSocket
{
    Q_OBJECT
public:
    explicit TcpSocket(qintptr handle);
    ~TcpSocket();
signals:
    void SignalDisConnect(qintptr );
    void SignalReadData(qintptr ,const QByteArray&);
public slots:
    void SlotDisConnected();
    void SlotReadData();
private:
    qintptr _handle;

};

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    TcpServer();
    ~TcpServer();
protected:
    virtual void incomingConnection(qintptr handle)override;
public slots:
    void SlotClientDisConnected(qintptr handle);
    void SlotReadData(qintptr handle,const QByteArray& data);
    void SlotTimeout();
signals:
    void SignalClientConnected(qintptr,QTcpSocket*);
    void SignalClientDisConnected(qintptr );
    void SignalReadData(qintptr ,const QByteArray &);
private:
    QMap<qintptr,TcpSocket*> _clients;
    QTimer *_timer;
};

#endif // TCPSERVER_H
