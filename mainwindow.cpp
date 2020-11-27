#include "mainwindow.h"
#include "ui_mainwindow.h"


extern Settings currentSettings;
extern calibrationModel calibration_model;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    isLinked = true;

//    ui->groupBox_9->setVisible(false);

    qRegisterMetaType<vector<float>>("vector<float>");   //注册函数
    qRegisterMetaType<vector<double>>("vector<double>");   //注册函数
    qRegisterMetaType<vector<int>>("vector<int>");       //注册函数
    qRegisterMetaType<QVector<double>>("QVector<double>");   //注册函数
    qRegisterMetaType<QVector<QString>>("QVector<QString>");   //注册函数


    receSerial_Obj = new receSerial_msg;;
    receSerialThread = new QThread;
    receSerial_Obj->moveToThread(receSerialThread);
    receSerialThread->start();

    initSerial();  //串口的初始化
    initConnect();
    initLanguage();

    //从本地文件读取标定的距离  默认是300 距离
    QSettings paraSetting("parameters.ini", QSettings::IniFormat);
    calibrationDistance = paraSetting.value("OTPcalibration/calibrationDistance").toString().toInt();
    if(calibrationDistance<=0)
    {
        calibrationDistance = 300;
        paraSetting.setValue("OTPcalibration/calibrationDistance",QString::number(calibrationDistance));
    }
    int moduleIndex = paraSetting.value("calibration/chipNum").toString().toInt();
    if(moduleIndex<=0)
    {
        moduleIndex = 0;

    }
    ui->moduleNum_lineEdit->setText(QString::number(moduleIndex));


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initLanguage()
{
    m_speech = new QTextToSpeech(this);
    m_speech->setVolume(1);
    //    m_speech->setPitch(1);
    m_speech->setRate(0.2);

    QString strMsg = QStringLiteral("A T E 测试程序已经启动！");
    m_speech->say(strMsg);
}


//!\
//! 信号与槽的连接
void MainWindow::initConnect()
{
    //接收数据线程 接收并处理数据后，将处理结果发送给主线程的信号与槽
    connect(this,SIGNAL(openOrCloseSerial_signal(bool)),receSerial_Obj,SLOT(openOrCloseSerial_slot(bool)));
    connect(receSerial_Obj,SIGNAL(returnLinkInfo_signal(QString, bool)),this,SLOT(returnLinkInfo_slot(QString, bool)));
    connect(receSerial_Obj,&receSerial_msg::AckCmd_MainWindow_signal,this,&MainWindow::AckCmd_MainWindow_slot);
    connect(this,&MainWindow::sendSerialSignal,receSerial_Obj,&receSerial_msg::sendSerialSlot);
    connect(receSerial_Obj,&receSerial_msg::Disploy_log_signal,this,&MainWindow::Disploy_log);
    connect(receSerial_Obj,&receSerial_msg::toSendStatistic_signal,this,&MainWindow::toSendStatistic_slot);
    connect(this,&MainWindow::startGetStatisticData_signal,receSerial_Obj,&receSerial_msg::startGetStatisticData_slot);

}

//!
//! \brief MainWindow::SerialSetting_Enable_true
//! 使能串口的标签
void MainWindow::SerialSetting_Enable_true()
{
    ui->serialPortInfoListBox->setEnabled(true);
    ui->baudRateBox->setEnabled(true);
    ui->dataBitsBox->setEnabled(true);
    ui->parityBox->setEnabled(true);
    ui->stopBitsBox->setEnabled(true);
}

//!
//! \brief MainWindow::SerialSetting_Enable_false
//!
void MainWindow::SerialSetting_Enable_false()
{
    ui->serialPortInfoListBox->setEnabled(false);
    ui->baudRateBox->setEnabled(false);
    ui->dataBitsBox->setEnabled(false);
    ui->parityBox->setEnabled(false);
    ui->stopBitsBox->setEnabled(false);
}



//!
//! \brief MainWindow::initSerial
//!串口的初始化
void  MainWindow::initSerial()
{
    QFile file("setting.ini");
    QByteArray temp("\r\n");
    QString line[20];

    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream in(&file);
        int i = 0;
        while (!in.atEnd())
        {
            line[i] = in.readLine();
            i++;
        }
        file.close();
    }
    int numSeri_ = line[0].toInt();       //串口号
    int baudRateBox_ = line[1].toInt();   //波特率

    int num = 0;
    QStringList m_serialPortName;
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        num++;
        m_serialPortName << info.portName();
        qDebug()<<"serialPortName:"<<info.portName();
    }
    ui->serialPortInfoListBox->clear();
    ui->serialPortInfoListBox->addItems(m_serialPortName);
    if(numSeri_>num)
        ui->serialPortInfoListBox->setCurrentIndex(0);
    else
        ui->serialPortInfoListBox->setCurrentIndex(numSeri_);



    ui->baudRateBox->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->baudRateBox->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->baudRateBox->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->baudRateBox->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->baudRateBox->addItem(QStringLiteral("230400"), QSerialPort::Baud230400);
    ui->baudRateBox->addItem(QStringLiteral("256000"), QSerialPort::Baud256000);
    ui->baudRateBox->addItem(QStringLiteral("460800"), QSerialPort::Baud460800);

    ui->baudRateBox->addItem(tr("Custom"));
    ui->baudRateBox->setCurrentIndex(baudRateBox_);

    ui->dataBitsBox->addItem(QStringLiteral("5"), QSerialPort::Data5);
    ui->dataBitsBox->addItem(QStringLiteral("6"), QSerialPort::Data6);
    ui->dataBitsBox->addItem(QStringLiteral("7"), QSerialPort::Data7);
    ui->dataBitsBox->addItem(QStringLiteral("8"), QSerialPort::Data8);
    ui->dataBitsBox->setCurrentIndex(3);

    ui->parityBox->addItem(tr("None"), QSerialPort::NoParity);
    ui->parityBox->addItem(tr("Even"), QSerialPort::EvenParity);
    ui->parityBox->addItem(tr("Odd"), QSerialPort::OddParity);
    ui->parityBox->addItem(tr("Mark"), QSerialPort::MarkParity);
    ui->parityBox->addItem(tr("Space"), QSerialPort::SpaceParity);

    ui->stopBitsBox->addItem(QStringLiteral("1"), QSerialPort::OneStop);
