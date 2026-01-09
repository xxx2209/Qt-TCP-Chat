#include "Widget.h"
#include "ui_Widget.h"
#include <QMessageBox>
#include <QDateTime>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , isConnected(false)
{
    ui->setupUi(this);
    tcpSocket = new QTcpSocket(this);
    ui->ipEdit->setText("127.0.0.1");     // 默认本地IP
    ui->portEdit->setText("8888");        // 默认端口8888
    ui->msgEdit->setReadOnly(true);       // 消息显示区只读
    ui->sendBtn->setEnabled(false);       // 未连接时禁用发送按钮

    // 连接信号槽
    connect(tcpSocket, &QTcpSocket::readyRead, this, &Widget::on_readyRead);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &Widget::on_disconnected);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &Widget::on_errorOccurred);
}

Widget::~Widget()
{
    delete ui;
}

// 连接服务器
void Widget::on_connectBtn_clicked()
{
    if (!isConnected) {
        QString ip = ui->ipEdit->text();
        quint16 port = ui->portEdit->text().toUInt();
        tcpSocket->connectToHost(ip, port);
        if (tcpSocket->waitForConnected(3000)) {
            isConnected = true;
            ui->connectBtn->setText("断开连接");
            ui->sendBtn->setEnabled(true);
            ui->msgEdit->append("[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] 连接成功！");
        } else {
            QMessageBox::warning(this, "错误", "连接失败：" + tcpSocket->errorString());
        }
    } else {
        tcpSocket->disconnectFromHost();
    }
}

// 发送消息
void Widget::on_sendBtn_clicked()
{
    QString msg = ui->inputEdit->text().trimmed();
    if (msg.isEmpty()) return;

    // 发送消息到服务器
    QDataStream out(tcpSocket);
    out << QString("[自己]：%1").arg(msg);

    // 本地显示
    QString showMsg = "[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] [自己]：" + msg;
    ui->msgEdit->append(showMsg);
    ui->inputEdit->clear();
}

// 接收服务器转发的消息
void Widget::on_readyRead()
{
    QDataStream in(tcpSocket);
    QString msg;
    in >> msg;

    QString showMsg = "[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] " + msg;
    ui->msgEdit->append(showMsg);
}

// 断开连接
void Widget::on_disconnected()
{
    isConnected = false;
    ui->connectBtn->setText("连接服务器");
    ui->sendBtn->setEnabled(false);
    ui->msgEdit->append("[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] 已断开连接");
}

// 错误处理
void Widget::on_errorOccurred(QAbstractSocket::SocketError err)
{
    QMessageBox::warning(this, "错误", "网络错误：" + tcpSocket->errorString());
}
