// minidumpreader.cpp

#include "minidumpreader.h"

#include <QFile>

#include "google_breakpad/common/minidump_format.h"

std::expected<CrashReport, QString> MinidumpUtils::read(const QString& dumpPath)
{
    QFile file(dumpPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::unexpected(
            QStringLiteral("cannot open dump: %1").arg(file.errorString()));
    }

    CrashReport report;
    report.dumpPath = dumpPath;
    report.dumpSize = file.size();
    report.hasDump  = true;

    MDRawHeader header{};
    auto result = readBytes(file, 0, &header, sizeof(header));
    if (!result) {
        return std::unexpected(QStringLiteral("bad header: %1").arg(result.error()));
    }
    if (header.signature != MD_HEADER_SIGNATURE) {
        return std::unexpected(QStringLiteral("not a minidump (bad signature)"));
    }

    const quint32 directoryRva = header.stream_directory_rva;

    for (quint32 i = 0; i < header.stream_count; ++i) {
        MDRawDirectory directory{};
        const qint64 dirOffset =
            static_cast<qint64>(directoryRva) + i * sizeof(MDRawDirectory);
        result = readBytes(file, dirOffset, &directory, sizeof(directory));
        if (!result) {
            continue;
        }

        const qint64 rva = directory.location.rva;

        if (directory.stream_type == MD_EXCEPTION_STREAM) {
            MDRawExceptionStream stream{};
            result = readBytes(file, rva, &stream, sizeof(stream));
            if (!result) {
                continue;
            }
            const quint32 code = stream.exception_record.exception_code;
            const quint64 addr = stream.exception_record.exception_address;

            report.exceptionCode    = formatAddress(code);
            report.exceptionName    = describeExceptionCode(code);
            report.exceptionAddress = formatAddress(addr);
            report.threadId         = stream.thread_id;
        }
        else if (directory.stream_type == MD_MODULE_LIST_STREAM) {
            quint32 count = 0;
            result = readBytes(file, rva, &count, sizeof(count));
            if (!result) {
                continue;
            }

            const quint64 faultingAddr = report.exceptionAddress.isEmpty()
                ? 0
                : report.exceptionAddress.mid(2).toULongLong(nullptr, 16);

            for (quint32 m = 0; m < count; ++m) {
                const qint64 moduleOffset =
                    static_cast<qint64>(rva) + 4 + m * MD_MODULE_SIZE;

                quint64 base = 0;
                quint32 size = 0;
                quint32 nameRva = 0;
                (void)readBytes(file, moduleOffset, &base, sizeof(base));
                (void)readBytes(file, moduleOffset + 8, &size, sizeof(size));
                (void)readBytes(file, moduleOffset + 20, &nameRva, sizeof(nameRva));

                if (faultingAddr == 0 || faultingAddr < base ||
                    faultingAddr >= base + size) {
                    continue;
                }

                const auto name = readModuleName(file, nameRva);
                report.moduleName = name ? *name : QString();
                report.moduleOffset = formatAddress(faultingAddr - base);
                break;
            }
        }
    }

    return report;
}

std::expected<void, QString> MinidumpUtils::readBytes(QFile& file,
                                                      qint64 offset,
                                                      void* dst,
                                                      qint64 size)
{
    if (!dst || size <= 0) {
        return std::unexpected(QStringLiteral("bad read parameters"));
    }
    if (offset < 0 || !file.seek(offset)) {
        return std::unexpected(QStringLiteral("seek failed"));
    }
    if (file.read(reinterpret_cast<char*>(dst), size) != size) {
        return std::unexpected(QStringLiteral("short read"));
    }
    return {};
}

std::expected<QString, QString> MinidumpUtils::readModuleName(QFile& file, quint32 rva)
{
    quint32 lengthBytes = 0;
    auto result = readBytes(file, rva, &lengthBytes, sizeof(lengthBytes));
    if (!result || lengthBytes == 0 || lengthBytes > 1024 * 1024) {
        return std::unexpected(QStringLiteral("bad module name"));
    }

    const qint64 charCount = lengthBytes / sizeof(char16_t);
    QByteArray buffer(static_cast<qsizetype>(lengthBytes), Qt::Uninitialized);
    result = readBytes(file, rva + sizeof(quint32), buffer.data(), lengthBytes);
    if (!result) {
        return std::unexpected(result.error());
    }

    return QString::fromUtf16(
        reinterpret_cast<const char16_t*>(buffer.constData()), charCount);
}

QString MinidumpUtils::describeExceptionCode(quint32 code)
{
    switch (code) {
    case 0xC0000005L: return QStringLiteral("ACCESS_VIOLATION");
    case 0xC0000006L: return QStringLiteral("IN_PAGE_ERROR");
    case 0xC0000017L: return QStringLiteral("NOT_ENOUGH_MEMORY");
    case 0xC000001DL: return QStringLiteral("ILLEGAL_INSTRUCTION");
    case 0xC000008CL: return QStringLiteral("ARRAY_BOUNDS_EXCEEDED");
    case 0xC0000094L: return QStringLiteral("INT_DIVIDE_BY_ZERO");
    case 0xC00000FDL: return QStringLiteral("STACK_OVERFLOW");
    case 0xC0000409L: return QStringLiteral("STATUS_STACK_BUFFER_OVERRUN");
    case 0x80000003L: return QStringLiteral("BREAKPOINT");
    case 0x80000004L: return QStringLiteral("SINGLE_STEP");
    case 0xE06D7363L: return QStringLiteral("CPP_EXCEPTION");
    default:          return QString();
    }
}

QString MinidumpUtils::formatAddress(quint64 value)
{
    return QStringLiteral("0x") + QString::number(value, 16).toUpper();
}