#ifdef Q_OS_WIN
    ui->stopBitsBox->addItem(tr("1.5"), QSerialPort::OneAndHalfStop);
#endif
    ui->stopBitsBox->addItem(QStringLiteral("2"), QSerialPort::TwoStop);
}

void MainWindow::on_portScan_pushButton_clicked()
{
    QStringList m_serialPortName;
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        m_serialPortName << info.portName();
        qDebug()<<"serialPortName:"<<info.portName();
    }
    ui->serialPortInfoListBox->clear();
    ui->serialPortInfoListBox->addItems(m_serialPortName);
    QMessageBox::information(NULL,QStringLiteral("提示"),QStringLiteral("可用端口检测完毕！"));
}


//!
//! \brief MainWindow::on_openPort_pushButton_clicked
//! 打开串口
void MainWindow::on_openPort_pushButton_clicked()
{
    if(ui->openPort_pushButton->text() == "Open")
    {
        currentSettings.name = ui->serialPortInfoListBox->currentText();

        if (ui->baudRateBox->currentIndex() == 4) {
            currentSettings.baudRate = ui->baudRateBox->currentText().toInt();
        } else {
            currentSettings.baudRate = static_cast<QSerialPort::BaudRate>(
                        ui->baudRateBox->itemData(ui->baudRateBox->currentIndex()).toInt());
        }


        qDebug()<<" currentIndex ="<<ui->baudRateBox->currentIndex() <<" currentSettings.baudRate = "<<currentSettings.baudRate;
        currentSettings.stringBaudRate = QString::number(currentSettings.baudRate);

        currentSettings.dataBits = static_cast<QSerialPort::DataBits>(
                    ui->dataBitsBox->itemData(ui->dataBitsBox->currentIndex()).toInt());
        currentSettings.stringDataBits = ui->dataBitsBox->currentText();

        currentSettings.parity = static_cast<QSerialPort::Parity>(
                    ui->parityBox->itemData(ui->parityBox->currentIndex()).toInt());
        currentSettings.stringParity = ui->parityBox->currentText();

        currentSettings.stopBits = static_cast<QSerialPort::StopBits>(
                    ui->stopBitsBox->itemData(ui->stopBitsBox->currentIndex()).toInt());
        currentSettings.stringStopBits = ui->stopBitsBox->currentText();

        //         qDebug()<<"name="<<currentSettings.name<<" baudRate ="<<currentSettings.baudRate<<" dataBits="<<currentSettings.dataBits<<" parity="<<currentSettings.parity<<" stopBits="<<currentSettings.stopBits<<" flowCon"<<currentSettings.flowControl;


        emit openOrCloseSerial_signal(true);

        qDebug()<<"name="<<currentSettings.name<<" baudRate ="<<currentSettings.baudRate<<" dataBits="<<currentSettings.dataBits<<" parity="<<currentSettings.parity<<" stopBits="<<currentSettings.stopBits<<" flowCon"<<currentSettings.flowControl;


        QFile file("setting.ini");
        QByteArray temp("\r\n");
        QString line[20];

        if (file.open(QIODevice::ReadOnly))
        {
            QTextStream in(&file);
            int i = 0;
            while (!in.atEnd())
            {
                line[i] = in.readLine();
                i++;
            }
            file.close();
        }

        int seriNum = ui->serialPortInfoListBox->currentIndex();
        int baudBox = ui->baudRateBox->currentIndex();


        if(file.open(QIODevice::WriteOnly))
        {
            QByteArray writeData;
            writeData = QString::number(seriNum).toLatin1()+ temp + QString::number(baudBox).toLatin1()+temp+\
                    line[2].toLatin1()+ temp +line[3].toLatin1()+ temp+ line[4].toLatin1()+ temp +line[5].toLatin1();
            if (-1 == file.write(writeData))
            {
                qDebug()<<"ERROR";
            }
            file.close();
        }

    }
    else
    {
        emit openOrCloseSerial_signal(false);

    }
}



