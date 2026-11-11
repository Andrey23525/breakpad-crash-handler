// crashreport.h
//
// Everything the UI needs to describe one crash of the protected client.

#pragma once

#include <QDateTime>
#include <QString>

struct CrashReport {
    quint32 pid = 0;
    QString appName;

    // Set when a minidump was produced by the server.
    bool   hasDump  = false;
    QString dumpPath;
    qint64  dumpSize = 0;

    // Only meaningful for the "client exited without a dump" case.
    int exitCode = 0;

    // Filled by MinidumpUtils::read() when hasDump is true.
    QString exceptionCode;    // e.g. "0xC0000005"
    QString exceptionName;    // e.g. "ACCESS_VIOLATION"
    QString exceptionAddress; // e.g. "0x00007FF6DEADBEEF"
    quint32 threadId = 0;
    QString moduleName;       // basename of the faulting module
    QString moduleOffset;     // exceptionAddress - module base

    QDateTime crashTime;
};
