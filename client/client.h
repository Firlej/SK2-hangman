#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>

class QHostAddress;
class QJsonDocument;
class Client : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Client)
public:
    explicit Client(QObject *parent = nullptr);
public slots:
    void connectToServer(const QHostAddress &address, quint16 port);
    void login(const QString &userName);
    void sendMessage(const QString &text);
    void disconnectFromHost();
private slots:
    void onReadyRead();
signals:
    void connected();
    void headerUpdate(const QString &word);
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);
    void loginDuplicate();
    void userLimit();
    void wordUpdate(const QString &word);
    void usersLives(const QString &word);
private:
    QTcpSocket *m_clientSocket;
    bool m_loggedIn;
    void messageType(const QString &data);
};

#endif
