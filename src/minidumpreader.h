// minidumpreader.h
//
// Lightweight minidump inspection: reads the exception stream and module list
// directly from the raw minidump (using Breakpad's minidump_format.h), without
// the heavy symbol-processing library.

#pragma once

#include <expected>

#include "crashreport.h"

class MinidumpUtils {
public:
    // Parses |dumpPath| and returns a CrashReport with dump path/size plus the
    // exception and faulting-module fields filled in.
    // Returns std::unexpected(QString) with a readable error on failure.
    static std::expected<CrashReport, QString> read(const QString& dumpPath);

private:
    static std::expected<void, QString> readBytes(QFile& file,
                                                  qint64 offset,
                                                  void* dst,
                                                  qint64 size);
    static std::expected<QString, QString> readModuleName(QFile& file, quint32 rva);
    static QString describeExceptionCode(quint32 code);
    static QString formatAddress(quint64 value);
};
