// crashserver.h
//
// Thin QObject wrapper around google_breakpad::CrashGenerationServer (the
// out-of-process server side). Breakpad callbacks fire on its internal worker
// thread; we forward them as Qt signals, which are queued to the GUI thread.

#pragma once

#include <QObject>
#include <QString>
#include <string>

namespace google_breakpad {
class CrashGenerationServer;
class ClientInfo;
} // namespace google_breakpad

class CrashServer final : public QObject {
    Q_OBJECT

public:
    explicit CrashServer(QObject* parent = nullptr);
    ~CrashServer() override;

    CrashServer(const CrashServer&)            = delete;
    CrashServer& operator=(const CrashServer&) = delete;

    // Starts listening on |pipeName| and writes dumps into |dumpDir|.
    // Returns std::unexpected(QString) with a readable error on failure.
    std::expected<void, QString> start(const QString& pipeName, const QString& dumpDir);

    void stop();

signals:
    // A client requested a dump; Breakpad wrote it to |dumpPath|.
    void dumpRequested(quint32 pid, const QString& dumpPath);
    // A client process exited; |exitCode| is 0 for a clean shutdown.
    void clientExited(quint32 pid, int exitCode);

private:
    static void onDumpRequest(void* ctx,
                              const google_breakpad::ClientInfo* info,
                              const std::wstring* path);
    static void onClientExit(void* ctx,
                             const google_breakpad::ClientInfo* info);

    std::unique_ptr<google_breakpad::CrashGenerationServer> m_server;
};
