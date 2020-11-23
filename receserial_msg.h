#ifndef RECESERIAL_MSG_H
#define RECESERIAL_MSG_H

#include <QObject>
#include"globaldata.h"
#include<qsettings.h>






class receSerial_msg : public QObject
{
    Q_OBJECT
public:
    explicit receSerial_msg(QObject *parent = 0);

    QSerialPort *serial;

    QByteArray m_buffer;

    bool isTranslateFlag;     //解析数据 还是直接显示16进制的 切换标识 true：则对数据进行解析

    vector<double> StatisticData_vector;     //统计相关  均值方差

    QStringList   DistanceStr;               //显示tof peak相关

    bool clearFlag;

    int totallen;


    /////////////////////////
    //统计相关
    vector<double> StatisticLSB_vector;     //统计相关  LSB均值方差
    vector<double> StatisticMM_vector;      //          MM均值方差
    vector<double> StatisticPeak_vector;    //          peak均值
    vector<double> Statistic_decetionRate_vector;

    //检出率相关
    float detection_minOffset;
    float detection_maxOffset;


    QStringList   vangogh_DistanceStr;               //显示tof peak相关
    int statisticPoint_number;    //统计点的个数
    int confidence_offset;        //置信度阈值

    /******计算confidence 以及 dmax 相关**********/
    float C1;
    float C2;
    float C3;
    float R0;
    float row0;
    float row;
    float P0;

    float IT;
    float IT0;

    ///////////相机校正相关///////

    float A1_;
    float B1_;
    float C1_;
    float peakThreshold_;
    float D1_;
    float E1_;
    float C2_;
    float offset_;
    float MA_;


    bool isSaveFlag;
    int saveDistance;


signals:
    void dealedData_signal(QString,vector<double>,vector<double>);     //当前的tof值 ; plotData ; statisticData

    void showResultMsg_signal(QStringList,int);  //显示tof peak相关  主界面显示;  主线程中设定一个暂存变量，每秒钟在result窗口中显示append(),然后清空result  ,该包数据点的个数

    void returnLinkInfo_signal(QString, bool);


    void AckCmd_MainWindow_signal(QString,QString);
    //!
    //! \brief AckCmdUpgrade_signal
    //!//升级相关的信号，参数为两个，1：“86”：开始升级命令应答    参数2： “01”：成功
    //!                                                            “00” ：失败
    //!                                                            其他：命令有误
    //!                              “87”：升级过程中的命令应答 参数2： 应答的数据
    void AckCmdUpgrade_signal(QString,QString);


    void Disploy_log_signal(QString);

    void toSendStatistic_signal(int,vector<double>,vector<double>,vector<double>,vector<double>);     //distance,tof ,MM的容器,peak,检出率的容器

    void toShow_vangogh_ResultMsg_signal(QStringList,int);

public slots:
    void readDataSlot();

    bool msgCheck(QString msg);

    void openOrCloseSerial_slot(bool);

    void sendSerialSlot(QString);              //串口发送数据的槽函数

    QString addCheck(QString);

    QByteArray StringToByte(QString str);      //将QString 转换为 Byte的槽函数

    void startGetStatisticData_slot(int );   //距离 mm





};

#endif // RECESERIAL_MSG_H
