// senders.cpp

#include "senders.h"

#include <QFile>
#include <QUrl>

#include "stringutils.h"

std::expected<void, QString> EmailUtils::openInMailClient(const QString& recipient,
                                                          const QString& subject,
                                                          const QString& body)
{
    QUrl url;
    url.setScheme(QStringLiteral("mailto"));
    url.setPath(recipient);

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("subject"), subject);
    query.addQueryItem(QStringLiteral("body"), body);
    url.setQuery(query);

    if (!QDesktopServices::openUrl(url)) {
        return std::unexpected(QStringLiteral("no mail client available"));
    }
    return {};
}

Uploader::Uploader(QObject* parent)
    : QObject(parent)
{}

void Uploader::send(const CrashReport& report)
{
    const QUrl url(StringUtils::fromUtf8(constants::uploadUrl));
    if (!url.isValid()) {
        emit finished(false, QStringLiteral("invalid upload URL"));
        return;
    }

    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart productPart;
    productPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant(QStringLiteral("form-data; name=\"%1\"")
                                       .arg(StringUtils::fromUtf8(constants::uploadFieldProduct))));
    productPart.setBody(report.appName.toUtf8());
    multiPart->append(productPart);

    QHttpPart reportPart;
    reportPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                         QVariant(QStringLiteral("form-data; name=\"%1\"")
                                      .arg(StringUtils::fromUtf8(constants::uploadFieldReport))));
    reportPart.setBody(buildSummary(report).toUtf8());
    multiPart->append(reportPart);

    if (report.hasDump) {
        QHttpPart dumpPart;
        dumpPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                           QVariant(QStringLiteral("form-data; name=\"%1\"; filename=\"crash.dmp\"")
                                        .arg(StringUtils::fromUtf8(constants::uploadFieldDump))));
        dumpPart.setHeader(QNetworkRequest::ContentTypeHeader,
                           QVariant(QStringLiteral("application/octet-stream")));

        auto* dumpFile = new QFile(report.dumpPath);
        if (!dumpFile->open(QIODevice::ReadOnly)) {
            dumpFile->deleteLater();
            multiPart->deleteLater();
            emit finished(false, dumpFile->errorString());
            return;
        }
        dumpFile->setParent(multiPart);
        dumpPart.setBodyDevice(dumpFile);
        multiPart->append(dumpPart);
    }

    QNetworkRequest request(url);
    QNetworkReply* reply = m_manager.post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit finished(true, QString());
        }
        else {
            emit finished(false, reply->errorString());
        }
    });
}

QString Uploader::buildSummary(const CrashReport& report) const
{
    QStringList lines;
    lines << QStringLiteral("product: %1").arg(report.appName);
    lines << QStringLiteral("pid: %1").arg(report.pid);
    lines << QStringLiteral("time: %1").arg(report.crashTime.toString(Qt::ISODate));

    if (report.hasDump) {
        lines << QStringLiteral("exception: %1 (%2)")
                     .arg(report.exceptionCode, report.exceptionName);
        lines << QStringLiteral("address: %1").arg(report.exceptionAddress);
        lines << QStringLiteral("module: %1 + %2")
                     .arg(report.moduleName, report.moduleOffset);
        lines << QStringLiteral("dump: %1 (%2 bytes)")
                     .arg(report.dumpPath).arg(report.dumpSize);
    }
    else {
        lines << QStringLiteral("exit code: %1").arg(report.exitCode);
        lines << QStringLiteral("no dump available");
    }

    return lines.join(QStringLiteral("\n"));
}
