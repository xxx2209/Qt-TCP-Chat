#include "widget.h"
#include "ui_widget.h"
#include <QDataStream>
#include <QMessageBox>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    tcpServer = new QTcpServer(this);

    // 关键修复：绑定newConnection信号到槽函数（之前缺失）
    connect(tcpServer, &QTcpServer::newConnection, this, &Widget::on_newConnection);

    ui->portEdit->setText("8888");       // 默认端口8888
    ui->logEdit->setReadOnly(true);      // 日志显示区只读
}

Widget::~Widget()
{
    delete ui;
}

// 启动服务器
void Widget::on_startBtn_clicked()
{
    quint16 port = ui->portEdit->text().toUInt();
    if (tcpServer->listen(QHostAddress::Any, port)) {
        ui->startBtn->setEnabled(false);
        ui->logEdit->append("服务器启动成功，监听端口：" + QString::number(port));
    } else {
        QMessageBox::warning(this, "错误", "服务器启动失败：" + tcpServer->errorString());
    }
}

// 新客户端连接（最多允许2个客户端）
void Widget::on_newConnection()
{
    if (clientSockets.size() >= 2) {
        QTcpSocket *rejectSocket = tcpServer->nextPendingConnection();
        rejectSocket->write("服务器已达最大连接数（2人）");
        rejectSocket->disconnectFromHost();
        rejectSocket->deleteLater();
        ui->logEdit->append("拒绝新连接：服务器已达最大连接数（2人）");
        return;
    }

    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    clientSockets.append(clientSocket);
    ui->logEdit->append("新客户端连接：" + clientSocket->peerAddress().toString()
                        + ":" + QString::number(clientSocket->peerPort()));

    // 连接信号槽
    connect(clientSocket, &QTcpSocket::readyRead, this, &Widget::on_readyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Widget::on_clientDisconnected);
}

// 接收客户端消息并转发给另一个客户端
void Widget::on_readyRead()
{
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    // 读取原始消息（不拼接IP，让客户端自己处理显示）
    QByteArray rawData = senderSocket->readAll();
    // 服务器日志显示：带IP的完整信息
    QString logMsg = QString("[%1:%2]：%3")
                         .arg(senderSocket->peerAddress().toString())
                         .arg(senderSocket->peerPort())
                         .arg(QString(rawData));
    ui->logEdit->append(logMsg);

    // 转发给另一个客户端：直接发原始消息（不用QDataStream，降低客户端解析难度）
    for (QTcpSocket *socket : clientSockets) {
        if (socket != senderSocket && socket->state() == QTcpSocket::ConnectedState) {
            socket->write(rawData); // 转发原始消息，客户端直接显示即可
        }
    }
}

// 客户端断开连接
void Widget::on_clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        ui->logEdit->append("客户端断开连接：" + socket->peerAddress().toString()
                                                                   + ":" + QString::number(socket->peerPort()));
        clientSockets.removeOne(socket);
        socket->deleteLater();
        ui->startBtn->setEnabled(true); // 断开后允许重新启动服务器（可选）
    }
}
