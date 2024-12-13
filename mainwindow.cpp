#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QDateTime>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QUdpSocket(this))
    , timer(nullptr)
{
    ui->setupUi(this);
    initInterface();
    connect(socket, &QUdpSocket::stateChanged, [=](QAbstractSocket::SocketState socketState){
        ui->statusbar->showMessage(socket->errorString());
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initInterface()
{
    for (auto iface : QNetworkInterface::allInterfaces())
    {
        if(iface.isValid())
        {
            ui->iFaceJoin->addItem(iface.name());
            ui->iFaceSend->addItem(iface.name());
        }
    }
}

bool MainWindow::bind()
{
    Q_ASSERT(nullptr != socket);
    socket->abort();
    const QHostAddress addr(ui->bindAddr->text());
    const QHostAddress any(QHostAddress::AnyIPv4);
    // 绑定 local 无法收到多播；绑定 multicast / any 能够收到多播
    const QHostAddress & bindAddr = ui->cbAny ? any : addr;
    const quint16 bindPort = ui->cbRandom ? 0 : ui->bindPort->value();
    QAbstractSocket::BindMode mode = QAbstractSocket::DefaultForPlatform;
    {
        if(ui->cbShareAddress->isChecked())
            mode &= QAbstractSocket::ShareAddress;
        if(ui->cbDontShareAddress->isChecked())
            mode &= QAbstractSocket::DontShareAddress;
        if(ui->cbReuseAddressHint->isChecked())
            mode &= QAbstractSocket::ReuseAddressHint;
    }
    // 在 bind() 之前的 setSocketOption() 没有意义
    socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
    if(!socket->bind(bindAddr, bindPort, mode))
    {
        qWarning()<<"bind failed?" << socket->error() << socket->errorString();
        return false;
    }
    return true;
}

bool MainWindow::join()
{
    if(!ui->groupBoxJoin->isChecked())
        return true;
    Q_ASSERT(nullptr != socket);
    bool ok = false;
    const QHostAddress multicast(ui->multiAddr->text());
    if(ui->cbInterface->isChecked())
    {
        QNetworkInterface iface = QNetworkInterface::interfaceFromName( ui->iFaceJoin->currentText());
        ok = socket->joinMulticastGroup(multicast, iface);
    }
    else
    {
        ok = socket->joinMulticastGroup(multicast);
    }
    if(!ok)
    {
        qWarning()<<"join multicast group failed!" << socket->error() << socket->errorString();
        return false;
    }
    return true;
}

void MainWindow::send()
{
    Q_ASSERT(nullptr != socket);
    if(ui->cbInterface4Send->isChecked())
    {
        QNetworkInterface iface = QNetworkInterface::interfaceFromName( ui->iFaceSend->currentText());
        socket->setMulticastInterface(iface);
    }
    if(nullptr == timer)
    {
        timer = new QTimer(this);
    }
    const QByteArray datagram = QByteArray("Hello world");
    const QHostAddress address(ui->dstAddr->text());
    const quint16 port = ui->dstPort->value();
//    socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, 0);
    QObject::connect(timer, &QTimer::timeout, [=](){
        // socketOption() 在 bind() 之后才有意义
        const bool enable = ui->cbMulticastLoopbackOption->isChecked();
        socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, enable);
        qDebug() << "*MulticastLoopbackOption=" << socket->socketOption(QAbstractSocket::MulticastLoopbackOption);
        qint64 len = socket->writeDatagram(datagram, address, port);
        if(len != datagram.length())
        {
            qWarning() << "write datagram failed!" << socket->error() << socket->errorString();;
        }
        else
        {
            qDebug() << QDateTime::currentDateTime().toString() << "\n<<"  << datagram;
        }
    });
    timer->start(1000*5);
}

void MainWindow::on_groupBoxRecv_toggled(bool on)
{
    static QMetaObject::Connection handle;
    if(on)
    {
        Q_ASSERT(nullptr != socket);
        handle = QObject::connect(socket, &QUdpSocket::readyRead, [=](){
            QByteArray ba;
            QHostAddress addr;
            quint16 port;
            while(socket->hasPendingDatagrams()){
                qint64 ts = QDateTime::currentMSecsSinceEpoch();
                ba.resize(socket->pendingDatagramSize());
                socket->readDatagram(ba.data(), ba.size(), &addr, &port);
                qDebug() << ts << addr << port << "\n>>" << ba;//.toHex();
                ui->plainTextEditRecv->appendPlainText(QDateTime::currentDateTime().toString());
                ui->plainTextEditRecv->appendPlainText(addr.toString() + QString::number(port));
                ui->plainTextEditRecv->appendPlainText(QString::fromLatin1(ba.data()));
            }
        });
    }
    else
    {
        QObject::disconnect(handle);
    }
}

void MainWindow::on_btnSwitch_clicked(bool checked)
{
    if(checked)
    {
        ui->btnSwitch->setText("复位");
        if(bind())
        {
            join();
            send();
        }
    }
    else
    {
        ui->btnSwitch->setText("启动");
        socket->abort();
        timer->stop();
    }

}