//!
//! \brief MainWindow::returnLinkInfo_slot
//! \param str
//! \param flag
//! 串口连接的返回信息
void MainWindow::returnLinkInfo_slot(QString str, bool flag)
{
    if("open" == str)
    {
        if(true == flag)
        {
            isLinked = true;
            ui->openPort_pushButton->setText("Close");
            SerialSetting_Enable_false();
        }else
        {
            QMessageBox::critical(this, QStringLiteral("告警"), QStringLiteral("打开串口失败！"));
        }
    }else
    {
        if(true == flag)
        {
            isLinked = false;
            ui->openPort_pushButton->setText("Open");
            SerialSetting_Enable_true();

        }else
        {
            QMessageBox::critical(this, QStringLiteral("告警"), QStringLiteral("关闭串口失败！"));
        }
    }
}


//!到达
//! \brief MainWindow::AckCmd_MainWindow_signal
//!    PLC 返回 ： 8170     AckINfo : 04 复位   05 ：p1   06:p2,  07:p3
//!
void MainWindow::AckCmd_MainWindow_slot(QString returnCmd,QString AckInfo)
{
    if("8170" == returnCmd)
    {
        int flagInt = AckInfo.toInt(NULL,16);
        if(flagInt == 4)                          // 复位完车,下一步发送 自检命令
        {
            QString strMsg = QStringLiteral("PLC设置复位完成，下一步发送自检命令!");
            Disploy_log(strMsg);

            QString cmdStr = "5A 01 00 00 80 01";
            emit sendSerialSignal(cmdStr);
            m_speech->say(strMsg);

        }else if(flagInt == 5)                    //P1 到达
        {
            QString strMsg = QStringLiteral("PLC已经到达100mm位置,下一步开启采集命令！");     //同时发送开启采集100mm数据的命令
            Disploy_log(strMsg);

            QString cmdStr = "5A 01 00 00 84 01 " ;
            emit sendSerialSignal(cmdStr);

            emit startGetStatisticData_signal(p1);
            m_speech->say(strMsg);


        }else if(flagInt == 6)                    // p2 到达
        {
            QString strMsg = QStringLiteral("PLC已经到达300mm位置，下一步开启RCO标定！");
            Disploy_log(strMsg);

            QString cmdStr = "5A 01 00 00 81 01 ";    //下一步开启RCO标定
            emit sendSerialSignal(cmdStr);
            m_speech->say(strMsg);

        }else if(flagInt == 7)                    // p3 到达
        {
            QString strMsg = QStringLiteral("PLC已经到达500mm位置,下一步开启采集命令！");     //同时发送开启采集100mm数据的命令
            Disploy_log(strMsg);

            m_speech->say(strMsg);

            emit startGetStatisticData_signal(p3);
        }else if(flagInt == 0)
        {
            QString strMsg = QStringLiteral("PLC设置失败！");
            Disploy_log(strMsg);
            m_speech->say(strMsg);
        }
    }else if("8171" == returnCmd)
    {
        if(calibration_model == normal_model)   //正常工作模式下
        {
            QString strMsg = QStringLiteral("PLC已经到达位置，下一步开启RCO标定！")+QString::number(calibrationDistance)+"mm";
            Disploy_log(strMsg);

            QString cmdStr = "5A 01 00 00 81 01 ";    //下一步开启RCO标定
            emit sendSerialSignal(cmdStr);
            m_speech->say(strMsg);
        }else
        {
            QString strMsg = QStringLiteral("PLC已经到达位置")+QString::number(calibrationDistance)+"mm";
            m_speech->say(strMsg);
        }

    }
    else if("8180" == returnCmd )   // 初始化返回命令
    {
        int flagInt = AckInfo.toInt(NULL,16);
        if(1 == flagInt)  //初始化成功
        {
            //            QString strMsg = QStringLiteral("设备自检完成，下一步发送300mm命令!");
            //            Disploy_log(strMsg);
            //            m_speech->say(strMsg);

            //            QString cmdStr = "5A 01 00 00 70 06 ";      // 下一步发送 移动至 P2 的距离
            //            emit sendSerialSignal(cmdStr);

            QString strMsg = QStringLiteral("设备自检完成，下一步发送写入设备序列号的命令!");
            Disploy_log(strMsg);
            m_speech->say(strMsg);

            int XuliehaoInt = ui->moduleNum_lineEdit->text().toInt();
            QString xuliehaoStr_tmp = QString("%1").arg(XuliehaoInt,4,16,QChar('0'));
            QString xuliehaoStr = xuliehaoStr_tmp.mid(2,2) + xuliehaoStr_tmp.mid(0,2);

            QString cmdStr = "5A 01 03 00 85";
            cmdStr.append(xuliehaoStr);
            emit sendSerialSignal(cmdStr);

        }else   //初始化失败
        {
            QString strMsg = QStringLiteral("设备自检失败！");
            Disploy_log(strMsg);
            m_speech->say(strMsg);
        }
    }else if("8181" == returnCmd)                     //开启RCO标定的返回指令
    {
        if(AckInfo.isEmpty())                         //数据区为空，则表示标定失败
        {
            QString strMsg = QStringLiteral("RCO标定失败！") ;
            Disploy_log(strMsg);
            m_speech->say(strMsg);
        }else                                        //OTP返回的寄存器值 需保存
        {
            //            AckInfo   的值需要保存至本地
            ui->RCO_lineEdit->setText(AckInfo);   //显示RCO标定结果
            QStringList strList;
            strList<<QStringLiteral("RCO标定结果")<<AckInfo;
            writeTocsv(saveFilelPath,strList);
            //保存至本地文件

            QString strMsg = QStringLiteral("RCO标定完成！下一步发送下载标定固件的命令!") ;
            Disploy_log(strMsg);

            QString cmdStr = "5A 01 00 00 82 01";    //下载标定固件的命令
            emit sendSerialSignal(cmdStr);
            m_speech->say(strMsg);

        }
    }else if("8182" == returnCmd)                     //下载标定固件后
    {
        calibration_model = offset_model;    //处于 offset 标定的模式下

        if(AckInfo.isEmpty())                         //数据区为空，则表示
        {
            QString strMsg = QStringLiteral("下载标定固件失败！");
            Disploy_log(strMsg);
            m_speech->say(strMsg);

        }else                                        //OTP返回的寄存器值 需保存
        {
            int len = AckInfo.size();
            if(len!=12)
            {
                QMessageBox::information(NULL,QStringLiteral("警告"),QStringLiteral("OTP标定结果返回的值长度有误！"));
                return;
            }

            //            ui->OTP_lineEdit->setText(AckInfo);
            QStringList strList;
            strList<<QStringLiteral("OTP标定结果")<<AckInfo.mid(0,2)<<AckInfo.mid(2,2)<<AckInfo.mid(4,2)
                  <<AckInfo.mid(6,2)<<AckInfo.mid(8,2)<<AckInfo.mid(10,2);
            writeTocsv(saveFilelPath,strList);

            ui->OTP_lineEdit_1->setText(AckInfo.mid(0,2));
            ui->OTP_lineEdit_2->setText(AckInfo.mid(2,2));
            ui->OTP_lineEdit_3->setText(AckInfo.mid(4,2));
            ui->OTP_lineEdit_4->setText(AckInfo.mid(6,2));
            ui->OTP_lineEdit_5->setText(AckInfo.mid(8,2));
            ui->OTP_lineEdit_6->setText(AckInfo.mid(10,2));



            //保存至本地
            QString strMsg = QStringLiteral("标定已经完成完成，下一步发送断电重启的命令！");
            Disploy_log(strMsg);

            QString cmdStr = "5A 01 00 00 86 01";
            emit sendSerialSignal(cmdStr);
            m_speech->say(strMsg);

        }

    }else if("8183" == returnCmd)
    {

        int flagInt = AckInfo.toInt(NULL,16);
        if(1 == flagInt)                             //成功
        {
            QString strMsg = QStringLiteral("下载测距固件1完成！下一步开启测距的命令！");
            Disploy_log(strMsg);

            QString cmdStr = "5A 01 00 00 84 01";
            emit sendSerialSignal(cmdStr);
            m_speech->say(strMsg);


            emit startGetStatisticData_signal(calibrationDistance);

        }else                                        //失败
        {
            QString strMsg = QStringLiteral("下载测距固件失败！");
            Disploy_log(strMsg);
            m_speech->say(strMsg);
        }
    }else if("8185" == returnCmd)
    {
        QString xuleihaoStr = AckInfo;
        qDebug()<<QStringLiteral("序列号 = ")<<xuleihaoStr;

        QString strMsg = QStringLiteral("序列号已经写入完成，下一步发送命令:") + QString::number(calibrationDistance) +"mm";
        Disploy_log(strMsg);
        m_speech->say(strMsg);

        QString cmdStr = "5A 01 00 00 71 ";

        QString firstStr = QString("%1").arg((calibrationDistance+4)/100,2,10,QChar('0'));
        QString secondStr = QString("%1").arg((calibrationDistance+4)%100/10,2,10,QChar('0'));
        QString thirdStr = QString("%1").arg((calibrationDistance+4)%10,2,10,QChar('0'));

        cmdStr.append(firstStr);
        cmdStr.append(secondStr);
        cmdStr.append(thirdStr);
        emit sendSerialSignal(cmdStr);


        //        QString cmdStr = "5A 01 00 00 70 06 ";      // 下一步发送 移动至 P2 的距离
        //        emit sendSerialSignal(cmdStr);



    }else if("8085" == returnCmd)
    {
        QString xuliehaoStr = AckInfo.mid(2,2) + AckInfo.mid(0,2);
        int xuliehaoInt = xuliehaoStr.toInt(NULL,16);
        ui->xuliehaoRes_lineEdit->setText(QString::number(xuliehaoInt));

    }else if("8186" == returnCmd)    //断电重启命令已经完成
    {
        if(calibration_model == offset_model)
        {
            QString strMsg = QStringLiteral("断电重启命令已经完成,下一步发送下载测距固件1的命令!");
            Disploy_log(strMsg);
            m_speech->say(strMsg);

            QString cmdStr = "5A 01 02 00 83 01";
            emit sendSerialSignal(cmdStr);
        }else if(calibration_model == test_model)
        {
            QString strMsg = QStringLiteral("复测1模式下：断电重启命令已经完成,下一步发送下载测距固件2的命令!");
            Disploy_log(strMsg);
            m_speech->say(strMsg);

            QString cmdStr = "5A 01 02 00 88 01";
            emit sendSerialSignal(cmdStr);
        }else if(calibration_model == test_2_model)
        {
            QString strMsg = QStringLiteral("复测2模式下：断电重启命令已经完成,下一步发送下载测距固件2的命令!");
            Disploy_log(strMsg);
            m_speech->say(strMsg);

            QString cmdStr = "5A 01 02 00 88 01";
            emit sendSerialSignal(cmdStr);
        }else if(calibration_model == manual_test_model)
        {
            QString strMsg = QStringLiteral("手动复测模式下：断电重启命令已经完成,下一步发送下载测距固件2的命令!");
            Disploy_log(strMsg);
            m_speech->say(strMsg);

            QString cmdStr = "5A 01 02 00 88 01";
            emit sendSerialSignal(cmdStr);
        }


    }else if("8187" == returnCmd)
    {
        calibration_model = test_model;

        QString strMsg = QStringLiteral("offset写入已经完成 ，下一步发送断电重启的命令!");
        Disploy_log(strMsg);
        m_speech->say(strMsg);

        QString cmdStr = "5A 01 02 00 86 01";
        emit sendSerialSignal(cmdStr);

    }else if("8188" == returnCmd)
    {
        int flagInt = AckInfo.toInt(NULL,16);
        if(1 == flagInt)                             //成功
        {
            QString strMsg = QStringLiteral("下载测距固件2完成！下一步开启测距的命令！");
            Disploy_log(strMsg);

            QString cmdStr = "5A 01 00 00 84 01";
            emit sendSerialSignal(cmdStr);
            m_speech->say(strMsg);

            emit startGetStatisticData_signal(calibrationDistance);


        }else                                        //失败
        {
            QString strMsg = QStringLiteral("下载测距固件失败！");
            Disploy_log(strMsg);
            m_speech->say(strMsg);
        }


    }
}

