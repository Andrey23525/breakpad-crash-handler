// networkmonitor.h
//
// Tracks network reachability so the UI can disable the "send to developer"
// action while offline.

#pragma once

#include <QObject>

class NetworkMonitor final : public QObject {
    Q_OBJECT

public:
    explicit NetworkMonitor(QObject* parent = nullptr);

    [[nodiscard]] bool isOnline() const;

signals:
    void onlineChanged(bool online);

private:
    void setOnline(bool online);

    bool m_online = true;
};
