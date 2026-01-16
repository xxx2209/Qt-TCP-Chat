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
    // 绑定新连接信号
    connect(tcpServer, &QTcpServer::newConnection, this, &Widget::on_newConnection);
    ui->portEdit->setText("8888");
    ui->logEdit->setReadOnly(true);
}

Widget::~Widget()
{
    // 析构时断开所有客户端连接，避免内存泄漏
    foreach (QTcpSocket *socket, clientSockets) {
        if (socket->state() == QTcpSocket::ConnectedState) {
            socket->disconnectFromHost();
            socket->waitForDisconnected(1000);
        }
        socket->deleteLater();
    }
    clientSockets.clear();
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

// 新客户端连接（核心修复：增加socket有效性检查和错误处理）
void Widget::on_newConnection()
{
    if (clientSockets.size() >= 2) {
        QTcpSocket *rejectSocket = tcpServer->nextPendingConnection();
        if (!rejectSocket) return; // 检查socket是否有效

        // 发送拒绝提示
        QByteArray tipData = "[SERVER_TIP]服务器已达最大连接数（2人）";
        rejectSocket->write(tipData);
        // 等待发送完成，同时处理“客户端提前关闭”的情况
        if (rejectSocket->waitForBytesWritten(1000)) {
            ui->logEdit->append("已向新客户端发送拒绝提示");
        } else {
            ui->logEdit->append("发送拒绝提示失败：" + rejectSocket->errorString());
        }

        // 安全断开连接
        rejectSocket->disconnectFromHost();
        // 等待断开完成，避免数据残留
        if (rejectSocket->state() != QTcpSocket::UnconnectedState) {
            rejectSocket->waitForDisconnected(1000);
        }
        rejectSocket->deleteLater();
        ui->logEdit->append("拒绝新连接：服务器已达最大连接数（2人）");
        return;
    }

    // 正常连接逻辑：增加socket有效性检查和错误绑定
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    if (!clientSocket) {
        ui->logEdit->append("新客户端连接失败：socket创建无效");
        return;
    }

    // 绑定客户端的错误信号，避免异常断开
    connect(clientSocket, &QTcpSocket::errorOccurred, this, [=](QAbstractSocket::SocketError err) {
        ui->logEdit->append("客户端[" + clientSocket->peerAddress().toString() + ":" + QString::number(clientSocket->peerPort()) + "]错误：" + clientSocket->errorString());
    });

    clientSockets.append(clientSocket);
    ui->logEdit->append("新客户端连接：" + clientSocket->peerAddress().toString()
                        + ":" + QString::number(clientSocket->peerPort()));
    // 绑定消息接收和断开信号
    connect(clientSocket, &QTcpSocket::readyRead, this, &Widget::on_readyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Widget::on_clientDisconnected);
}

// 接收消息并转发（无修改，确保逻辑正确）
void Widget::on_readyRead()
{
    QTcpSocket *senderSocket = qobject_cast<QTcpSocket*>(sender());
    if (!senderSocket) return;

    QByteArray rawData = senderSocket->readAll();
    if (rawData.isEmpty()) return; // 忽略空数据
    QString msg = QString::fromUtf8(rawData);

    // 服务器日志
    QString logMsg = QString("[%1:%2]：%3")
                         .arg(senderSocket->peerAddress().toString())
                         .arg(senderSocket->peerPort())
                         .arg(msg);
    ui->logEdit->append(logMsg);

    // 转发给其他客户端
    QString senderInfo = QString("[%1:%2]").arg(senderSocket->peerAddress().toString()).arg(senderSocket->peerPort());
    QByteArray forwardData = QString("%1：%2").arg(senderInfo).arg(msg).toUtf8();

    foreach (QTcpSocket *socket, clientSockets) {
        if (socket != senderSocket && socket->state() == QTcpSocket::ConnectedState) {
            socket->write(forwardData);
            // 等待转发完成，避免数据丢失
            socket->waitForBytesWritten(500);
        }
    }
}

// 客户端断开连接（无修改，确保列表清理正确）
void Widget::on_clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        ui->logEdit->append("客户端断开连接：" + socket->peerAddress().toString()
                                                                   + ":" + QString::number(socket->peerPort()));
        clientSockets.removeOne(socket);
        socket->deleteLater();
        // 所有客户端断开后允许重启服务器
        if (clientSockets.isEmpty()) {
            ui->startBtn->setEnabled(true);
        }
    }
}
