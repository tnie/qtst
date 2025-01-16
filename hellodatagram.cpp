#include "hellodatagram.h"

HelloDatagram::HelloDatagram(QHostAddress expectedSrc)
    :expectedSrc(expectedSrc)
{

}

QTime HelloDatagram::getTime() const
{
    Q_ASSERT(verify(datagram));
    const QByteArray bytes = datagram.data();
    QByteArray payload = bytes.mid(QByteArray("Hello").size());
    QString str = QString::fromUtf8(payload).simplified();
    return QTime::fromString(str);
}

bool HelloDatagram::verify(QNetworkDatagram datagram) const
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

QString HelloDatagram::toString() const
{
    return QString::fromUtf8(datagram.data());
}

QHostAddress HelloDatagram::getExpectedSrc() const
{
    return expectedSrc;
}

void HelloDatagram::setExpectedSrc(const QHostAddress &value)
{
    expectedSrc = value;
}

REGISTER_IDATAGRAM(HelloDatagram, "hello")
