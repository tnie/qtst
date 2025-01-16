#ifndef HELLODATAGRAM_H
#define HELLODATAGRAM_H

#include "idatagram.h"
#include <QTime>

class HelloDatagram : public IDatagram
{
public:
    HelloDatagram() = default;
    HelloDatagram(QHostAddress expectedSrc);
    QTime getTime() const;
    // IDatagram interface
public:
    bool verify(QNetworkDatagram) const override;
    virtual QString toString() const override;

    QHostAddress getExpectedSrc() const;
    void setExpectedSrc(const QHostAddress &value);

private:
    QHostAddress expectedSrc;
};

#endif // HELLODATAGRAM_H
