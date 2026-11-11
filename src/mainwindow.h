// mainwindow.h
//
// Main crash-handler window: shows a captured snapshot and offers three
// actions — save locally, send by email, send to the developer. The window is
// created lazily (see main.cpp) and only appears when the client crashes.

#pragma once

#include <QMainWindow>

#include "crashreport.h"

class NetworkMonitor;
class Uploader;
class QLabel;
class QPushButton;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& appName,
                        NetworkMonitor* monitor,
                        QWidget* parent = nullptr);

    // A client requested a dump (Breakpad wrote it to |dumpPath|).
    void onDumpRequested(quint32 pid, const QString& dumpPath);
    // A client process exited abnormally (a clean exit is handled upstream).
    void onClientExited(quint32 pid, int exitCode);

private:
    void buildUi();
    void retranslateUi();

    void showCrash(const CrashReport& report);
    void showNoDump(quint32 pid, int exitCode);
    void updateActions();

    void saveSnapshot();
    void sendEmail();
    void sendToDeveloper();

    static QString formatBytes(qint64 bytes);
    static QString formatExitCode(int code);

    // Widgets.
    QLabel*      m_headerLabel = nullptr;
    QLabel*      m_hintLabel   = nullptr;

    QLabel*      m_appCaption      = nullptr;
    QLabel*      m_appValue        = nullptr;
    QLabel*      m_pidCaption      = nullptr;
    QLabel*      m_pidValue        = nullptr;
    QLabel*      m_exceptionCaption = nullptr;
    QLabel*      m_exceptionValue  = nullptr;
    QLabel*      m_moduleCaption   = nullptr;
    QLabel*      m_moduleValue     = nullptr;
    QLabel*      m_dumpCaption     = nullptr;
    QLabel*      m_dumpValue       = nullptr;
    QLabel*      m_sizeCaption     = nullptr;
    QLabel*      m_sizeValue       = nullptr;
    QLabel*      m_timeCaption     = nullptr;
    QLabel*      m_timeValue       = nullptr;
    QLabel*      m_exitCodeCaption = nullptr;
    QLabel*      m_exitCodeValue   = nullptr;

    QPushButton* m_saveButton  = nullptr;
    QPushButton* m_emailButton = nullptr;
    QPushButton* m_devButton   = nullptr;
    QPushButton* m_closeButton = nullptr;

    // State.
    QString     m_appName;
    CrashReport m_report;
    bool        m_hasCrash = false;
    bool        m_online   = true;

    // Dependencies.
    NetworkMonitor* m_monitor  = nullptr;
    Uploader*       m_uploader = nullptr;
};
