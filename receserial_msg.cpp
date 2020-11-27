#include "receserial_msg.h"

extern Settings currentSettings;

receSerial_msg::receSerial_msg(QObject *parent) : QObject(parent)
{

    qDebug()<<" the thread begin "<<endl;
    isTranslateFlag = true;
    serial = NULL;
    clearFlag = false;

    isSaveFlag = false;


    /******读取本地的配置参数  confidence 与 Dmax ******/

    QSettings paraSetting("parameters.ini", QSettings::IniFormat);
    C1 = paraSetting.value("confidence_para/C1").toString().toFloat();
    C2 = paraSetting.value("confidence_para/C2").toString().toFloat();
    C3 = paraSetting.value("confidence_para/C3").toString().toFloat();
    R0 = paraSetting.value("confidence_para/R0").toString().toFloat();
    row0 = paraSetting.value("confidence_para/row0").toString().toFloat();
    row = paraSetting.value("confidence_para/row").toString().toFloat();
    P0 = paraSetting.value("confidence_para/P0").toString().toFloat();

    IT = paraSetting.value("confidence_para/IT").toString().toFloat();
    IT0 = paraSetting.value("confidence_para/IT0").toString().toFloat();



    QString statisStr = paraSetting.value("Stastic/statis_num").toString();
    QString confidenceStr = paraSetting.value("Stastic/confidence_offset").toString();
    statisticPoint_number = statisStr.toInt();
    confidence_offset = confidenceStr.toInt();

    if(statisticPoint_number<=0)
    {
        statisticPoint_number = 50;
    }
    if(confidence_offset<=0)
    {
        confidence_offset = 50;
    }


    qDebug()<<"C1 = "<<C1<<"  C2="<<C2<<" C3="<<C3<<"  R0="<<R0<<"  row0="<<row0<<"  row="<<row<<"  p0="<<P0
           <<"statisticPoint_number= "<<statisticPoint_number<<"  confidence_offset="<<confidence_offset;





    //读取本地校准参数
    A1_ = paraSetting.value("Calibration/A1").toString().toFloat();
    B1_ = paraSetting.value("Calibration/B1").toString().toFloat();
    C1_ = paraSetting.value("Calibration/C1").toString().toFloat();
    peakThreshold_ = paraSetting.value("Calibration/peakThreshold").toString().toFloat();
    D1_ = paraSetting.value("Calibration/A2").toString().toFloat();
    E1_ = paraSetting.value("Calibration/B2").toString().toFloat();
    C2_ = paraSetting.value("Calibration/C2").toString().toFloat();
    offset_ = paraSetting.value("Calibration/offset").toString().toFloat();
    MA_ = paraSetting.value("Calibration/MA").toString().toFloat();

    qDebug()<<"A1="<<A1_<<" B1="<<B1_<<" C1="<<" tofThreadhold ="<<peakThreshold_<<C1_<<" P1="<<D1_<<" P2="<<E1_<<" OFFSET="<<offset_;



    statisticPoint_number = paraSetting.value("Statistic/statisticPoint_number").toString().toInt();
    if(statisticPoint_number <= 0)
    {
        statisticPoint_number = 35;
        paraSetting.setValue("Statistic/statisticPoint_number",QString::number(statisticPoint_number));
    }





}

void receSerial_msg::openOrCloseSerial_slot(bool flag)
{
    if(NULL == serial)
    {
        serial = new QSerialPort(this);
        connect(serial, SIGNAL(readyRead()), this, SLOT(readDataSlot()),Qt::DirectConnection);

    }

    if(true == flag)   //打开串口
    {
        serial->setPortName(currentSettings.name);
        serial->setBaudRate(currentSettings.baudRate);
        serial->setDataBits(currentSettings.dataBits);
        serial->setParity(currentSettings.parity);
        serial->setStopBits(currentSettings.stopBits);
        serial->setFlowControl(currentSettings.flowControl);
        if(serial->open(QIODevice::ReadWrite))
        {
            qDebug()<<"serial open success!!";
            emit returnLinkInfo_signal("open",true);
        }else{
            qDebug()<<"serial open error";
            emit returnLinkInfo_signal("open",false);
        }
    }else              //关闭串口
    {
        serial->close();
        emit returnLinkInfo_signal("close",true);
        m_buffer.clear();
    }

}


