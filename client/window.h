#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QAbstractSocket>

class Client;
class QStandardItemModel;
namespace Ui { class Window; }
class Window : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(Window)
public:
    explicit Window(QWidget *parent = nullptr);
    ~Window();
private:
    Ui::Window *ui;
    Client *m_Client;
    QStandardItemModel *m_Model;
private slots:
    void attemptConnection();
    void connectedToServer();
    void attemptLogin(const QString &userName);
    void headerUpdate(const QString &word);
    void sendMessage();
    void disconnectedFromServer();
    void loginDuplicate();
    void userLimit();
    void wordUpdate(const QString &word);
    void usersLives(const QString &word);
    void error(QAbstractSocket::SocketError socketError);
};

#endif // WINDOW_H
