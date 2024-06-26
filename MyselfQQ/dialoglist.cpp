#include "dialoglist.h"
#include "ui_dialoglist.h"
#include<QToolButton>
#include"widget.h"
#include<QMessageBox>


DialogList::DialogList(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DialogList)
{
    ui->setupUi(this);

    //设置标题
    setWindowTitle("MySelfQQ 2020");
    //设置图标
    setWindowIcon(QPixmap(":/images/qq.png"));

    //准备图标
    QList<QString>nameList;
    nameList << "太想爱你" << "爱情余味" <<"九月雨"<<"Cherry"<<"淡然"
             <<"娇娇girl"<<"落水无痕"<<"青墨暖暖"<<"无语";


    QStringList iconNameList; //图标资源列表
    iconNameList << "ftbz"<< "ymrl" <<"qq" <<"Cherry"<< "dr"
                 <<"jj"<<"lswh"<<"qmnn"<<"wy";

    QVector<QToolButton*>vToolBtn;

    for(int i = 0; i < 9; i++)
    {
        //设置好友
        QToolButton *btn = new QToolButton;
        //设置文字
        btn->setText(nameList[i]);
        //设置头像
        QString str = QString(":/images/%1.png").arg(iconNameList.at(i));
        btn->setIcon(QPixmap(str));
        //设置头像大小
        btn->setIconSize(QPixmap(str).size());
        //设置按钮风格
        btn->setAutoRaise(true);
        //设置文字和图片一起显示
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        //加到垂直布局
        ui->vLayout->addWidget(btn);
        //容器保存住9个按钮
        vToolBtn.push_back(btn);
        //9个标识默认初始化
        isShow.push_back(false);
    }

    //对9个按钮进行添加信号槽
    for(int i=0; i < vToolBtn.size(); i++)
    {
        connect(vToolBtn[i], &QToolButton::clicked, [=](){
            //如果打开了和某人的聊天窗口再点击也不会打开聊天窗口
            if(isShow[i])
            {
                QString str = QString("%1窗口已经被打开了").arg(vToolBtn[i]->text());
                QMessageBox::warning(this, "警告", str);
                return;
            }

            isShow[i] = true;
            //弹出聊天对话框，参数1为0顶层方式弹出,参数2窗口名字
            Widget* widget = new Widget(0, vToolBtn[i]->text());
            //设置窗口标题
            widget->setWindowTitle(vToolBtn[i]->text());
            widget->setWindowIcon(vToolBtn[i]->icon());
            widget->show();

            connect(widget, &Widget::closeWidget, [=](){
                isShow[i] = false;
            });
        });
    }
}

DialogList::~DialogList()
{
    delete ui;
}
