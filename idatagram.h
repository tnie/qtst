#ifndef IDATAGRAM_H
#define IDATAGRAM_H

#include <QNetworkDatagram>

class IDatagram{
public:
    IDatagram();
    virtual ~IDatagram();
    virtual bool verify(QNetworkDatagram) const = 0;
    virtual QString toString() const { return QString(); }

    QNetworkDatagram getDatagram() const;
    void setDatagram(QNetworkDatagram);

protected:
    QNetworkDatagram datagram;
};

class Factory {
    Factory() = default;
    Factory(const Factory&) = delete;
    Factory& operator=(const Factory&) = delete;
    Factory(Factory&&) = delete;
    Factory& operator=(Factory&&) = delete;

public:
    static Factory& get() {
        static Factory instance;
        return instance;
    }
    void insert(QString key, IDatagram*);
    IDatagram *value(QString key) const;
    QList<IDatagram*> values() const;

    template<typename T>
    struct register_t
    {
        register_t(QString key)
        {
            Factory::get().insert(key, new T());
        }
    };

private:
    QMap<QString, IDatagram*> map_;
};

#define REGISTER_IDATAGRAM_VNAME(T) reg_datagram_##T##_
#define REGISTER_IDATAGRAM(T, key) static Factory::register_t<T> REGISTER_IDATAGRAM_VNAME(T)(key);

#endif // IDATAGRAM_H
