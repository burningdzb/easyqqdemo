#include "widget.h"
#include "ui_widget.h"
#include<QDataStream>
#include<QMessageBox>
#include<QDateTime>
#include<QColorDialog>
#include<QFileDialog>
#include<QFile>
#include<QTextStream>

Widget::Widget(QWidget *parent, QString name) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    //初始化操作
    udpSocket = new QUdpSocket(this);
    //用户名获取
    uName = name;
    //端口号
    this->port = 9999;

    //绑定端口号  //采用ShareAddress模式(即允许其它的服务连接到相同的地址和端口，特别是用在多客户端监听同一个服务器端口等时特别有效)，和ReuseAddressHint模式(重新连接服务器)
    udpSocket->bind(port, QUdpSocket::ShareAddress |QUdpSocket::ReuseAddressHint);

    //发送新用户进入
    sndMsg(UsrEnter);

    //点击发送按钮发送消息
    connect(ui->sendBtn, &QPushButton::clicked, [=](){
        sndMsg(Msg);
    });

    //监听别人发送的消息
    connect(udpSocket,&QUdpSocket::readyRead,this,&Widget::ReceiveMessage);

    //点击退出按钮，实现关闭窗口
    connect(ui->exitBtn, &QPushButton::clicked, [=](){
        this->close();
    });

    //改变字体
    connect(ui->fontCbx, &QFontComboBox::currentFontChanged, [=](const QFont &font){
        ui->msgTxtEdit->setCurrentFont(font);
        ui->msgTxtEdit->setFocus();
    });

    //改变字号
    void(QComboBox::*cbxsignal)(const QString& text) = &QComboBox::currentIndexChanged;
    connect(ui->sizeCbx, cbxsignal, [=](const QString& text){
        ui->msgTxtEdit->setFontPointSize(text.toDouble());
        ui->msgTxtEdit->setFocus();
    });

    //字体加粗
    connect(ui->boldTBtn, &QToolButton::clicked, [=](bool isCheck){
        if(isCheck)
        {
            ui->msgTxtEdit->setFontWeight(QFont::Bold);
        }
        else
        {
            ui->msgTxtEdit->setFontWeight(QFont::Normal);
        }
    });

    //倾斜
    connect(ui->italicTBtn, &QToolButton::clicked, [=](bool Check){
        ui->msgTxtEdit->setFontItalic(Check);
    });

    //下划线
    connect(ui->underlineTBtn, &QToolButton::clicked, [=](bool check){
        ui->msgTxtEdit->setFontUnderline(check);
    });

    //改变颜色
    connect(ui->colorTBtn, &QToolButton::clicked, [=](){
        QColor color = QColorDialog::getColor(Qt::red);
        ui->msgTxtEdit->setTextColor(color);
    });

    //保存聊天记录
    connect(ui->saveTBtn, &QToolButton::clicked, [=](){
        if(ui->msgBrowser->document()->isEmpty())
        {
             QMessageBox::warning(this,"警告","聊天记录为空，无法保存！",QMessageBox::Ok);
        }
        else
        {
            QString fName = QFileDialog::getSaveFileName(this, "保存聊天记录","聊天记录","(*.txt)");
            if(fName.isEmpty())
            {
                QMessageBox::warning(this,"警告","保存路径为空，无法保存！",QMessageBox::Ok);
            }
            else
            {
                //保存名称不为空再做保存操作
                QFile file(fName);
                file.open(QIODevice::WriteOnly | QFile::Text);

                QTextStream stream(&file);
                stream << ui->msgBrowser->toPlainText();
                file.close();
            }
        }
    });

    //清空聊天记录
    connect(ui->clearTBtn, &QToolButton::clicked, [=](){
        ui->msgBrowser->clear();
    });
}

