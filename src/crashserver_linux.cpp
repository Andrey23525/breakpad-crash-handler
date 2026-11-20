#include "crashserver.h"
#include <QDir>
#include <QFile>
#include <unistd.h>
#include <sys/socket.h>
#include "client/linux/crash_generation/crash_generation_server.h"
#include "client/linux/crash_generation/client_info.h"

CrashServer::CrashServer(QObject* parent) : QObject(parent) {}
CrashServer::~CrashServer() { stop(); }
std::expected<void, QString> CrashServer::start(const QString& channel, const QString& dumpDir)
{
    stop();
    bool ok = false;
    const int fd = channel.toInt(&ok);
    int type = 0;
    socklen_t size = sizeof(type);
    if (!ok || fd < 3 || getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &size) || type != SOCK_SEQPACKET)
        return std::unexpected(QStringLiteral("--server-fd must be an inherited Breakpad sequenced-packet socket"));
    if (dumpDir.isEmpty() || !QDir().mkpath(dumpDir))
        return std::unexpected(QStringLiteral("Cannot create dump directory"));
    m_fd = dup(fd);
    if (m_fd < 0)
        return std::unexpected(QStringLiteral("Cannot duplicate report socket"));
    const std::string path = QFile::encodeName(QDir(dumpDir).absolutePath()).toStdString();
    m_server = std::make_unique<google_breakpad::CrashGenerationServer>(
        m_fd, &CrashServer::onDumpRequest, this, nullptr, nullptr, true, &path);
    if (!m_server->Start()) {
        stop();
        return std::unexpected(QStringLiteral("CrashGenerationServer::Start() failed"));
    }
    return {};
}
void CrashServer::stop()
{
    m_server.reset();
    if (m_fd >= 0) close(m_fd);
    m_fd = -1;
}
void CrashServer::onDumpRequest(void* ctx, const google_breakpad::ClientInfo* info,
                              const DumpPath* path)
{
    if (ctx && info && path)
        emit static_cast<CrashServer*>(ctx)->dumpRequested(info->pid(), QFile::decodeName(path->c_str()));
}
