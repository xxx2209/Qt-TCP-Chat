#include "Widget.h"
#include "ui_Widget.h"
#include <QMessageBox>
#include <QDateTime>
#include <QKeyEvent>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
    , isConnected(false)
{
    ui->setupUi(this);
    tcpSocket = new QTcpSocket(this);
    ui->ipEdit->setText("127.0.0.1");
    ui->portEdit->setText("8888");
    ui->msgEdit->setReadOnly(true);
    ui->sendBtn->setEnabled(false);
    // 连接信号槽
    connect(tcpSocket, &QTcpSocket::readyRead, this, &Widget::on_readyRead);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &Widget::on_disconnected);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &Widget::on_errorOccurred);
    // 安装事件过滤器
    ui->inputEdit->installEventFilter(this);
}

Widget::~Widget()
{
    delete ui;
}

bool Widget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->inputEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (!keyEvent) return QWidget::eventFilter(obj, event);

        if ((keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)) {
            if (!keyEvent->modifiers().testFlag(Qt::ShiftModifier)) {
                on_sendBtn_clicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

// 连接服务器（无修改，确保逻辑正确）
void Widget::on_connectBtn_clicked()
{
    if (!isConnected) {
        QString ip = ui->ipEdit->text();
        quint16 port = ui->portEdit->text().toUInt();
        // 重置socket状态，避免之前的错误影响
        tcpSocket->abort();
        tcpSocket->connectToHost(ip, port);
        if (tcpSocket->waitForConnected(3000)) {
            isConnected = true;
            ui->connectBtn->setText("断开连接");
            ui->sendBtn->setEnabled(true);
            ui->msgEdit->append("[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] 连接成功！");
            serverTip.clear(); // 连接成功后清空之前的提示
        } else {
            // 连接超时，直接显示系统错误
            QMessageBox::warning(this, "错误", "连接失败：" + tcpSocket->errorString());
        }
    } else {
        tcpSocket->disconnectFromHost();
    }
}

// 发送消息（无修改，确保只发纯消息）
void Widget::on_sendBtn_clicked()
{
    QString msg = ui->inputEdit->toPlainText().trimmed();
    if (msg.isEmpty() || !isConnected) return;

    QString showMsg = "[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] [自己]：" + msg;
    ui->msgEdit->append(showMsg);

    QByteArray sendData = msg.toUtf8();
    tcpSocket->write(sendData);
    ui->inputEdit->clear();
}

// 接收消息（核心修复：区分服务器提示和聊天消息）
void Widget::on_readyRead()
{
    QByteArray data = tcpSocket->readAll();
    if (data.isEmpty()) return;
    QString receivedMsg = QString::fromUtf8(data);

    // 约定：服务器提示以[SERVER_TIP]开头，聊天消息无此前缀
    if (receivedMsg.startsWith("[SERVER_TIP]")) {
        // 提取服务器提示（去掉前缀）
        serverTip = receivedMsg.remove(0, 12); // "[SERVER_TIP]"长度为12
        // 直接显示提示
        ui->msgEdit->append("[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] 服务器提示：" + serverTip);
    } else {
        // 聊天消息，正常显示
        QString showMsg = "[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] " + receivedMsg;
        ui->msgEdit->append(showMsg);
    }
}

// 断开连接（无修改，确保状态重置）
void Widget::on_disconnected()
{
    isConnected = false;
    ui->connectBtn->setText("连接服务器");
    ui->sendBtn->setEnabled(false);
    ui->msgEdit->append("[" + QDateTime::currentDateTime().toString("HH:mm:ss") + "] 已断开连接");
}

// 错误处理（核心修复：优先显示服务器提示）
void Widget::on_errorOccurred(QAbstractSocket::SocketError err)
{
    // 如果有服务器提示，优先显示提示；否则显示系统错误
    if (!serverTip.isEmpty()) {
        QMessageBox::information(this, "连接提示", serverTip);
        serverTip.clear();
    } else {
        // 过滤“正常断开”的错误（如用户主动关闭连接）
        if (err != QAbstractSocket::RemoteHostClosedError || !isConnected) {
            QMessageBox::warning(this, "错误", "网络错误：" + tcpSocket->errorString());
        }
    }
}
