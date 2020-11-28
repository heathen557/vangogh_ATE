#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include"globaldata.h"
#include"receserial_msg.h"
#include<QDateTime>
#include <QTextToSpeech>
#include<QScrollBar>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void initLanguage();

    void initConnect();

    //串口初始化相关
    void initSerial();
    void SerialSetting_Enable_true();
    void SerialSetting_Enable_false();


    QTextToSpeech *m_speech;

    float LSB_mean,LSB_std;
    float MM_mean,MM_std;
    float MaiKuan_mean;


    //标定距离
    int  calibrationDistance;

    //复测距离
    int reTestDistance;
private slots:

    void Disploy_log(QString);

    void on_portScan_pushButton_clicked();

    void on_openPort_pushButton_clicked();

    void returnLinkInfo_slot(QString str, bool flag);

    void AckCmd_MainWindow_slot(QString,QString);

    void on_selfCheck_pushButton_clicked();

    void toSendStatistic_slot(int,vector<double>,vector<double>,vector<double>,vector<double>);

    void writeTocsv(QString filePath,QStringList textList);

    void on_startCalibration_pushButton_clicked();

    void on_clear_pushButton_clicked();

    void on_readModuleNum_pushButton_clicked();


    void on_set_SteelDistance_pushButton_clicked();

    void on_manual_getData_pushButton_clicked();

signals:
    void openOrCloseSerial_signal(bool);

    void sendSerialSignal(QString);

    void sendSerialASIIC_signal(QString );

    void startGetStatisticData_signal(int );   //距离 mm

private:
    Ui::MainWindow *ui;

    receSerial_msg  *receSerial_Obj;      //串口接收数据线程
    QThread *receSerialThread;
    bool isLinked;   //是否连接的标识
    QString saveFilelPath;

    // manual_保存相关
    QString manualSaveFilePath;
};

#endif // MAINWINDOW_H



//!ATE自动化标定流程
//! 一、 自检按钮的命令：
//!     1、发送托盘、反光板复位
//!     2、接收到命令，表示复位完成： 发送 初始化自检模式
//!     3、接收到命令，表示自检OK；  发送 移动嗲位置 P2 ：30cm
//!     4、接收到命令，表示已经到达P1, 发送开启RCO标定
//!     5、接收到命令，并存储RCO的寄存器值， 发送 下载固件的命令（同时会进行OTP的校准）；
//!     6、接收命令并存储OTP校准值， 弹出标定完成；
//!
//!
//! 二、自动测试的命令
//!     1、发送下载测距固件的命令；
//!     2、接收到返回命令，发送 移动至P1位置：10cm
//!     3、接收到命令，发送 开启采集命令；
//!     4、接收到命令，开启采集数据（采集三十五个点），统计均值、方差、极差；
//!     5、统计结束后，发送移动至p3位置 50cm
//!     6、接收到命令后，开启采集数据（采集三十五） 统计均值、方差、极差；
//!     7、自动测试完成