//!
//! \brief receSerial_msg::readDataSlot  串口接收数据，并对命令进行解析 （暂时约定传输数据为小端模式）
//!   1、首先接收到字符串 以后把它转换成16进制的字符串类型，本次处理时去掉空格；
//!   2、将接收到的数据添加到成员变量m_buffer中；
//!   3、根据长度字段将单个命令提取出来
//!   4、对单个命令进行解析
//!   5、命令解析完毕后，从m_buffer中剔除到这个命令，并更新totallen
void receSerial_msg::readDataSlot()
{
    QByteArray temp = serial->readAll();
    QString strHex;//16进制数据

    if (!temp.isEmpty())
    {
        strHex = temp.toHex();
        strHex = strHex.toUpper();
//        qDebug()<<" strHex = "<<strHex;
        //        return;

        m_buffer.append(strHex);
        totallen = m_buffer.size();

        if(isTranslateFlag)    //转换成十进制的tof和peak进行显示解析
        {
            while(totallen)
            {
                if(totallen <12)   //单个命令最少要有6个字节 所以长度为12       5A 03 00 01 3A XX
                    return;


                int indexOf5A = m_buffer.indexOf("5A",0);
                if(indexOf5A < 0)  //没有找到5A
                {
                    qDebug()<<QStringLiteral("接收数据有误，不存在5A")<<"index ="<<indexOf5A<<"buffer"<<m_buffer<<endl;
                    m_buffer.clear();
                    totallen = m_buffer.size();
                    return;
                }else if(indexOf5A>0)  //第一次的时候前面会有冗余数据，删掉
                {
                    m_buffer = m_buffer.right(totallen-indexOf5A);
                    totallen = m_buffer.size();
                    if(totallen <12)
                        return;
                }

                //               qDebug()<<"m_buffer = "<<m_buffer;
                //以下数据为5A打头数据
                //首先根据长度字段 来提取出整条数据，数据长度不足的话返回
                QString lenStr= m_buffer.mid(6,2) + m_buffer.mid(4,2);
                int dataLen = lenStr.toInt(NULL,16) * 2;       //数据区的长度  这个长度包含一个字节的 地址
                int len = dataLen + 5 * 2;                     //5A 03（命令） 04 00（长度）  + 检验位共5个字节   这个len是单个包的总长度
                if(totallen < len)                            //本次接收不够一个包,返回 等待下次接收
                    return;

                dataLen = dataLen - 2;                         //减去一个字节的 地址   2019-12-10

                //进行和校验
                QString single_Data = m_buffer.left(len);       //single_Data就是单个命令
                if(!msgCheck(single_Data))
                {
                    //                    qDebug()<<QStringLiteral("和校验失败,singleData =")<<single_Data;
                    //                    m_buffer = m_buffer.right(totallen - len);                                                  //一帧处理完毕 减去该帧的长度
                    //                    totallen = m_buffer.size();
                    m_buffer.clear();
                    totallen = 0;

                    return;
                }
                //             qDebug()<<" receive single_Data = "<<single_Data;

                QString returnCmdStr = single_Data.mid(2,2);   //命令标识
                ////////////////////////////////////////////////////////单个命令处理代码块//////////////////////////////////////////////////////////////////////////////////////////

                //! PLC控制的返回命令
                if("81" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("70" == secCmd)
                    {
                        QString cmdAck = "8170";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }
                }

                //
                if("81" == returnCmdStr)
                {
                    QString secCmd =single_Data.mid(8,2);
                    if("71" == secCmd)
                    {
                        QString cmdAck = "8171";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }


                }




                //              初始化（设备自检）命令  的返回命令
                if("81" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("80" == secCmd)
                    {
                        QString cmdAck = "8180";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }
                }

                //              开启RCO 标定命令 的返回命令
                if("81" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("81" == secCmd)
                    {
                        QString cmdAck = "8181";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }

                }

                //             下载标定固件（包含BVD、CKP_MP标定）命令 的返回命令
                if("81" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("82" == secCmd)
                    {
                        QString cmdAck = "8182";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }
                }

                //              下载测试固件1命令  的返回命令
                if("81" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("83" == secCmd)
                    {
                        QString cmdAck = "8183";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }

                }

                // 写入序列号的命令 返回命令
                if("81" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("85" == secCmd)
                    {
                        QString cmdAck = "8185";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }
                }


                //读取序列号的命令
                if("80" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("85" == secCmd)
                    {
                        QString cmdAck = "8085";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }

                }


                //断电重启成功的返回命令
                if("81"==returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("86" == secCmd)
                    {
                        QString cmdAck = "8186";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }
                }

                // offset 写入命令后的返回命令
                if("81" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("87"== secCmd)
                    {
                        QString cmdAck = "8187";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }
                }

                //下载测距固件2 的返回命令
                if("81" == returnCmdStr)
                {
                    QString secCmd = single_Data.mid(8,2);
                    if("88" == secCmd)
                    {
                        QString cmdAck = "8188";
                        QString dataStr = single_Data.mid(10,dataLen);
                        emit AckCmd_MainWindow_signal(cmdAck,dataStr);
                    }
                }


                //  开始测距命令  连续测量的返回命令
                if("81" == returnCmdStr)
                {

//                    qDebug()<<"singleData = "<<single_Data;
                    QString secCmd = single_Data.mid(8,2);
                    if("84" == secCmd)
                    {
                        QString dataStr = single_Data.mid(10,dataLen);
                        QString currentSingleData;
                        currentSingleData.clear();
                        float tmp_LSB = 0;
                        float tmp_MM = 0;
                        int pointNum = 0;

                        float tmp_peak = 0;
                        float noise_mean = 0;
                        float N1 = 0;
                        float confidence = 0 ;
                        float Dmax = 0;

                        float reference_LSB = 0;
                        float reference_MM = 0;

                        float MaiKuan_float = 0;


                        //2个字节的 tof 4个字节的peak  2个字节的脉宽
                        for(int i=0; i<dataLen; i+=16)    //6个字节  2个字节mm  4个字节peak
                        {
                            pointNum++;
                            //16进制数据转化为10进制 然后再转化成字符串
                            // 1  16进制数据转化为10进制 然后再转化成字符串
                            QString strTmp = dataStr.mid(i+2,2) + dataStr.mid(i+0,2);
                            tmp_LSB = strTmp.toInt(NULL,16);   //1

                            //这个暂时不显示
                            //                          strTmp = dataStr.mid(i+6,2) + dataStr.mid(i+4,2);
                            //                          tmp_MM = strTmp.toInt(NULL,16);    //2
                            //                          tmp_MM = k_parameter * tmp_MM +b_parameter;

                            strTmp = dataStr.mid(10,2) + dataStr.mid(8,2) + dataStr.mid(6,2) + dataStr.mid(4,2);
                            tmp_peak = strTmp.toInt(NULL,16);  //3
                            //                            strTmp = dataStr.mid(18,2) + dataStr.mid(16,2);
                            //                            noise_mean = strTmp.toInt(NULL,16); // 4
                            //                            strTmp = dataStr.mid(22,2) + dataStr.mid(20,2);
                            //                            reference_LSB = strTmp.toInt(NULL,16);  //5
                            //                            strTmp = dataStr.mid(26,2) + dataStr.mid(24,2);
                            //                            reference_MM = strTmp.toInt(NULL,16);   //6

                            strTmp = dataStr.mid(14,2) + dataStr.mid(12,2);
                            MaiKuan_float = strTmp.toInt(NULL,16);


                            //alter 2020-08-28
                            float bias_float;
                            float tmpPeak_float = tmp_peak/(1e5);
                            if(tmp_LSB<peakThreshold_)
                            {
                                bias_float = A1_*pow(tmpPeak_float,2) +B1_ * tmpPeak_float + C1_;
                            }else
                            {
                                bias_float = D1_*tmpPeak_float + E1_;
                            }


                            tmp_MM = tmp_LSB + bias_float - offset_;


                            //2 显示
                            //                            N1 = noise_mean * C1;


                            //                            //3 计算confidence
                            //                            float N2 = noise_mean * C2;
                            //                            float sigma = sqrt(C3 * noise_mean);
                            //                            if(tmp_peak<(N2+3*sigma))
                            //                            {
                            //                                confidence = 0;
                            //                            }else if(tmp_peak > (N2+6*sigma))
                            //                            {
                            //                                confidence = 63.0;
                            //                            }else
                            //                            {
                            //                                float tmpFloat = 8 * (6*sigma - tmp_peak + N2)/(3*sigma);
                            //                                confidence = (64.0 - pow(tmpFloat,2));
                            //                            }

                            //                            // 4 计算DMAX
                            //                            float tmpFloat_up = R0 * sqrt((row * P0)/(row0));
                            //                            float tmpFloat_1 = 3 + sqrt(9+4*(N2+3*sigma));
                            //                            float tmpFloat_2 = pow(tmpFloat_1/2.0,2) - N2;
                            //                            float tmpFloat_down = sqrt(tmpFloat_2);
                            //                            Dmax = tmpFloat_up/tmpFloat_down;


                            //                            currentSingleData = QString::number(tmp_LSB);             // 1 原始TOF
                            //                            currentSingleData.append("   ");
                            //                            currentSingleData.append(QString::number(tmp_MM,'f',1));   //2 校正后的TOF
                            //                            currentSingleData.append("   ");
                            //                            currentSingleData.append(QString::number(tmp_peak));       //3  peak
                            //                            currentSingleData.append("   ");
                            //                            currentSingleData.append(QString::number(N1,'f',3));       //4 噪声水平
                            //                            currentSingleData.append("Mcps   ");
                            //                            currentSingleData.append(QString::number(reference_LSB));  //5 MP个数
                            //                            currentSingleData.append("   ");
                            //                            currentSingleData.append(QString::number(int(confidence)));//6 confidence level
                            //                            currentSingleData.append("/64   ");
                            //                            currentSingleData.append(QString::number(Dmax,'f',3));     //7 DMax
                            //                            currentSingleData.append("mm   ");


                            qDebug()<<"  isSaveFlag="<<isSaveFlag<<" StatisticPeak_vector.size ="<<StatisticPeak_vector.size();

                            if(isSaveFlag)
                            {
                                //1 统计信息相关的变量存储   LSB 的存储
                                int StatisticLSB_offset = StatisticLSB_vector.size() -statisticPoint_number;
                                if(StatisticLSB_offset >= 0 )
                                {
                                    StatisticLSB_vector.erase(StatisticLSB_vector.begin(),StatisticLSB_vector.begin()+StatisticLSB_offset+1);
                                }

                                StatisticLSB_vector.push_back(tmp_LSB);



                                //2、 MM 单位的存储
                                int StatisticMM_offset = StatisticMM_vector.size() - statisticPoint_number;
                                if(StatisticMM_offset >= 0 )
                                {
                                    StatisticMM_vector.erase(StatisticMM_vector.begin(),StatisticMM_vector.begin()+StatisticMM_offset + 1);
                                }

                                StatisticMM_vector.push_back(tmp_MM);


                                //3、 peak 值得存储
                                int StatisticPeak_offset = StatisticPeak_vector.size() - statisticPoint_number;
                                if(StatisticPeak_offset>=0)
                                {
                                    StatisticPeak_vector.erase(StatisticPeak_vector.begin(),StatisticPeak_vector.begin()+StatisticPeak_offset+1);
                                }

                                StatisticPeak_vector.push_back(tmp_peak);



                                //4、 检出率相关的存储
                                int StatisticDecetion_offset = Statistic_decetionRate_vector.size() - statisticPoint_number;
                                if(StatisticDecetion_offset >= 0)
                                {
                                    Statistic_decetionRate_vector.erase(Statistic_decetionRate_vector.begin(),Statistic_decetionRate_vector.begin()+StatisticDecetion_offset+1);
                                }
                                Statistic_decetionRate_vector.push_back(MaiKuan_float);


                                int len = StatisticPeak_vector.size();
                                if(len>=statisticPoint_number)
                                {
                                    isSaveFlag = false;   //停止存储
                                    emit toSendStatistic_signal(saveDistance,StatisticLSB_vector,StatisticMM_vector,StatisticPeak_vector,Statistic_decetionRate_vector);
                                    StatisticLSB_vector.clear();
                                    StatisticMM_vector.clear();
                                    StatisticPeak_vector.clear();
                                    Statistic_decetionRate_vector.clear();

                                }


                            }

                            vangogh_DistanceStr.append(currentSingleData);

                        }

                        //                      emit toSendStatistic_signal(StatisticLSB_vector,StatisticMM_vector,StatisticPeak_vector,Statistic_decetionRate_vector);
                        //                      emit toShow_vangogh_ResultMsg_signal(vangogh_DistanceStr,pointNum);
                        vangogh_DistanceStr.clear();
                    }

                }





                ////////////////////////////////////////////////////////单个命令处理代码块//////////////////////////////////////////////////////////////////////////////////////////
                m_buffer = m_buffer.right(totallen - len);                                                  //一帧处理完毕 减去该帧的长度
                totallen = m_buffer.size();
            }
        }
        else   //直接打印16进制的数据
        {
            DistanceStr.append(m_buffer);
            emit showResultMsg_signal(DistanceStr,0);                                                   //发送用于界面显示的数据  显示TOF或者PEAK 或者16进制数据
            DistanceStr.clear();                                                                      //清空
            m_buffer.clear();
            totallen = m_buffer.size();
        }

    }
}



