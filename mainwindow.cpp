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
    , timer(new QTimer(this))
{
    ui->setupUi(this);
    initInterface();
    QString bindTip = "绑定本地地址无法收到多播报文；绑定多播地址只能接收多播报文；推荐绑定ANY";
    ui->bindAddr->setStatusTip(bindTip);
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
    connect(socket, &QUdpSocket::stateChanged, [=](QAbstractSocket::SocketState socketState){
        ui->statusbar->showMessage(socket->errorString());
    });
    const QHostAddress addr(ui->bindAddr->text());
    const QHostAddress any(QHostAddress::AnyIPv4);
    // 绑定 local 无法收到多播；绑定 multicast / any 能够收到多播
    const QHostAddress & bindAddr = ui->cbAny->isChecked() ? any : addr;
    const quint16 bindPort = ui->cbRandom->isChecked() ? 0 : ui->bindPort->value();
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

bool MainWindow::recv()
{
    static QMetaObject::Connection handle;
    if(!ui->groupBoxRecv->isChecked())
    {
        QObject::disconnect(handle);
        return true;
    }
    Q_ASSERT(nullptr != socket);
    // 在 socket 发送数据之后，disconnect 后重新关联槽函数似乎无法收到报文 #todo#
    handle = QObject::connect(socket, &QUdpSocket::readyRead, [=](){
        QByteArray ba;
        QHostAddress addr;
        quint16 port;
        while(socket->hasPendingDatagrams()){
            ba.resize(socket->pendingDatagramSize());
            socket->readDatagram(ba.data(), ba.size(), &addr, &port);
            qDebug() << addr << port << "\n>>" << QString::fromUtf8(ba);
            ui->plainTextEditRecv->appendPlainText(addr.toString() + ':' + QString::number(port));
            ui->plainTextEditRecv->appendPlainText(">> " + QString::fromUtf8(ba));
        }
    });
    return join() && handle;
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
        QString text = ui->iFaceJoin->currentText();
        QNetworkInterface iface = QNetworkInterface::interfaceFromName( text);
        // #todo# 追加，前序加租未撤销
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

bool MainWindow::send()
{
    static QMetaObject::Connection handle;
    if(!ui->groupBoxSend->isChecked())
    {
        QObject::disconnect(handle);
        Q_ASSERT(false == handle);
        if(timer != nullptr)
            timer->stop();
        return true;
    }
    Q_ASSERT(nullptr != socket);
    if(ui->cbInterface4Send->isChecked())
    {
        QString text = ui->iFaceSend->currentText();
        QNetworkInterface iface = QNetworkInterface::interfaceFromName( text);
        // #todo# 无法撤销，除非重建 socket
        socket->setMulticastInterface(iface);
    }
    Q_ASSERT(nullptr != timer);
    handle = QObject::connect(timer, &QTimer::timeout, [=](){
        // socketOption() 在 bind() 之后才有意义
        const bool enable = ui->cbMulticastLoopbackOption->isChecked();
        socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, enable);
        qDebug() << "*MulticastLoopbackOption=" << socket->socketOption(QAbstractSocket::MulticastLoopbackOption);
        const QString data = QDateTime::currentDateTime().toString();
        const QByteArray datagram= data.toUtf8();
        const QHostAddress address(ui->dstAddr->text());
        const quint16 port = ui->dstPort->value();
        qint64 len = socket->writeDatagram(datagram, address, port);
        if(len != datagram.length())
        {
            qWarning() << "write datagram failed!" << socket->error() << socket->errorString();
        }
        else
        {
            qDebug() << "<<"  << data;
            ui->plainTextEditSend->appendPlainText("<< " + data);
        }
    });
    const int seconds = ui->sliderFrequency->value();
    timer->start(1000* seconds);
    return handle;
}

void MainWindow::on_btnSwitch_clicked(bool checked)
{
    if(checked)
    {
        ui->btnSwitch->setText("复位(&T)");
        if(bind())
        {
            recv();
            send();
        }
    }
    else
    {
        ui->btnSwitch->setText("启动(&B)");
        socket->abort();
        // #todo# 会误杀 stateChanged 槽函数
        socket->disconnect();
        timer->stop();
        timer->disconnect();
    }

}
