#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_startBtn_clicked();          // 启动服务器
    void on_newConnection();             // 新客户端连接
    void on_readyRead();                 // 接收客户端消息
    void on_clientDisconnected();        // 客户端断开连接

private:
    Ui::Widget *ui;
    QTcpServer *tcpServer;
    QList<QTcpSocket*> clientSockets;    // 存储两个客户端Socket
};
#endif // WIDGET_H