//字节校验 从第二个字节开始，到倒数第二个字节 求和并取反 判断是否与最后一个字节相等
bool receSerial_msg::msgCheck(QString msg)
{
    int len = msg.length();
    int i=2;
    int num = 0;
    for(;i<len-2;i+=2)
    {
        num += msg.mid(i,2).toInt(NULL,16);
    }

    int checkNum = msg.mid(i,2).toInt(NULL,16);
    if(quint8(~num) == checkNum)
    {
        return true;
    }else
    {
        return false;
    }
}


//!
//! \brief receSerial_msg::sendSerialSlot
//! \param sendCmdStr: 不带校验位的 QString 类型的字符串  QString cmdStr = "5A 03 04 00 0A 00 88 13 ";
//! 1、添加校验位
//! 2、转换为字节数,存储在QByteArray
//! 3、向串口发送数据
//! 4、清空缓存区
void receSerial_msg::sendSerialSlot(QString sendCmdStr)
{
    QByteArray sendArray;
    QString wholeStr;
    wholeStr = addCheck(sendCmdStr);     //添加校验
    sendArray = StringToByte(wholeStr);  //转换成字节数据
    if(serial!=NULL && serial->isWritable())
    {
        totallen = 0;
        m_buffer.clear();
        serial->write(sendArray);            //串口发送字节数据
        serial->flush();                     //清空缓冲区
    }
    else
    {
        qDebug()<<"can not write right now ";
    }

}


