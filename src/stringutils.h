// stringutils.h
//
// Small string helpers shared across the project (project codestyle keeps
// common free functions in *Utils classes rather than anonymous namespaces).

#pragma once

#include <QString>
#include <string_view>

class StringUtils {
public:
    static QString fromUtf8(std::string_view text)
    {
        return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
    }
};
