// networkmonitor.cpp

#include "networkmonitor.h"

NetworkMonitor::NetworkMonitor(QObject* parent)
    : QObject(parent)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
    const bool loaded =
#if QT_VERSION >= QT_VERSION_CHECK(6, 4, 0)
        QNetworkInformation::loadBackendByFeatures(
            QNetworkInformation::Feature::Reachability);
#else
        QNetworkInformation::load(QNetworkInformation::Feature::Reachability);
#endif
    if (loaded) {
        QNetworkInformation* info = QNetworkInformation::instance();
        m_online = info->reachability() != QNetworkInformation::Reachability::Disconnected;
        connect(info, &QNetworkInformation::reachabilityChanged, this,
                [this](QNetworkInformation::Reachability reachability) {
                    setOnline(reachability !=
                              QNetworkInformation::Reachability::Disconnected);
                });
    }
#else
    // QNetworkInformation is unavailable before Qt 6.3: assume online.
    m_online = true;
#endif
}

bool NetworkMonitor::isOnline() const
{
    return m_online;
}

void NetworkMonitor::setOnline(bool online)
{
    if (m_online == online) {
        return;
    }
    m_online = online;
    emit onlineChanged(m_online);
}
