#ifndef IDATAGRAM_H
#define IDATAGRAM_H

#include <QNetworkDatagram>

class IDatagram{
public:
    IDatagram();
    virtual ~IDatagram();
    virtual bool verify(QNetworkDatagram) const = 0;
    virtual QString toString() const {}

    QNetworkDatagram getDatagram() const;
    void setDatagram(QNetworkDatagram);

protected:
    QNetworkDatagram datagram;
};

#include <QTime>

class StDatagram : public IDatagram
{
public:
    StDatagram() = default;
    StDatagram(QHostAddress expectedSrc);
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

class PersonDatagram : public IDatagram
{
    // IDatagram interface
public:
    bool verify(QNetworkDatagram) const override;
    virtual QString toString() const override;
};

#endif // IDATAGRAM_H
