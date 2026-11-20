// handlerconfiguration.h
//
// Project-specific values forwarded by the protected application at startup.
// Empty fields mean "use the compiled-in default" from project_config.h.

#pragma once

#include <QString>

struct HandlerConfiguration final {
    QString productName;
    QString supportEmail;
    QString uploadUrl;
    QString uploadFieldProduct;
    QString uploadFieldReport;
    QString uploadFieldDump;
};
