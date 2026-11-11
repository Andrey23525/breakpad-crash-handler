// senders.h
//
// Two delivery paths for a captured snapshot:
//   * EmailUtils  — opens the user's default mail client (mailto:).
//   * Uploader    — HTTP multipart POST to the developer endpoint (async).

#pragma once

#include <expected>

#include <QObject>

#include "crashreport.h"

class EmailUtils {
public:
    // Composes a mailto: URL from the given fields and hands it to the OS.
    // Returns std::unexpected(QString) if no mail client could be opened.
    static std::expected<void, QString> openInMailClient(const QString& recipient,
                                                         const QString& subject,
                                                         const QString& body);
};

class Uploader final : public QObject {
    Q_OBJECT

public:
    explicit Uploader(QObject* parent = nullptr);

    // Uploads |report| (with the dump attached when present).
    void send(const CrashReport& report);

signals:
    // |detail| is a technical error string (not translated) when !ok.
    void finished(bool ok, const QString& detail);

private:
    QString buildSummary(const CrashReport& report) const;

    QNetworkAccessManager m_manager;
};
