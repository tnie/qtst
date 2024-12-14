#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>

namespace {
QString addrType(const QHostAddress& addr)
{
    if(addr.isBroadcast())
        return "广播地址";
    if(addr.isMulticast())
        return "多播地址";
    if(addr.isLoopback())
        return "回环地址";
    if(addr.isGlobal())
        return "广域网或局域网地址";
    if(addr.isNull())
        return "无效地址";
    return "";
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QUdpSocket(this))
    , timer(nullptr)
{
    ui->setupUi(this);
    initInterface();
    QString bindTip = "绑定本地地址无法收到多播报文；绑定多播地址只能接收多播报文；推荐绑定ANY";
    ui->bindAddr->setStatusTip(bindTip);
    ui->labelAddrType->clear();
    connect(ui->dstAddr, &QLineEdit::textEdited, this, [=](const QString& text){
        bool isMulticast = QHostAddress(text).isMulticast();
        ui->groupBoxMulti->setVisible(isMulticast);
        ui->iFaceSend->setEnabled(isMulticast);
        ui->cbInterface4Send->setEnabled(isMulticast);
        ui->cbMulticastLoopbackOption->setEnabled(isMulticast);
        QString type = addrType(QHostAddress(text));
        ui->labelAddrType->setText(type);
    });
    connect(ui->groupBoxSend, &QGroupBox::toggled, this, [=](bool on){
        on ? send() : stopSend();
    });
    connect(ui->sliderFrequency, &QSlider::valueChanged, this, [this](int seconds){
        if(timer)
            timer->start(1000* seconds);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initInterface()
{
    for (auto & iface : QNetworkInterface::allInterfaces())
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
    connect(socket, &QUdpSocket::stateChanged, socket, [=](QAbstractSocket::SocketState socketState){
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
    connect(ui->cbMulticastLoopbackOption, &QCheckBox::stateChanged, socket, [=](int state){
        // socketOption() 在 bind() 之后才有意义
        socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, state);
        qDebug() << "*MulticastLoopbackOption=" << socket->socketOption(QAbstractSocket::MulticastLoopbackOption);
    });
    connect(ui->iFaceSend, &QComboBox::currentTextChanged, socket, [=](const QString& text){
        QNetworkInterface iface = QNetworkInterface::interfaceFromName( text);
        // 无法撤销，除非重建 socket
        socket->setMulticastInterface(iface);
    });

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
    handle = QObject::connect(socket, &QUdpSocket::readyRead, this, [=](){
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
        // #todo# 追加，前序加组未撤销
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

void MainWindow::stopSend()
{
    if(nullptr != timer)
    {
        timer->deleteLater();
        timer = nullptr;
    }
}

void MainWindow::send()
{
    if(nullptr != timer)
        return;
    Q_ASSERT(nullptr != socket);
    if(ui->cbInterface4Send->isChecked())
    {
        QString text = ui->iFaceSend->currentText();
        QNetworkInterface iface = QNetworkInterface::interfaceFromName( text);
        // 无法撤销，除非重建 socket
        socket->setMulticastInterface(iface);
    }
    timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, [=](){
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
}

void MainWindow::on_btnSwitch_toggled(bool checked)
{
    if(checked)
    {
        if(nullptr == socket)
            socket = new QUdpSocket(this);
        if(bind())
        {
            ui->btnSwitch->setText("复位(&T)");
            recv();
            send();
        }
        else
        {
            QMessageBox::warning(this, this->windowTitle(), "绑定失败");
            ui->btnSwitch->setChecked(false);
        }
    }
    else
    {
        ui->btnSwitch->setText("启动(&B)");
        socket->disconnect();
        // 重新创建 QUdpSocket 对象，因为有些设置无法清除（多播发送接口），有些设置不容易清除（已加入的组）
        socket->deleteLater();
        socket = nullptr;
        stopSend();
    }
}
