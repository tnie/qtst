#ifndef STUDENT_H
#define STUDENT_H

#include "navprotocol.h"

// 报文结构体格式如下:
// -----------------------
// 大小    | 描述   | 权值 |
// --------|-------|------|
// 4 byte  | 学号   |     |
// 1 byte  | 性别   |     |
// 2 byte  | 数学   | 10  |
// 2 byte  | 英语   | 10  |
// 4 byte  | 总分   | 10  |
// 8 byte  | 时戳   |     |
// -----------------------

class Student : public NavData {
public:
    static QSharedPointer<const NavProtocol> getProtocol()
    {
        static QSharedPointer<const NavProtocol> navProtocol;
        if(nullptr == navProtocol ){
            NavProtocol * protocol = new NavProtocol;
            protocol->addField<quint32>("学号", 1);
            protocol->addField<quint8>("性别", 1);
            protocol->addField<qint16>("数学", 10);
            protocol->addField<qint16>("英语", 10);
            protocol->addField<qint16>("总分", 10);
            protocol->addField<qint64>("时戳");
            navProtocol = QSharedPointer<const NavProtocol>(protocol);
        }
        return navProtocol;
    }
    Student() {
        this->setProtocol(getProtocol());
    }

    void setNum(int num) {
        setVal("学号", num);
    }
    int getNum() const {
        return getVal("学号").toInt();
    }
    void setGender(bool female) {
        setVal("性别", female ? 1 : 0);
    }
    bool isFemale() const {
        return getVal("性别").toInt();
    }
    void setMath(double score) {
        setVal("数学", score);
    }
    double getMath() const {
        return getVal<double>("数学");
    }
    void setEng(double score) {
        setVal("英语", score);
    }
    double getEng() const {
        return getVal<double>("英语");
    }
    // void setSum(double score) {
    //     setVal("总分", score);
    // }
    // double getSum() const {
    //     return getDoubleVal("总分");
    // }
    void setTimestamp(qint64 ts) {
        setVal("时戳", ts);
    }
    qint64 timestamp() const {
        return getVal("时戳").toLongLong();
    }
};

#endif // STUDENT_H
