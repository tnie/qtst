#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QUdpSocket;
class QTimer;
class QNetworkDatagram;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnSwitch_toggled(bool checked);

    void on_actNewDemo_triggered();

    void on_actQuit_triggered();

    void on_actNewTab_triggered();

    void on_actAbout_triggered();

private:
    Ui::MainWindow *ui;
    QUdpSocket * socket;
    QTimer *timer;
    void initInterface();
    bool bind();
    void recv();
    void stopRecv();
    bool join();
    void send();
    void stopSend();
    void processTheDatagram(const QNetworkDatagram&);
};
#endif // MAINWINDOW_H
