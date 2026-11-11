// crashserver.cpp

#include "crashserver.h"

#include <windows.h>

#include "client/windows/crash_generation/crash_generation_server.h"
#include "client/windows/crash_generation/client_info.h"

CrashServer::CrashServer(QObject* parent)
    : QObject(parent)
{}

CrashServer::~CrashServer()
{
    stop();
}

std::expected<void, QString> CrashServer::start(const QString& pipeName, const QString& dumpDir)
{
    if (pipeName.isEmpty() || dumpDir.isEmpty()) {
        return std::unexpected(QStringLiteral("empty pipe name or dump directory"));
    }

    const std::wstring pipe = pipeName.toStdWString();
    const std::wstring dumps = dumpDir.toStdWString();

    m_server = std::make_unique<google_breakpad::CrashGenerationServer>(
        pipe,
        nullptr,                    // no custom pipe security attributes
        nullptr, nullptr,           // OnClientConnected
        &CrashServer::onDumpRequest, this,
        &CrashServer::onClientExit, this,
        nullptr, nullptr,           // OnClientUploadRequest
        true,                       // generate_dumps
        &dumps);

    if (!m_server->Start()) {
        m_server.reset();
        return std::unexpected(QStringLiteral("CrashGenerationServer::Start() failed"));
    }

    return {};
}

void CrashServer::stop()
{
    m_server.reset();
}

void CrashServer::onDumpRequest(void* ctx,
                                const google_breakpad::ClientInfo* info,
                                const std::wstring* path)
{
    auto* self = static_cast<CrashServer*>(ctx);
    if (!self || !info) {
        return;
    }

    const QString dumpPath = path ? QString::fromStdWString(*path) : QString();
    emit self->dumpRequested(info->pid(), dumpPath);
}

void CrashServer::onClientExit(void* ctx,
                               const google_breakpad::ClientInfo* info)
{
    auto* self = static_cast<CrashServer*>(ctx);
    if (!self || !info) {
        return;
    }

    int exitCode = -1;
    if (info->process_handle()) {
        DWORD code = 0;
        if (GetExitCodeProcess(info->process_handle(), &code)) {
            exitCode = static_cast<int>(code);
        }
    }

    emit self->clientExited(info->pid(), exitCode);
}
