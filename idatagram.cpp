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

QHostAddress StDatagram::getExpectedSrc() const
{
    return expectedSrc;
}

void StDatagram::setExpectedSrc(const QHostAddress &value)
{
    expectedSrc = value;
}

#include "student.h"

void StudentDatagram::print()
{
    Student student;
    // 反序列化
    student.deserialize(datagram.data());
    // 打印
    qint64 ts = student.timestamp();
    QTime time = QDateTime::fromSecsSinceEpoch(ts).time();
    qDebug() << student.getNum() << student.isFemale() << student.getMath()
             << student.getEng() << time;
}
