#include "window.h"
#include "ui_window.h"
#include "client.h"
#include <QStandardItemModel>
#include <QInputDialog>
#include <QMessageBox>
#include <QHostAddress>
#include <QJsonObject>
#include <QDebug>
#include <QTimer>

#define PORT 2000

Window::Window(QWidget *parent) : QWidget(parent), ui(new Ui::Window), m_Client(new Client(this))
{
    ui->setupUi(this);
    
    connect(m_Client, &Client::connected, this, &Window::connectedToServer);
    connect(m_Client, &Client::headerUpdate, this, &Window::headerUpdate);
    connect(m_Client, &Client::disconnected, this, &Window::disconnectedFromServer);
    connect(m_Client, &Client::error, this, &Window::error);
    connect(m_Client, &Client::loginDuplicate, this, &Window::loginDuplicate);
    connect(m_Client, &Client::userLimit, this, &Window::userLimit);
    connect(m_Client, &Client::wordUpdate, this, &Window::wordUpdate);
    connect(m_Client, &Client::usersLives, this, &Window::usersLives);

    connect(ui->connectButton, &QPushButton::clicked, this, &Window::attemptConnection);

    connect(ui->A, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->B, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->C, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->D, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->E, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->F, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->G, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->H, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->I, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->J, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->K, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->L, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->M, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->M, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->O, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->P, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->Q, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->R, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->S, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->T, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->U, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->V, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->W, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->X, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->Y, &QPushButton::clicked, this, &Window::sendMessage);
    connect(ui->Z, &QPushButton::clicked, this, &Window::sendMessage);
}

Window::~Window()
{
    delete ui;
}

void Window::attemptConnection()
{
    const QString hostAddress = QInputDialog::getText(
        this
        , tr("Connect")
        , tr("Address")
        , QLineEdit::Normal
        , QStringLiteral("127.0.0.1")
    );
    if (hostAddress.isEmpty())
        return;
    m_Client->connectToServer(QHostAddress(hostAddress), PORT);
}

void Window::wordUpdate(const QString &word)
{
    ui->word->setText(word);
}

void Window::usersLives(const QString &word)
{
    ui->users->setText(word);
}

void Window::connectedToServer()
{
    const QString newUsername = QInputDialog::getText(this, tr("Connect"), tr("Address"));
    if (newUsername.isEmpty()) {
        QMessageBox::warning(this, "Error", "Empty login");
        return m_Client->disconnectFromHost();
    }
    attemptLogin(newUsername);
}

void Window::attemptLogin(const QString &userName)
{
    m_Client->login(userName);
}

void Window::sendMessage()
{
    auto button = qobject_cast<QPushButton *>(sender());
    QString message = button->objectName();
    m_Client->sendMessage("guess," + message[message.length()-1]);
}

void Window::disconnectedFromServer()
{
    QMessageBox::warning(this, tr("Disconnected"), tr("The host terminated the connection"));
}

void Window::headerUpdate(const QString &word)
{
    ui->label->setText(word);
}

void Window::loginDuplicate()
{
    QMessageBox::warning(this, "Error", "Duplicate username");
}

void Window::userLimit()
{
    QMessageBox::warning(this, "Error", "Too many players. Try again later.");
}

void Window::error(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
    case QAbstractSocket::ProxyConnectionClosedError:
        return;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::critical(this, tr("Error"), tr("The host refused the connection"));
        break;
    case QAbstractSocket::ProxyConnectionRefusedError:
        QMessageBox::critical(this, tr("Error"), tr("The proxy refused the connection"));
        break;
    case QAbstractSocket::ProxyNotFoundError:
        QMessageBox::critical(this, tr("Error"), tr("Could not find the proxy"));
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::critical(this, tr("Error"), tr("Could not find the server"));
        break;
    case QAbstractSocket::SocketAccessError:
        QMessageBox::critical(this, tr("Error"), tr("You don't have permissions to execute this operation"));
        break;
    case QAbstractSocket::SocketResourceError:
        QMessageBox::critical(this, tr("Error"), tr("Too many connections opened"));
        break;
    case QAbstractSocket::SocketTimeoutError:
        QMessageBox::warning(this, tr("Error"), tr("Operation timed out"));
        return;
    case QAbstractSocket::ProxyConnectionTimeoutError:
        QMessageBox::critical(this, tr("Error"), tr("Proxy timed out"));
        break;
    case QAbstractSocket::NetworkError:
        QMessageBox::critical(this, tr("Error"), tr("Unable to reach the network"));
        break;
    case QAbstractSocket::UnknownSocketError:
        QMessageBox::critical(this, tr("Error"), tr("An unknown error occured"));
        break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        QMessageBox::critical(this, tr("Error"), tr("Operation not supported"));
        break;
    case QAbstractSocket::ProxyAuthenticationRequiredError:
        QMessageBox::critical(this, tr("Error"), tr("Your proxy requires authentication"));
        break;
    case QAbstractSocket::ProxyProtocolError:
        QMessageBox::critical(this, tr("Error"), tr("Proxy comunication failed"));
        break;
    case QAbstractSocket::TemporaryError:
    case QAbstractSocket::OperationError:
        QMessageBox::warning(this, tr("Error"), tr("Operation failed, please try again"));
        return;
    default:
        Q_UNREACHABLE();
    }
}
