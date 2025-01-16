#include "navprotocol.h"

#include <QtEndian>

class Field
{
public:
    Field() = default;
    Field(QString name):name_(name) {}

    QString name() const {
        return name_;
    }
    void setName(const QString &name) {
        name_ = name;
    }

    virtual QByteArray serialize(const QVariant) const = 0;
    virtual QVariant deserialize(const QByteArray) const = 0;
    virtual unsigned serializedSize() const = 0;
    virtual ~Field() {}

private:
    QString name_;  // 方便调试
};

template<typename T>
class FieldImpl: public Field
{
public:
    FieldImpl() = default;
    FieldImpl(QString name, double factor): Field(name), factor_(factor) {
        static_assert(std::is_integral<T>::value, "");
        static_assert(sizeof (T) <= sizeof(uint64_t), "");
        Q_ASSERT(factor_ != 0.0);
    }

    // Field interface
public:
    virtual QByteArray serialize(const QVariant var) const override {
        QByteArray bytes;
        bytes.resize(sizeof(T));
        /* @TODO 针对 64位整数会丢失精度
         * 但如果终端使用 double 来表达数据，那丢失精度是无法避免的 */
        double value = var.toDouble();
        value *= factor_;
        if constexpr (std::is_same_v<T, int64_t>) {
            qToBigEndian<qint64>(value , bytes.data());
        }
        else if constexpr (std::is_same_v<T, uint64_t>) {
            qToBigEndian<quint64>(value , bytes.data());
        }
        else {
            qToBigEndian<T>(value , bytes.data());
        }
        return bytes;
    }
    virtual QVariant deserialize(const QByteArray bytes) const override {
        if(bytes.size() < sizeof(T))
            return QVariant();
        const void* src = static_cast<const void*>(bytes.data());
        T value;
        /* NOTE 在部分平台中（Linux + Qt5.13.2），int64_t 和 qint64 不是相同的类型
         * 且 qToBigEndian() / qFromBigEndian() 使用的 qbswap() 未针对 int64_t / uint64_t 实例化 */
        if constexpr (std::is_same_v<T, int64_t>) {
            value = qFromBigEndian<qint64>(src);
        }
        else if constexpr (std::is_same_v<T, uint64_t>) {
            value = qFromBigEndian<quint64>(src);
        }
        else {
            // 虽然 qFromBigEndian 手册中未提及 int8_t/uint8_t, 但实现是存在这两个类型的特例化的
            value = qFromBigEndian<T>(src);
        }
        return (factor_ != 1.0) ? value / factor_ : value;
    }
    virtual unsigned serializedSize() const  override {
        return sizeof (T);
    }

private:
    double factor_ = 1.0;
};


NavProtocol::NavProtocol()
{

}

NavProtocol::~NavProtocol()
{
    for(int i=0; i<fields.size(); ++i)
    {
        delete fields.at(i);
    }
}

QByteArray NavProtocol::serialize(const QVector<QVariant> values) const
{
    QByteArray bytes;
    for(int i=0; i< fields.size(); ++i)
    {
        bytes += fields[i]->serialize(values.at(i));
    }
    return bytes;
}

QVector<QVariant> NavProtocol::deserialize(const QByteArray bytes) const
{
    QVector<QVariant> values;
    int idx = 0;
    for(int i=0; i< fields.size(); ++i)
    {
        QVariant var = fields[i]->deserialize(bytes.mid(idx));
        idx += fields[i]->serializedSize();
        values.push_back(var);
    }
    return values;
}

QStringList NavProtocol::allFieldName() const
{
    QStringList names;
    for (int i = 0; i < fields.size(); ++i) {
        names.push_back(fields[i]->name());
    }
    return names;
}

template<typename T>
void NavProtocol::addField(QString name, double factor) {
    auto * field = new FieldImpl<T>(name, factor);
    fields.push_back(field);
}

template void NavProtocol::addField<  int8_t>(QString, double);
template void NavProtocol::addField< uint8_t>(QString, double);
template void NavProtocol::addField< int16_t>(QString, double);
template void NavProtocol::addField<uint16_t>(QString, double);
template void NavProtocol::addField< int32_t>(QString, double);
template void NavProtocol::addField<uint32_t>(QString, double);
template void NavProtocol::addField< int64_t>(QString, double);
template void NavProtocol::addField<uint64_t>(QString, double);

//template void NavProtocol::addField<  qint8>(QString, double);
//template void NavProtocol::addField< quint8>(QString, double);
//template void NavProtocol::addField< qint16>(QString, double);
//template void NavProtocol::addField<quint16>(QString, double);
//template void NavProtocol::addField< qint32>(QString, double);
//template void NavProtocol::addField<quint32>(QString, double);
template void NavProtocol::addField< qint64>(QString, double);
template void NavProtocol::addField<quint64>(QString, double);

NavData::NavData()
{

}

QByteArray NavData::serialize() const {
    return protocol->serialize(this->values_);
}

void NavData::deserialize(const QByteArray bytes) {
    values_ = protocol->deserialize(bytes);
}

QSharedPointer<const NavProtocol> NavData::getProtocol() const
{
    return protocol;
}

#include <QDebug>

void NavData::setProtocol(QSharedPointer<const NavProtocol> ptr)
{
    if(ptr.data() == protocol.data())
        return;
    name2idx_.clear();
    values_.clear();
    protocol = ptr;
    QStringList fields = protocol->allFieldName();
    values_.resize(fields.size());
    for (int i = 0; i < fields.size(); ++i) {
        QString name = fields[i];
        if(!name.isEmpty()) {
            QMap<QString, int>::iterator it = name2idx_.find(name);
            if(it != name2idx_.end())
                qWarning() << __FUNCTION__ << ':' << __LINE__
                           << "There is already an item with the key:" << name;
        }
        name2idx_.insert(name, i);
    }
}

template<typename T>
T NavData::getVal(QString name, T defaultVal /*= 0*/) const {
    if(std::is_arithmetic<T>::value) {
        QVariant var = getVal(name);
        switch (var.type()) {
        case QVariant::Int:
            return var.toInt();
        case QVariant::UInt:
            return var.toUInt();
        case QVariant::LongLong:
            return var.toLongLong();
        case QVariant::ULongLong:
            return var.toULongLong();
        case QVariant::Double:
            return var.toDouble();
        default:
            break;
        }
    }
    return defaultVal;
}

template  int8_t NavData::getVal< int8_t>(QString,  int8_t) const;
template uint8_t NavData::getVal<uint8_t>(QString, uint8_t) const;
template  int16_t NavData::getVal<int16_t>(QString,  int16_t) const;
template uint16_t NavData::getVal<uint16_t>(QString, uint16_t) const;
template  int32_t NavData::getVal< int32_t>(QString,  int32_t) const;
template uint32_t NavData::getVal<uint32_t>(QString, uint32_t) const;
template  int64_t NavData::getVal< int64_t>(QString,  int64_t) const;
template uint64_t NavData::getVal<uint64_t>(QString, uint64_t) const;
template  float NavData::getVal<float>(QString,  float) const;
template double NavData::getVal<double>(QString, double) const;
