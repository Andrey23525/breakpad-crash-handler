// main.cpp

#include <QApplication>
#include <QCommandLineParser>
#include <QFont>
#include <QLocale>
#include <QMessageBox>
#include <QStyleFactory>
#include <QTranslator>

#include "crashserver.h"
#include "handlerconfiguration.h"
#include "mainwindow.h"
#include "networkmonitor.h"
#include "stringutils.h"

// Installs the translation matching the requested language (or the system
// locale when none is forced) from the embedded resource. Source language is
// English; Russian and Simplified Chinese ship via crash_handler_ru_RU.qm and
// crash_handler_zh_CN.qm.
static void installTranslators(QApplication& app, const QString& forcedLanguage)
{
    static QTranslator translator;

    const QStringList languages = forcedLanguage.isEmpty()
        ? QLocale().uiLanguages()
        : QStringList{forcedLanguage};

    for (const QString& lang : languages) {
        const QString full = QLocale(lang).name();
        if (translator.load(QStringLiteral("crash_handler_") + full,
                            QStringLiteral(":/translations"))) {
            app.installTranslator(&translator);
            return;
        }
        const QString shortName = lang.left(2);
        if (translator.load(QStringLiteral("crash_handler_") + shortName,
                            QStringLiteral(":/translations"))) {
            app.installTranslator(&translator);
            return;
        }
    }
}

// Values supplied by the protected application win over the compiled defaults.
static QString valueOrDefault(const QString& value, std::string_view fallback)
{
    return value.isEmpty() ? StringUtils::fromUtf8(fallback) : value;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Breakpad Crash Handler"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setQuitOnLastWindowClosed(true);

    // Windows 11 look: use the modern style when available and the Segoe UI
    // font that Fluent uses.
    if (QStyleFactory::keys().contains(QStringLiteral("windows11"))) {
        QApplication::setStyle(QStringLiteral("windows11"));
    }
#ifdef Q_OS_WIN
    QApplication::setFont(QFont(QStringLiteral("Segoe UI"), 9));
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Out-of-process Breakpad crash handler"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption pipeOption(QStringLiteral("pipe"),
                                  QStringLiteral("Named pipe of the protected client"),
                                  QStringLiteral("name"));
    QCommandLineOption dumpsOption(QStringLiteral("dumps"),
                                   QStringLiteral("Directory to write minidumps into"),
                                   QStringLiteral("dir"));
    QCommandLineOption parentOption(QStringLiteral("parent"),
                                    QStringLiteral("PID of the protected client"),
                                    QStringLiteral("pid"));
    QCommandLineOption appNameOption(QStringLiteral("app-name"),
                                     QStringLiteral("Display name of the protected client"),
                                     QStringLiteral("name"));
    QCommandLineOption supportEmailOption(QStringLiteral("support-email"),
                                          QStringLiteral("Support email for the email action"),
                                          QStringLiteral("address"));
    QCommandLineOption uploadUrlOption(QStringLiteral("upload-url"),
                                       QStringLiteral("Endpoint for the developer upload"),
                                       QStringLiteral("url"));
    QCommandLineOption uploadFieldProductOption(QStringLiteral("upload-field-product"),
                                                QStringLiteral("Multipart field for the product"),
                                                QStringLiteral("name"));
    QCommandLineOption uploadFieldReportOption(QStringLiteral("upload-field-report"),
                                               QStringLiteral("Multipart field for the report"),
                                               QStringLiteral("name"));
    QCommandLineOption uploadFieldDumpOption(QStringLiteral("upload-field-dump"),
                                             QStringLiteral("Multipart field for the dump"),
                                             QStringLiteral("name"));
    QCommandLineOption langOption(QStringLiteral("lang"),
                                  QStringLiteral("Force the UI language (e.g. en, ru_RU)"),
                                  QStringLiteral("locale"));
    parser.addOptions({pipeOption, dumpsOption, parentOption, appNameOption,
                       supportEmailOption, uploadUrlOption, uploadFieldProductOption,
                       uploadFieldReportOption, uploadFieldDumpOption, langOption});
#ifdef Q_OS_LINUX
    QCommandLineOption serverFdOption(QStringLiteral("server-fd"),
        QStringLiteral("Inherited Breakpad server socket descriptor"), QStringLiteral("fd"));
    parser.addOption(serverFdOption);
#endif
    parser.process(app);

    installTranslators(app, parser.value(langOption));

    QString pipeName = parser.value(pipeOption);
    if (pipeName.isEmpty()) {
        pipeName = QStringLiteral("\\\\.\\pipe\\")
            + StringUtils::fromUtf8(constants::pipeName);
    }

#ifdef Q_OS_LINUX
    pipeName = parser.value(serverFdOption);
#endif

    const QString dumpDir = parser.value(dumpsOption);

    if (dumpDir.isEmpty()) {
        QMessageBox::critical(nullptr, QStringLiteral("Breakpad Crash Handler"),
                              QStringLiteral("Missing --dumps argument."));
        return 1;
    }

    HandlerConfiguration configuration;
    configuration.productName = valueOrDefault(parser.value(appNameOption),
                                               constants::productName);
    configuration.supportEmail = valueOrDefault(parser.value(supportEmailOption),
                                                constants::supportEmail);
    configuration.uploadUrl = valueOrDefault(parser.value(uploadUrlOption),
                                             constants::uploadUrl);
    configuration.uploadFieldProduct = valueOrDefault(parser.value(uploadFieldProductOption),
                                                     constants::uploadFieldProduct);
    configuration.uploadFieldReport = valueOrDefault(parser.value(uploadFieldReportOption),
                                                    constants::uploadFieldReport);
    configuration.uploadFieldDump = valueOrDefault(parser.value(uploadFieldDumpOption),
                                                   constants::uploadFieldDump);

    auto* server = new CrashServer(&app);
    auto* monitor = new NetworkMonitor(&app);

    // The window is created lazily — only when the client crashes or exits
    // abnormally — so the handler stays invisible while the app is healthy.
    MainWindow* window = nullptr;
    auto ensureWindow = [&]() -> MainWindow* {
        if (!window) {
            window = new MainWindow(configuration, monitor);
            window->setAttribute(Qt::WA_DeleteOnClose);
        }
        return window;
    };

    QObject::connect(server, &CrashServer::dumpRequested, &app,
                     [&](quint32 pid, const QString& path) {
                         ensureWindow()->onDumpRequested(pid, path);
                     });

    QObject::connect(server, &CrashServer::clientExited, &app,
                     [&](quint32 pid, int code) {
                         if (code == 0) {
                             // Clean shutdown: nothing to report, exit quietly.
                             QApplication::quit();
                             return;
                         }
                         ensureWindow()->onClientExited(pid, code);
                     });

    const auto started = server->start(pipeName, dumpDir);
    if (!started) {
        QMessageBox::critical(nullptr, QStringLiteral("Breakpad Crash Handler"),
                              started.error());
        return 1;
    }

    return app.exec();
}