//为命令添加校验位 （会自动更新报文长度）
QString receSerial_msg::addCheck(QString str)
{
    QString tmpStr = str;
    tmpStr.replace(" ","");
    int len = tmpStr.length();
    //数据区的长度为 len-4
    int dataLen = len/2-4;

    QString dataTmp = QString("%1").arg(dataLen,4,16,QLatin1Char('0'));
    QString lenStr = dataTmp.mid(2,2)+dataTmp.mid(0,2);
    tmpStr = tmpStr.replace(4,4,lenStr);
    int sum = 0;
    for(int i = 2; i<len; i+=2)
    {
        sum += tmpStr.mid(i,2).toInt(NULL,16);
    }
    quint8 checkValue = (quint8)(~sum);
    QString resStr = tmpStr.append(QString("%1").arg(checkValue,2,16,QLatin1Char('0')));

    //    qDebug()<<"addCheck = "<<resStr;
    return resStr;
}


//! \brief receSerial_msg::StringToByte
//! \return
//! 将QString QByteArray
QByteArray receSerial_msg::StringToByte(QString str)
{
    QByteArray byte_arr;
    bool ok;
    str = str.replace(" ","");    //去掉空格键
    int len=str.size();
    for(int i=0;i<len;i+=2){
        byte_arr.append(char(str.mid(i,2).toUShort(&ok,16)));
    }
    //    qDebug()<<" byte_arr's len = "<<byte_arr.size()<<"    "<<byte_arr;

    //下面这段程序是将Byte 转化为QString的类型，将结果输出 对比发出的数据是否正确
    QDataStream out(&byte_arr,QIODevice::ReadWrite);
    QString strHex;
    while (!out.atEnd())
    {
        qint8 outChar=0;
        out>>outChar;
        QString str=QString("%1").arg(outChar&0xFF,2,16,QLatin1Char('0'));

        if (str.length()>1)
        {
            strHex+=str+" ";
        }
        else
        {
            strHex+="0"+str+" ";
        }
    }
    strHex = strHex.toUpper();
    QString msgStr =QStringLiteral("发送的原始数据为：")+strHex+"   len="+QString::number( strHex.length()/3);
    emit Disploy_log_signal(msgStr);
    return byte_arr;
}



//! \brief receSerial_msg::startGetStatisticData_slot
//!开始保存统计数据的命令
void receSerial_msg::startGetStatisticData_slot(int distance)
{
    saveDistance = distance;
    isSaveFlag = true;

}






