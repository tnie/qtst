#include "idatagram.h"
#include <QNetworkDatagram>

IDatagram::IDatagram()
{

}

IDatagram::~IDatagram()
{

}

QNetworkDatagram IDatagram::getDatagram() const
{
    return datagram;
}

void IDatagram::setDatagram(QNetworkDatagram value)
{
    datagram = value;
}

StDatagram::StDatagram(QHostAddress expectedSrc)
    :expectedSrc(expectedSrc)
{

}

QTime StDatagram::getTime() const
{
    Q_ASSERT(verify(datagram));
    const QByteArray bytes = datagram.data();
    QByteArray payload = bytes.mid(QByteArray("Hello").size());
    QString str = QString::fromUtf8(payload).simplified();
    return QTime::fromString(str);
}

bool StDatagram::verify(QNetworkDatagram datagram) const
{
    if(expectedSrc.isNull() || datagram.senderAddress() == expectedSrc)
    {
        constexpr int  pos = 3;
        const char id[] = {0x6c /*l*/, 0x6f /*o*/};

        const int length = sizeof(id);
        const QByteArray bytes = datagram.data();
        if(pos + length < bytes.size())
        {
            const QByteArray key = bytes.mid(pos, length);
            if(key == QByteArray(id, length))
            {
                return true;
            }
        }
    }
    return false;
}

QString StDatagram::toString() const
{
    return QString::fromUtf8(datagram.data());
}

QHostAddress StDatagram::getExpectedSrc() const
{
    return expectedSrc;
}

void StDatagram::setExpectedSrc(const QHostAddress &value)
{
    expectedSrc = value;
}

#include "example.h"

QString PersonDatagram::toString() const
{
    Person person;
    QByteArray bytes = datagram.data();
    person.deserialize(bytes);
    int age = person.getAge();
    if(age < 20) {
        Student student;
        student.deserialize(datagram.data());
        return student.toString();
    }
    else {
        Worker worker;
        worker.deserialize(datagram.data());
        return worker.toString();
    }
    return person.toString();
}

bool PersonDatagram::verify(QNetworkDatagram datagram) const
{
    Person person;
    QByteArray bytes = datagram.data();
    person.deserialize(bytes);
    int age = person.getAge();
    return (age >=0 && age <= 120);
}