void Widget::usrEnter(QString username)
{
    //除理新用户加入
    //更新右侧TableWidget
    bool isEmpty1 = ui->usrTblWidget->findItems(username, Qt::MatchExactly).isEmpty();
    if(isEmpty1)
    {
        QTableWidgetItem *usr = new QTableWidgetItem(username);

        //插入行
        ui->usrTblWidget->insertRow(0);
        ui->usrTblWidget->setItem(0,0,usr);

        //追加聊天记录
        ui->msgBrowser->setTextColor(Qt::gray);
        ui->msgBrowser->append(QString("%1 上线了").arg(username));

        //设置在线人数
        ui->usrNumLbl->setText(QString("在线用户: %1").arg(ui->usrTblWidget->rowCount()));

        //已经在线的各个端点也要告知新加入的端点他们自己的信息，若不这样做，在新端点用户列表中就无法显示其他已经在线的用户
        sndMsg(UsrEnter);

    }
}


void Widget::usrLeft(QString usrname,QString time)
{
    //更新右侧tableWidget
    bool isEmpty = ui->usrTblWidget->findItems(usrname, Qt::MatchExactly).isEmpty();
    if(!isEmpty)
    {
        int row = ui->usrTblWidget->findItems(usrname, Qt::MatchExactly).first()->row();
        ui->usrTblWidget->removeRow(row);

        //追加聊天记录
        ui->msgBrowser->setTextColor(Qt::gray);
        ui->msgBrowser->append(QString("%1 于 %2离开").arg(usrname).arg(time));

        //在线人数更新
        ui->usrNumLbl->setText(QString("在线用户: %1").arg(ui->usrTblWidget->rowCount()));
    }
}


void Widget::sndMsg(MsgType type)
{
    //发送消息分为3种类型
    //发送的数据做分段处理，第一段 类型， 第二段用户名，第三段具体内容

    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream<<type<<getUsr(); //第一段类型添加到流中,第二段用户名
    switch(type){
    case Msg:  //普通消息发送
        if(ui->msgTxtEdit->toPlainText() == "")  //如果用户输入内容为空，进行警告
        {
            QMessageBox::warning(this, "警告", "发送内容不能为空");
            return;
        }
        stream<<getMsg();//第三段具体内容
        break;
    case UsrEnter:

        break;
    case UsrLeft:
        break;
    default:
        break;
    }

    //书写报文，广播发送让所有人都看到
    udpSocket->writeDatagram(array, QHostAddress::Broadcast, port);
}


//接受udp消息
void Widget::ReceiveMessage()
{
    //拿到数据报文，获取长度
    qint64 size = udpSocket->pendingDatagramSize();
    QByteArray array = QByteArray(size, 0);
    udpSocket->readDatagram(array.data(), size);

    //解析数据， 第一段类型 第二段用户名 第三段内容
    QDataStream stream(&array, QIODevice::ReadOnly);
    int msgType;//读取到类型
    QString usrName;
    QString msg;

    //获取当前时间
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    stream>>msgType;
    switch (msgType)
    {
    case Msg: //普通聊天
        stream>>usrName>>msg;

        //最近聊天记录
        ui->msgBrowser->setTextColor(Qt::blue);
        ui->msgBrowser->setCurrentFont(QFont("Times New Roman",12));
        ui->msgBrowser->append("[ " + usrName + " ]" + time);
        ui->msgBrowser->append(msg);

        break;
     case UsrEnter:
        stream>>usrName;
        usrEnter(usrName);
        break;
    case UsrLeft:
        stream>>usrName;
        usrLeft(usrName, time);
        break;
    }

}

QString Widget::getUsr()
{
    return this->uName;
}


QString Widget::getMsg()
{

    QString str = ui->msgTxtEdit->toHtml();

    //清空输入框
    ui->msgTxtEdit->clear();
    //设置光标再回到输入框
    ui->msgTxtEdit->setFocus();
    return str;
}

//关闭窗口事件，关闭窗口直接发送一个信号
void Widget::closeEvent(QCloseEvent*e)
{
    emit this->closeWidget();
    sndMsg(UsrLeft);
    //断开套接字
    udpSocket->close();
    udpSocket->destroyed();

    QWidget::closeEvent(e);
}

Widget::~Widget()
{
    delete ui;
}
