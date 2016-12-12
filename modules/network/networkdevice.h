#ifndef NETWORKDEVICE_H
#define NETWORKDEVICE_H

#include <QObject>
#include <QJsonObject>

namespace dcc {

namespace network {

class NetworkDevice : public QObject
{
    Q_OBJECT

public:
    enum DeviceType
    {
        Wired,
        Wireless,
    };

public:
    explicit NetworkDevice(const DeviceType type, const QJsonObject &data, QObject *parent = 0);
    explicit NetworkDevice(const NetworkDevice &device);

    bool enabled() const { return m_enabled; }
    void setEnabled(const bool enabled);
    DeviceType type() const { return m_type; }
    const QJsonObject data() const { return m_data; }
    const QString path() const;

signals:
    void enableChanged(const bool enabled) const;

private:
    const DeviceType m_type;
    const QJsonObject m_data;

    bool m_enabled;
};

}   // namespace network

}   // namespace dcc

#endif // NETWORKDEVICE_H
