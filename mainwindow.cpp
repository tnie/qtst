#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>
#include <QNetworkDatagram>

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
QString stateString(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QAbstractSocket::UnconnectedState:
        return "UnconnectedState";
    case QAbstractSocket::HostLookupState:
        return "HostLookupState";
    case QAbstractSocket::ConnectingState:
        return "ConnectingState";
    case QAbstractSocket::ConnectedState:
        return "ConnectedState";
    case QAbstractSocket::BoundState:
        return "BoundState";
    case QAbstractSocket::ListeningState:
        return "ListeningState";
    case QAbstractSocket::ClosingState:
        return "ClosingState";
    default:
        break;
    }
    return "Unknown";
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(nullptr)
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
        (on && (nullptr != socket)) ? send() : stopSend();
    });
    connect(ui->groupBoxRecv, &QGroupBox::toggled, this, [=](bool on){
        (on && (nullptr != socket)) ? recv() : stopRecv();
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
    Q_ASSERT(nullptr == socket);
    socket = new QUdpSocket(this);
    connect(socket, &QUdpSocket::stateChanged, socket, [=](QAbstractSocket::SocketState socketState){
        ui->plainTextEditLog->appendPlainText("=> " + stateString(socketState));
    });
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), socket,
            [=](QAbstractSocket::SocketError socketError){
        qWarning() << socketError << socket->errorString();
        ui->plainTextEditLog->appendPlainText(socket->errorString());
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
        qWarning()<<"bind failed!" << socket->error() << socket->errorString();
        socket->deleteLater();
        socket = nullptr;
        return false;
    }
    connect(ui->cbMulticastLoopbackOption, &QCheckBox::stateChanged, socket, [=](int state){
        // socketOption() 在 bind() 之后才有意义
        socket->setSocketOption(QAbstractSocket::MulticastLoopbackOption, state);
        QVariant option = socket->socketOption(QAbstractSocket::MulticastLoopbackOption);
        qDebug() << "*MulticastLoopbackOption=" << option;
        ui->plainTextEditLog->appendPlainText(QString("*MulticastLoopbackOption=%1").arg(option.toInt()));
    });
    connect(ui->iFaceSend, &QComboBox::currentTextChanged, socket, [=](const QString& text){
        QNetworkInterface iface = QNetworkInterface::interfaceFromName( text);
        // 无法撤销，除非重建 socket
        socket->setMulticastInterface(iface);
    });

    return true;
}

void MainWindow::recv()
{
    Q_ASSERT(nullptr != socket);
    while(socket->hasPendingDatagrams())
    {
        socket->receiveDatagram(); // skip
    }
    // emit readyRead() 信号依赖 receiveDatagram() 重置标志位
    QObject::connect(socket, &QUdpSocket::readyRead, this, [=](){
        if(!socket->hasPendingDatagrams())
        {
            QNetworkDatagram datagram = socket->receiveDatagram(); // workaround for bad checksum
            QString msg("** readyRead emitted, but nothing to get :(");
            QString warning = QString(R"(<font color="red">%1</font>)").arg(msg);
            ui->plainTextEditRecv->appendHtml(warning);
            processTheDatagram(datagram);
        }
        while(socket->hasPendingDatagrams()){
            QNetworkDatagram datagram = socket->receiveDatagram();
            processTheDatagram(datagram);
        }
    });
    socket->receiveDatagram(0); // workaround for connect() again
    join();
}

void MainWindow::stopRecv()
{
    if(nullptr != socket)
    {
        QObject::disconnect(socket, &QUdpSocket::readyRead, 0, 0);
    }
}

bool MainWindow::join()
{
    if(!ui->groupBoxJoin->isChecked())
        return true;
    if(ui->groupBoxJoin->isHidden())
        return true;
    Q_ASSERT(nullptr != socket);
    if(QUdpSocket::BoundState != socket->state())
    {
        // QUdpSocket::joinMulticastGroup() called on a QUdpSocket when not in QUdpSocket::BoundState
        return true;
    }
    bool ok = false;
    const QHostAddress multicast(ui->multiAddr->text());
    if(ui->cbInterface->isChecked())
    {
        QString text = ui->iFaceJoin->currentText();
        QNetworkInterface iface = QNetworkInterface::interfaceFromName( text);
        // 重复加相同的组会报错；但离组再加又会干扰接收
//        socket->leaveMulticastGroup(multicast, iface);
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

void MainWindow::processTheDatagram(const QNetworkDatagram & datagram)
{
    QString text;
    QTextStream stream(&text);
    stream << datagram.senderAddress().toString() << ":" << datagram.senderPort() << " => "
          << datagram.destinationAddress().toString() << ":" << datagram.destinationPort();
    ui->plainTextEditRecv->appendPlainText(text);
    text = QString::fromUtf8(datagram.data());
    if(text.isEmpty())
        text = QString::fromUtf8(datagram.data().toHex());
    ui->plainTextEditRecv->appendPlainText(">> " + text);
}

void MainWindow::send()
{
    Q_ASSERT(nullptr != socket);
    if(ui->cbInterface4Send->isChecked())
    {
        if(QAbstractSocket::BoundState == socket->state())
        {
            QString text = ui->iFaceSend->currentText();
            QNetworkInterface iface = QNetworkInterface::interfaceFromName( text);
            // 无法撤销（只能更改），除非重建 socket
            socket->setMulticastInterface(iface);
        }
        else
        {
            //1. QUdpSocket::setMulticastInterface() called on a QUdpSocket when not in QUdpSocket::BoundState
            //2. 避免未绑定时直接发送，使用默认参数绑定端口
            return;
        }
    }
    Q_ASSERT(nullptr == timer);
    timer = new QTimer(this);
    QObject::connect(timer, &QTimer::timeout, this, [=](){
        const QString data = QDateTime::currentDateTime().toString();
        const QByteArray datagram= data.toUtf8();
        const QHostAddress address(ui->dstAddr->text());
        const quint16 port = ui->dstPort->value();
        Q_ASSERT(QAbstractSocket::ConnectedState != socket->state());
        Q_ASSERT(datagram.size() <= 512);   // 见 writeDatagram() 手册
        qint64 len = socket->writeDatagram(datagram, address, port);
        if(len != datagram.length())
        {
            qWarning() << "write datagram failed!" << socket->error() << socket->errorString();
        }
        else
        {
            QString text;
            QTextStream stream(&text);
            stream << address.toString() << ":" << port << " <= "
                      << socket->localAddress().toString() << ":" << socket->localPort();
            qDebug() << text << "\n<<"  << data;
            ui->plainTextEditSend->appendPlainText(text);
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
        if(bind())
        {
            ui->btnSwitch->setText("复位(&T)");
            if(ui->groupBoxRecv->isChecked() && ui->groupBoxRecv->isVisible())
                recv();
            if(ui->groupBoxSend->isChecked())
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
