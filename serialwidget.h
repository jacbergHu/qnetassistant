#ifndef SERIALWIDGET_H
#define SERIALWIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QDebug>
#include <QTimer>
#include <QByteArray>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QMessageBox>
#include <QSerialPortInfo>
#include "globalfunction.h"

namespace Ui {
class SerialWidget;
}

class SerialWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SerialWidget(QWidget *parent = 0);
    ~SerialWidget();
    void InitSignalSlot();  //初始化信号与槽
    QVariantHash SaveConfig();//保存配置
    void LoadConfig(QJsonObject &jsonData);//加载配置
    void RetranslateUi();
protected:
    void SetSerialPort();//设置串口参数
    void GetAvailablePorts(); //获取可用的串口号
    QByteArray ChecksumCompute(QString msg); //计算校验位
public slots:
    void SlotShowModeChange();  //接收区显示模式改变
    void SlotSendModeChange();  //发送区发送模式改变
    void SlotOpenBtnClicked();  //打开/关闭 串口
    void SlotClearReceiveBtnClicked();  //清空接收区
    void SlotResetBtnClicked();     //计数复位
    void SlotSendBtnClicked();      //发送按钮点击
    void SlotClearInputBtnClicked();    //清空输入
    void SlotSaveDataBtnClicked();  //保存接收区数据
    void SlotFileLoadBtnClicked();  //加载文件内容
    void SlotReadData();        //读取数据
    void SlotUpdateState();     //更新复选框状态

    void SlotReceivetoFile();   //接受内容存入文件
    void SlotDataFromFile();    //发送数据来源于文件

    void SlotAutoSendAddBit();  //自动发送检验位(附加位)
    void SlotChangeTimer(); //修改定时器
    void SlotShowDataStop();

private:
    Ui::SerialWidget *ui;
    QSerialPort *_serial;   //串口描述
    QStringList _serialPortNames;   //记录可用串口号

    QTimer *_timer;     //刷新复选框状态定时器
    QTimer *_cycleTimer;    //循环发送数据定时器

    struct status _status;  //记录复选框状态
    QFile *_saveFile;   //保存文件指针
    QFile *_sourceFile; //发送数据来源指针
    volatile bool _rIsHex;  //接收是否是Hex显示
    volatile bool _sIsHex;  //发送是否是Hex显示
    volatile bool _isNegative;  //是否发送负检验
    volatile bool _nIsConnected = false;   //是否已建立连接

    int _receiveCount = 0;  //接收计数
    int _sendCount = 0;     //发送计数
};

#endif // SERIALWIDGET_H