//!\
//! 显示日志
void  MainWindow::Disploy_log(QString msg)
{
    //    m_speech->say(msg);
    QDateTime currentTime = QDateTime::currentDateTime();
    QString logStr = currentTime.toString("yyyy-MM-dd hh:mm:ss");

    logStr.append("    ").append(msg);
    ui->log_textEdit->append(logStr);

    ui->log_textEdit->verticalScrollBar()->setValue(ui->log_textEdit->verticalScrollBar()->maximum());

}



/**********************************下面就是自动化部分************************************************/


void MainWindow::on_selfCheck_pushButton_clicked()
{
    QString cmdStr = "5A 01 00 00 70 04 ";   //PLC控制复位
    emit sendSerialSignal(cmdStr);
}


//!
//! \brief MainWindow::toSendStatistic_slot
//!接收来自 线程的统计数据
void MainWindow::toSendStatistic_slot(int distance, vector<double> StatisticLSB_vector, vector<double> StatisticMM_vector, vector<double> StatisticPeak_vector, vector<double> detectionRate_vector)
{
    qDebug()<<"toSendStatistic_slot  distacne = "<<distance<<"  size = "<<StatisticLSB_vector.size();

    int len;
    if(calibration_model == offset_model)   //offset标定的模式下    发送移动导轨的信号,p3:500mm ;    计算相关的统计信息
    {

        //计算LSB的均值和方差
        len = StatisticLSB_vector.size();
        if(len<1)
            return;

        // 1 计算LSB的均值和方差
        LSB_mean = std::accumulate(std::begin(StatisticLSB_vector),std::end(StatisticLSB_vector),0.0)/len;
        float LSB_Accum = 0.0;
        std::for_each(std::begin(StatisticLSB_vector),std::end(StatisticLSB_vector),[&](const double d){
            LSB_Accum += (d-LSB_mean)*(d-LSB_mean);
        });
        LSB_std = sqrt(LSB_Accum/(len-1));

        ui->mean_10_label->setText(QString::number(LSB_mean));
        ui->std_10_label->setText(QString::number(LSB_std));


        // 2 毫米的均值和方差
        len = StatisticMM_vector.size();
        if(len<1)
            return;
        MM_mean = std::accumulate(std::begin(StatisticMM_vector),std::end(StatisticMM_vector),0.0)/len;

        float MM_Accum = 0.0;
        std::for_each(std::begin(StatisticMM_vector),std::end(StatisticMM_vector),[&](const double d){
            MM_Accum += (d-MM_mean)*(d-MM_mean);
        });
        MM_std = sqrt(MM_Accum/(len-1));

        ui->alterMean_10_label->setText(QString::number(MM_mean));
        ui->alterStd_10_label->setText(QString::number(MM_std));


        // peak的值
        len = StatisticPeak_vector.size();
        if(len<1)
            return;
        float Peak_mean = std::accumulate(std::begin(StatisticPeak_vector),std::end(StatisticPeak_vector),0.0)/len;
        ui->peakMean_10_label->setText(QString::number(Peak_mean));


        //极差
        float max_MM = *max_element(StatisticMM_vector.begin(),StatisticMM_vector.end());
        float min_MM = *min_element(StatisticMM_vector.begin(),StatisticMM_vector.end());
        float jicha_mm = max_MM - min_MM;
        ui->jicha_10_label->setText(QString::number(jicha_mm));

        //脉宽
        len = detectionRate_vector.size();
        if(len<1)
            return;
        MaiKuan_mean = std::accumulate(std::begin(detectionRate_vector),std::end(detectionRate_vector),0.0)/len;
        ui->maikuan_10_label->setText(QString::number(MaiKuan_mean));



        //写入 offset的值
        int offsetValue = LSB_mean - calibrationDistance;
        QString offsetStrTmp = QString("%1").arg(offsetValue,4,16,QChar('0'));
        QString offsetStr = offsetStrTmp.mid(2,2) + offsetStrTmp.mid(0,2);
        QString cmdStr = "5A 01 02 00 87";
        cmdStr.append(offsetStr);
        emit sendSerialSignal(cmdStr);
        ui->offset_10_label->setText(QString::number(offsetValue));
        QString strMsg = QStringLiteral("下一步写入offset");
        m_speech->say(strMsg);






        QStringList strList;
        strList<<QStringLiteral("offset标定结果:");
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm均值"))<<QString::number(LSB_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm方差"))<<QString::number(LSB_std);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm均值"))<<QString::number(MM_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm修正方差"))<<QString::number(MM_std);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm_peak均值"))<<QString::number(Peak_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm极差"))<<QString::number(jicha_mm);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm脉宽"))<<QString::number(MaiKuan_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QStringLiteral("offset"))<<QString::number(offsetValue);
        writeTocsv(saveFilelPath,strList);
        strList.clear();



    }else if(test_model == calibration_model)
    {
        //计算LSB的均值和方差
        len = StatisticLSB_vector.size();
        if(len<1)
            return;

        // 1 计算LSB的均值和方差
        LSB_mean = std::accumulate(std::begin(StatisticLSB_vector),std::end(StatisticLSB_vector),0.0)/len;
        float LSB_Accum = 0.0;
        std::for_each(std::begin(StatisticLSB_vector),std::end(StatisticLSB_vector),[&](const double d){
            LSB_Accum += (d-LSB_mean)*(d-LSB_mean);
        });
        LSB_std = sqrt(LSB_Accum/(len-1));

        ui->mean_50_label->setText(QString::number(LSB_mean));
        ui->std_50_label->setText(QString::number(LSB_std));


        len = StatisticMM_vector.size();
        if(len<1)
            return;
        MM_mean = std::accumulate(std::begin(StatisticMM_vector),std::end(StatisticMM_vector),0.0)/len;

        float MM_Accum = 0.0;
        std::for_each(std::begin(StatisticMM_vector),std::end(StatisticMM_vector),[&](const double d){
            MM_Accum += (d-MM_mean)*(d-MM_mean);
        });
        MM_std = sqrt(MM_Accum/(len-1));

        ui->alterMean_50_label->setText(QString::number(MM_mean));
        ui->alterStd_50_label->setText(QString::number(MM_std));


        len = StatisticPeak_vector.size();
        if(len<1)
            return;
        float Peak_mean = std::accumulate(std::begin(StatisticPeak_vector),std::end(StatisticPeak_vector),0.0)/len;
        ui->peakMean_50_label->setText(QString::number(Peak_mean));

        //极差
        float max_MM = *max_element(StatisticMM_vector.begin(),StatisticMM_vector.end());
        float min_MM = *min_element(StatisticMM_vector.begin(),StatisticMM_vector.end());
        float jicha_mm = max_MM - min_MM;
        ui->jicha_50_label->setText(QString::number(jicha_mm));

        // 脉宽
        len = detectionRate_vector.size();
        if(len<1)
            return;
        MaiKuan_mean = std::accumulate(std::begin(detectionRate_vector),std::end(detectionRate_vector),0.0)/len;
        ui->maikuan_50_label->setText(QString::number(MaiKuan_mean));


        // 切换模式 ，并重新启动电源
        calibration_model = test_2_model;
        QString cmdStr = "5A 01 02 00 86 01";
        emit sendSerialSignal(cmdStr);   //重新启动电源
        QString strMsg = QStringLiteral("下一步断电重启的命令！");
        m_speech->say(strMsg);




        QStringList strList;
        strList<<QStringLiteral("复测结果1：");
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm复测均值"))<<QString::number(LSB_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm复测方差"))<<QString::number(LSB_std);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm修正均值"))<<QString::number(MM_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm修正方差"))<<QString::number(MM_std);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm_peak均值"))<<QString::number(Peak_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm极差"))<<QString::number(jicha_mm);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm脉宽"))<<QString::number(MaiKuan_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();




    }else if(test_2_model == calibration_model)   //第二次复测的结果
    {
        //计算LSB的均值和方差
        len = StatisticLSB_vector.size();
        if(len<1)
            return;

        // 1 计算LSB的均值和方差
        LSB_mean = std::accumulate(std::begin(StatisticLSB_vector),std::end(StatisticLSB_vector),0.0)/len;
        float LSB_Accum = 0.0;
        std::for_each(std::begin(StatisticLSB_vector),std::end(StatisticLSB_vector),[&](const double d){
            LSB_Accum += (d-LSB_mean)*(d-LSB_mean);
        });
        LSB_std = sqrt(LSB_Accum/(len-1));

        ui->mean_50_label_2->setText(QString::number(LSB_mean));
        ui->std_50_label_2->setText(QString::number(LSB_std));


        len = StatisticMM_vector.size();
        if(len<1)
            return;
        MM_mean = std::accumulate(std::begin(StatisticMM_vector),std::end(StatisticMM_vector),0.0)/len;

        float MM_Accum = 0.0;
        std::for_each(std::begin(StatisticMM_vector),std::end(StatisticMM_vector),[&](const double d){
            MM_Accum += (d-MM_mean)*(d-MM_mean);
        });
        MM_std = sqrt(MM_Accum/(len-1));

        ui->alterMean_50_label_2->setText(QString::number(MM_mean));
        ui->alterStd_50_label_2->setText(QString::number(MM_std));


        len = StatisticPeak_vector.size();
        if(len<1)
            return;
        float Peak_mean = std::accumulate(std::begin(StatisticPeak_vector),std::end(StatisticPeak_vector),0.0)/len;
        ui->peakMean_50_label_2->setText(QString::number(Peak_mean));

        //极差
        float max_MM = *max_element(StatisticMM_vector.begin(),StatisticMM_vector.end());
        float min_MM = *min_element(StatisticMM_vector.begin(),StatisticMM_vector.end());
        float jicha_mm = max_MM - min_MM;
        ui->jicha_50_label_2->setText(QString::number(jicha_mm));

        // 脉宽
        len = detectionRate_vector.size();
        if(len<1)
            return;
        MaiKuan_mean = std::accumulate(std::begin(detectionRate_vector),std::end(detectionRate_vector),0.0)/len;
        ui->maikuan_50_label_2->setText(QString::number(MaiKuan_mean));

        QString strMsg = QStringLiteral("测试已经完成！");
        m_speech->say(strMsg);

        QStringList strList;
        strList<<QStringLiteral("复测结果2：");
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm复测均值"))<<QString::number(LSB_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm复测方差"))<<QString::number(LSB_std);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm修正均值"))<<QString::number(MM_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm修正方差"))<<QString::number(MM_std);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm_peak均值"))<<QString::number(Peak_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm极差"))<<QString::number(jicha_mm);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
        strList<<(QString::number(calibrationDistance)+QStringLiteral("mm脉宽"))<<QString::number(MaiKuan_mean);
        writeTocsv(saveFilelPath,strList);
        strList.clear();
    }else if(manual_test_model == calibration_model)
    {
        //计算LSB的均值和方差
        len = StatisticLSB_vector.size();
        if(len<1)
            return;

        // 1 计算LSB的均值和方差
        LSB_mean = std::accumulate(std::begin(StatisticLSB_vector),std::end(StatisticLSB_vector),0.0)/len;
        float LSB_Accum = 0.0;
        std::for_each(std::begin(StatisticLSB_vector),std::end(StatisticLSB_vector),[&](const double d){
            LSB_Accum += (d-LSB_mean)*(d-LSB_mean);
        });
        LSB_std = sqrt(LSB_Accum/(len-1));

        ui->mean_50_label->setText(QString::number(LSB_mean));
        ui->std_50_label->setText(QString::number(LSB_std));


        len = StatisticMM_vector.size();
        if(len<1)
            return;
        MM_mean = std::accumulate(std::begin(StatisticMM_vector),std::end(StatisticMM_vector),0.0)/len;

        float MM_Accum = 0.0;
        std::for_each(std::begin(StatisticMM_vector),std::end(StatisticMM_vector),[&](const double d){
            MM_Accum += (d-MM_mean)*(d-MM_mean);
        });
        MM_std = sqrt(MM_Accum/(len-1));

        ui->alterMean_50_label->setText(QString::number(MM_mean));
        ui->alterStd_50_label->setText(QString::number(MM_std));


        len = StatisticPeak_vector.size();
        if(len<1)
            return;
        float Peak_mean = std::accumulate(std::begin(StatisticPeak_vector),std::end(StatisticPeak_vector),0.0)/len;
        ui->peakMean_50_label->setText(QString::number(Peak_mean));

        //极差
        float max_MM = *max_element(StatisticMM_vector.begin(),StatisticMM_vector.end());
        float min_MM = *min_element(StatisticMM_vector.begin(),StatisticMM_vector.end());
        float jicha_mm = max_MM - min_MM;
        ui->jicha_50_label->setText(QString::number(jicha_mm));

        // 脉宽
        len = detectionRate_vector.size();
        if(len<1)
            return;
        MaiKuan_mean = std::accumulate(std::begin(detectionRate_vector),std::end(detectionRate_vector),0.0)/len;
        ui->maikuan_50_label->setText(QString::number(MaiKuan_mean));
    }
}





