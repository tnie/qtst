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

void Factory::insert(QString key, IDatagram * ptr)
{
    if(map_.contains(key)) {
        qWarning() << __FUNCTION__ << ':' << __LINE__
                   << "There is already an item with the key:" << key;
    }
    map_.insert(key, ptr);
}

IDatagram *Factory::value(QString key) const
{
    return map_.value(key, nullptr);
}

QList<IDatagram *> Factory::values() const
{
    return map_.values();
}
