#ifndef NAVPROTOCOL_H
#define NAVPROTOCOL_H

#include <QString>
#include <QVariant>

class Field;

class NavProtocol
{
    NavProtocol(const NavProtocol&);
    NavProtocol& operator=(const NavProtocol&);
    NavProtocol(NavProtocol&&);
    NavProtocol& operator=(NavProtocol&&);
public:
    NavProtocol();
    virtual ~NavProtocol();

    template<typename T>
    void addField(QString name, double factor = 1.0);
    QByteArray serialize(const QVector<QVariant> values) const;
    QVector<QVariant> deserialize(const QByteArray bytes) const;

    QStringList allFieldName() const;

private:
    QVector<Field*> fields;
};

#include <QSharedPointer>

class NavData
{
public:
    NavData();
    virtual ~NavData() {}

    QSharedPointer<const NavProtocol> getProtocol() const;
    void setProtocol(QSharedPointer<const NavProtocol> ptr);

    QByteArray serialize() const;
    void deserialize(const QByteArray bytes);

protected:
    template<typename T>
    void setVal(QString name, T val) {
        int idx = name2idx_.value(name);
        values_[idx] = QVariant(val);
    }
    QVariant getVal(QString name) const {
        int idx = name2idx_.value(name, -1);
        return (idx < 0) ? QVariant() : values_[idx];
    }
    // NOTE 内部存在强制类型转换
    template<typename T>
    T getVal(QString name, T defaultVal = 0) const;

protected:
    /* 1. 属性名称方便理解，却并非获取属性值的唯一方法
     * 2. 直接使用索引获取属性值更加高效，但维护代码的成本较高
     * 3. 属性名称并非协议的构成部分！！ */
    QMap<QString, int> name2idx_;
private:
    QVector<QVariant> values_;
    QSharedPointer<const NavProtocol> protocol = nullptr;
};

#endif // NAVPROTOCOL_H
