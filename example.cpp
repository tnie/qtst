#include "example.h"
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator>

namespace randomGen {
inline bool gender() {
    return QRandomGenerator::global()->generate() & 0x1;
}
inline double balance() {
    double val = QRandomGenerator::global()->generateDouble();
    val *= QRandomGenerator::global()->generate64();
    return val;
}
}

QByteArray produce::student()
{
    Student student;
    student.setGender(randomGen::gender());
    student.setScore(QRandomGenerator::global()->generateDouble() * 100.0);
    student.setAge(QRandomGenerator::global()->generate() % 20);
    student.setTimestamp(QDateTime::currentSecsSinceEpoch());
    return student.serialize();
}

QByteArray produce::worker()
{
    Worker worker;
    worker.setGender(randomGen::gender());
    worker.setAge(20 + QRandomGenerator::global()->generate() % 100);
    worker.setBalance(randomGen::balance());
    return worker.serialize();
}

QByteArray produce::random()
{
    return randomGen::gender() ? worker() : student();
}

QString Student::toString() const {
    QString time = QDateTime::fromSecsSinceEpoch(timestamp()).time().toString();
    return QString("%1 score %2 time %3").arg(Person::toString()).arg(getScore()).arg(time);
}