/*
 *  文件路径包括文件名：
 *  textList : 一行的数据
 */
void MainWindow::writeTocsv(QString filePath, QStringList textList)
{
    QFile data(filePath);
    if(data.open(QFile::WriteOnly /*| QFile::Truncate*/ | QFile::Append))        // 打开文件
    {
        QTextStream out(&data);    // 输入流
        for(int i=0; i<textList.size(); i++)
        {
            out<<textList[i]<<",";
        }
        out<<"\n";
    }
    data.close();
}





// 开启自动标定
void MainWindow::on_startCalibration_pushButton_clicked()
{
    calibration_model = normal_model;
    //清空
    ui->RCO_lineEdit->clear();
    ui->OTP_lineEdit_1->clear();
    ui->OTP_lineEdit_2->clear();
    ui->OTP_lineEdit_3->clear();
    ui->OTP_lineEdit_4->clear();
    ui->OTP_lineEdit_5->clear();
    ui->OTP_lineEdit_6->clear();
    ui->mean_10_label->setText("");
    ui->std_10_label->setText("");
    ui->alterMean_10_label->setText("");
    ui->alterStd_10_label->setText("");
    ui->peakMean_10_label->setText("");
    ui->jicha_10_label->setText("");

    ui->mean_50_label->setText("");
    ui->std_50_label->setText("");
    ui->alterMean_50_label->setText("");
    ui->alterStd_50_label->setText("");
    ui->peakMean_50_label->setText("");
    ui->jicha_50_label->setText("");

    ui->maikuan_10_label->setText("");
    ui->maikuan_50_label->setText("");
    ui->alterMean_50_label_2->setText("");
    ui->offset_10_label->setText("");




    //模组编号自动+1
    int moduleIndex = ui->moduleNum_lineEdit->text().toInt()+1;
    ui->moduleNum_lineEdit->setText(QString::number(moduleIndex));

    QString filePath = ui->filePath_lineEdit->text();
    QString moduleName = ui->moduleNum_lineEdit->text();

    if(filePath.isEmpty() || moduleName.isEmpty())
    {
        QMessageBox::warning(NULL,QStringLiteral("提示"),QStringLiteral("存储路径、模组编号不能为空"));
        return;
    }

    saveFilelPath = filePath+"\\"+moduleName+".csv";

    QString cmdStr = "5A 01 00 00 80 01";   //设备自检
    emit sendSerialSignal(cmdStr);


    //写入编号
    QSettings paraSetting("parameters.ini", QSettings::IniFormat);
    paraSetting.setValue("calibration/chipNum",QString::number(moduleIndex));

    // 产品计数+1
    int num = ui->productNum_label->text().toInt();
    ui->productNum_label->setText(QString::number(num+1));

    QStringList strList;
    strList<<QStringLiteral("编号")<<QString::number(moduleIndex);
    writeTocsv(saveFilelPath,strList);

}

