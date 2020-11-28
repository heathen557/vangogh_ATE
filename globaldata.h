#ifndef GLOBALDATA_H
#define GLOBALDATA_H

#include <QtSerialPort/QSerialPort>
#include<QtSerialPort/QSerialPortInfo>
#include<QDebug>
#include<QDataStream>
#include<vector>
#include<QTimer>
#include<QList>
#include<QMessageBox>
#include<QThread>


#define p1 100
#define p2 300
#define p3 500


enum calibrationModel{
    normal_model,        //正常模式
    offset_model,        //offset标定
    test_model,          //第一次复测
    test_2_model,        //第二次复测
    manual_test1_model,    //手动测试1
    manual_test2_model,    //手动测试1
    manual_test3_model,    //手动测试1
    other_model
};

struct Settings {
    QString name;
    qint32 baudRate;
    QString stringBaudRate;
    QSerialPort::DataBits dataBits;
    QString stringDataBits;
    QSerialPort::Parity parity;
    QString stringParity;
    QSerialPort::StopBits stopBits;
    QString stringStopBits;
    QSerialPort::FlowControl flowControl;
    QString stringFlowControl;
    bool localEchoEnabled;
};


using namespace std;



#endif // GLOBALDATA_H


