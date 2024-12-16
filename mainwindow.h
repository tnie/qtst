#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QUdpSocket;
class QTimer;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnSwitch_toggled(bool checked);

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
};
#endif // MAINWINDOW_H