void MainWindow::on_clear_pushButton_clicked()
{
    ui->log_textEdit->clear();
}

void MainWindow::on_readModuleNum_pushButton_clicked()
{
    QString cmdStr = "5A 00 03 00 85 00 00 ";
    emit sendSerialSignal(cmdStr);
}

//!
//! \brief MainWindow::on_set_SteelDistance_pushButton_clicked
//!设置导轨距离
void MainWindow::on_set_SteelDistance_pushButton_clicked()
{
    calibration_model = manual_test_model;
    int distance = ui->manualDistance_lineEdit->text().toInt();
    QString cmdStr = "5A 01 00 00 71 ";
    QString firstStr = QString("%1").arg((distance+4)/100,2,10,QChar('0'));
    QString secondStr = QString("%1").arg((distance+4)%100/10,2,10,QChar('0'));
    QString thirdStr = QString("%1").arg((distance+4)%10,2,10,QChar('0'));
    cmdStr.append(firstStr);
    cmdStr.append(secondStr);
    cmdStr.append(thirdStr);
    emit sendSerialSignal(cmdStr);


    QString strMsg = QStringLiteral("发送设置导轨距离的命令");
    m_speech->say(strMsg);

}

//!
//! \brief MainWindow::on_manual_getData_pushButton_clicked
//! 采集数据
void MainWindow::on_manual_getData_pushButton_clicked()
{
    ui->RCO_lineEdit->clear();
    ui->OTP_lineEdit_1->clear();
    ui->OTP_lineEdit_2->clear();
    ui->OTP_lineEdit_3->clear();
    ui->OTP_lineEdit_4->clear();
    ui->OTP_lineEdit_5->clear();
    ui->OTP_lineEdit_6->clear();
    ui->mean_10_label->setText("");
    ui->std_10_label->setText("");
    ui->alterMean_10_label->setText("");
    ui->alterStd_10_label->setText("");
    ui->peakMean_10_label->setText("");
    ui->jicha_10_label->setText("");

    ui->mean_50_label->setText("");
    ui->std_50_label->setText("");
    ui->alterMean_50_label->setText("");
    ui->alterStd_50_label->setText("");
    ui->peakMean_50_label->setText("");
    ui->jicha_50_label->setText("");


    calibration_model = manual_test_model;
    QString cmdStr = "5A 01 02 00 86 01";
    emit sendSerialSignal(cmdStr);

    QString strMsg = QStringLiteral("发送电源重启的命令");
    m_speech->say(strMsg);


}
