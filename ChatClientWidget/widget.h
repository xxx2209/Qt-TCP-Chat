#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QDataStream>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_connectBtn_clicked();         // 连接服务器
    void on_sendBtn_clicked();            // 发送消息
    void on_readyRead();                 // 接收服务器消息
    void on_disconnected();               // 断开连接
    void on_errorOccurred(QAbstractSocket::SocketError err); // 错误处理

private:
    Ui::Widget *ui;
    QTcpSocket *tcpSocket;
    bool isConnected;
    QString serverTip;
};

#endif // CHATCLIENTWIDGET_H
