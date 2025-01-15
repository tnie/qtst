#ifndef EXAMPLE_H
#define EXAMPLE_H

#include "navprotocol.h"

// 报文结构体格式如下:
// -----------------------
// 大小    | 描述   | 权值 |
// --------|-------|------|
// 1 byte  | 性别   |     |
// 1 byte  | 年龄   |     |
// -----------------------

class Person : public NavData {
public:
    struct Protocol: public NavProtocol
    {
        Protocol() {
            addField<quint8>("性别", 1);
            addField<uint8_t>("年龄");
        }
    };

    Person() {
        auto protocol = QSharedPointer<const NavProtocol>(new Protocol);
        this->setProtocol(protocol);
    }
    int getAge() const {
        return getVal<int>("年龄");
    }
    void setAge(int age) {
        setVal("年龄", age);
    }
    bool isFemale() const {
        return getVal("性别").toInt();
    }
    void setGender(bool female) {
        setVal("性别", female ? 1 : 0);
    }
    virtual QString toString() const {
        return QString("[%1] age %2").arg(isFemale() ? 'F' : 'M').arg(getAge());
    }
};

// 报文结构体格式如下:
// -----------------------
// 大小    | 描述   | 权值 |
// --------|-------|------|
// 1 byte  | 性别   |     |
// 1 byte  | 年龄   |     |
// 8 byte  | 余额   | 100 |
// -----------------------

class Worker : public Person
{
public:
    struct Protocol: public Person::Protocol
    {
        Protocol() {
            addField<int64_t>("余额", 100);
        }
    };
    Worker() {
        auto protocol = QSharedPointer<const NavProtocol>(new Protocol);
        this->setProtocol(protocol);
    }
    double getBalance() const {
        return getVal<double>("余额");
    }
    void setBalance(double balance) {
        setVal("余额", balance);
    }
    virtual QString toString() const override {
        return QString("%1 balance %2").arg(Person::toString()).arg(getBalance());
    }
};

// 报文结构体格式如下:
// -----------------------
// 大小    | 描述   | 权值 |
// --------|-------|------|
// 1 byte  | 性别   |     |
// 1 byte  | 年龄   |     |
// 4 byte  | 分数   | 10  |
// 8 byte  | 时戳   |     |
// -----------------------

class Student : public Person {
public:
    struct Protocol : public Person::Protocol
    {
        Protocol() {
            addField<int32_t>("分数", 10);
            addField<qint64>("时戳");
        }
    };

    Student() {
        auto protocol = QSharedPointer<const NavProtocol>(new Protocol);
        this->setProtocol(protocol);
    }

    void setScore(double score) {
        setVal("分数", score);
    }
    double getScore() const {
        return getVal("分数").toDouble();
    }
    void setTimestamp(qint64 ts) {
        setVal("时戳", ts);
    }
    qint64 timestamp() const {
        return getVal("时戳").toLongLong();
    }
    virtual QString toString() const override;
};

namespace produce {
QByteArray student();
QByteArray worker();
QByteArray random();
}

#endif // EXAMPLE_H
