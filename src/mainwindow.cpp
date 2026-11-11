// mainwindow.cpp

#include "mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "minidumpreader.h"
#include "networkmonitor.h"
#include "senders.h"
#include "stringutils.h"

MainWindow::MainWindow(const QString& appName,
                       NetworkMonitor* monitor,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_appName(appName)
    , m_monitor(monitor)
    , m_uploader(new Uploader(this))
{
    buildUi();

    if (m_monitor) {
        m_online = m_monitor->isOnline();
        connect(m_monitor, &NetworkMonitor::onlineChanged, this, [this](bool online) {
            m_online = online;
            updateActions();
            statusBar()->showMessage(online ? tr("Network available")
                                            : tr("Network unavailable"));
        });
    }

    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveSnapshot);
    connect(m_emailButton, &QPushButton::clicked, this, &MainWindow::sendEmail);
    connect(m_devButton, &QPushButton::clicked, this, &MainWindow::sendToDeveloper);
    connect(m_closeButton, &QPushButton::clicked, this, &MainWindow::close);

    connect(m_uploader, &Uploader::finished, this,
            [this](bool ok, const QString& detail) {
                m_devButton->setEnabled(true);
                if (ok) {
                    statusBar()->showMessage(tr("Snapshot sent to the developer."));
                }
                else {
                    QMessageBox::critical(this, tr("Send error"),
                                          tr("Failed to send the snapshot: %1").arg(detail));
                }
            });

    retranslateUi();
    updateActions();
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);

    m_headerLabel = new QLabel(central);
    m_headerLabel->setTextFormat(Qt::PlainText);
    mainLayout->addWidget(m_headerLabel);

    m_hintLabel = new QLabel(central);
    m_hintLabel->setTextFormat(Qt::PlainText);
    m_hintLabel->setVisible(false);
    m_hintLabel->setWordWrap(true);
    mainLayout->addWidget(m_hintLabel);

    auto* grid = new QGridLayout;

    auto addRow = [grid](int row, QLabel*& caption, QLabel*& value) {
        caption = new QLabel;
        value = new QLabel;
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        grid->addWidget(caption, row, 0);
        grid->addWidget(value, row, 1);
    };

    addRow(0, m_appCaption, m_appValue);
    addRow(1, m_pidCaption, m_pidValue);
    addRow(2, m_exceptionCaption, m_exceptionValue);
    addRow(3, m_moduleCaption, m_moduleValue);
    addRow(4, m_dumpCaption, m_dumpValue);
    addRow(5, m_sizeCaption, m_sizeValue);
    addRow(6, m_timeCaption, m_timeValue);
    addRow(7, m_exitCodeCaption, m_exitCodeValue);

    grid->setColumnStretch(1, 1);
    mainLayout->addLayout(grid);

    auto* buttonRow = new QHBoxLayout;
    m_saveButton  = new QPushButton(central);
    m_emailButton = new QPushButton(central);
    m_devButton   = new QPushButton(central);
    m_closeButton = new QPushButton(central);

    buttonRow->addWidget(m_saveButton);
    buttonRow->addWidget(m_emailButton);
    buttonRow->addWidget(m_devButton);
    buttonRow->addStretch();
    buttonRow->addWidget(m_closeButton);

    mainLayout->addLayout(buttonRow);

    setCentralWidget(central);
    statusBar();
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("Crash Handler"));

    m_appCaption->setText(tr("Application:"));
    m_pidCaption->setText(tr("Process (PID):"));
    m_exceptionCaption->setText(tr("Exception:"));
    m_moduleCaption->setText(tr("Module:"));
    m_dumpCaption->setText(tr("Dump:"));
    m_sizeCaption->setText(tr("Size:"));
    m_timeCaption->setText(tr("Time:"));
    m_exitCodeCaption->setText(tr("Exit code:"));

    m_saveButton->setText(tr("Save snapshot…"));
    m_emailButton->setText(tr("Send by email"));
    m_devButton->setText(tr("Send to developer"));
    m_closeButton->setText(tr("Close"));

    // Header reflects the current state (idle unless a crash already arrived).
    if (!m_hasCrash) {
        m_headerLabel->setText(tr("Waiting for a crash…"));
    }
}

void MainWindow::onDumpRequested(quint32 pid, const QString& dumpPath)
{
    if (m_hasCrash) {
        return;
    }

    CrashReport report;
    report.pid       = pid;
    report.appName   = m_appName;
    report.dumpPath  = dumpPath;
    report.hasDump   = true;
    report.crashTime = QDateTime::currentDateTime();

    auto parsed = MinidumpUtils::read(dumpPath);
    if (parsed) {
        parsed->pid       = pid;
        parsed->appName   = m_appName;
        parsed->crashTime = report.crashTime;
        report = std::move(*parsed);
    }

    showCrash(report);
}

void MainWindow::onClientExited(quint32 pid, int exitCode)
{
    if (m_hasCrash) {
        return;
    }

    showNoDump(pid, exitCode);
}

void MainWindow::showCrash(const CrashReport& report)
{
    m_report   = report;
    m_hasCrash = true;

    m_headerLabel->setText(tr("The application crashed"));
    m_hintLabel->setVisible(false);

    m_appValue->setText(report.appName);
    m_pidValue->setText(QString::number(report.pid));
    m_exceptionValue->setText(report.exceptionCode.isEmpty()
                                  ? tr("unknown")
                                  : QStringLiteral("%1 (%2)")
                                        .arg(report.exceptionCode, report.exceptionName));
    m_moduleValue->setText(report.moduleName.isEmpty()
                               ? tr("unknown")
                               : QStringLiteral("%1 + %2")
                                     .arg(report.moduleName, report.moduleOffset));
    m_dumpValue->setText(report.dumpPath);
    m_sizeValue->setText(formatBytes(report.dumpSize));
    m_timeValue->setText(report.crashTime.toString(QStringLiteral("dd.MM.yyyy hh:mm:ss")));
    m_exitCodeValue->setText(QStringLiteral("—"));

    updateActions();
    show();
    raise();
    activateWindow();
}

void MainWindow::showNoDump(quint32 pid, int exitCode)
{
    m_report.pid      = pid;
    m_report.appName  = m_appName;
    m_report.hasDump  = false;
    m_report.exitCode = exitCode;
    m_report.crashTime = QDateTime::currentDateTime();
    m_hasCrash = true;

    m_headerLabel->setText(tr("The application terminated without a dump"));
    m_hintLabel->setText(tr("The application may have been terminated by the protection or externally. No dump was created."));
    m_hintLabel->setVisible(true);

    m_appValue->setText(m_appName);
    m_pidValue->setText(QString::number(pid));
    m_exceptionValue->setText(QStringLiteral("—"));
    m_moduleValue->setText(QStringLiteral("—"));
    m_dumpValue->setText(QStringLiteral("—"));
    m_sizeValue->setText(QStringLiteral("—"));
    m_timeValue->setText(m_report.crashTime.toString(QStringLiteral("dd.MM.yyyy hh:mm:ss")));
    m_exitCodeValue->setText(formatExitCode(exitCode));

    updateActions();
    show();
    raise();
    activateWindow();
}

void MainWindow::updateActions()
{
    const bool hasDump = m_hasCrash && m_report.hasDump;

    m_saveButton->setEnabled(hasDump);
    m_emailButton->setEnabled(m_hasCrash);
    m_devButton->setEnabled(m_hasCrash && m_online);
}

void MainWindow::saveSnapshot()
{
    if (!m_report.hasDump) {
        return;
    }

    const QString target = QFileDialog::getSaveFileName(
        this,
        tr("Save snapshot"),
        QDir::home().filePath(QStringLiteral("crash.dmp")),
        tr("Dump file (*.dmp);;All files (*)"));

    if (target.isEmpty()) {
        return;
    }

    if (QFile::exists(target)) {
        QFile::remove(target);
    }

    QFile source(m_report.dumpPath);
    if (!source.copy(target)) {
        QMessageBox::critical(this, tr("Save error"),
                              tr("Failed to save the snapshot to \"%1\".\n%2")
                                  .arg(target, source.errorString()));
        return; // keep the main window open so the user can retry
    }

    statusBar()->showMessage(tr("Snapshot saved: %1").arg(target));
}

void MainWindow::sendEmail()
{
    QString body = tr("Crash snapshot of application %1.\n\n").arg(m_appName);
    body += QStringLiteral("pid: %1\n").arg(m_report.pid);

    if (m_report.hasDump) {
        body += QStringLiteral("exception: %1 (%2)\n")
                    .arg(m_report.exceptionCode, m_report.exceptionName);
        body += QStringLiteral("module: %1 + %2\n")
                    .arg(m_report.moduleName, m_report.moduleOffset);
        body += QStringLiteral("dump: %1\n").arg(m_report.dumpPath);
    }
    else {
        body += QStringLiteral("exit code: %1\n").arg(m_report.exitCode);
    }

    const auto result = EmailUtils::openInMailClient(
        StringUtils::fromUtf8(constants::supportEmail),
        tr("Crash snapshot (%1)").arg(m_appName),
        body);

    if (!result) {
        QMessageBox::warning(this, tr("Send error"),
                             tr("Failed to open the mail client."));
    }
}

void MainWindow::sendToDeveloper()
{
    if (!m_hasCrash || !m_online) {
        return;
    }

    m_devButton->setEnabled(false);
    statusBar()->showMessage(tr("Sending the snapshot to the developer…"));
    m_uploader->send(m_report);
}

QString MainWindow::formatBytes(qint64 bytes)
{
    if (bytes < 0) {
        return QStringLiteral("—");
    }

    constexpr double kb = 1024.0;
    constexpr double mb = kb * 1024.0;
    constexpr double gb = mb * 1024.0;

    const double value = static_cast<double>(bytes);
    if (value >= gb) {
        return QStringLiteral("%1 GB").arg(value / gb, 0, 'f', 2);
    }
    if (value >= mb) {
        return QStringLiteral("%1 MB").arg(value / mb, 0, 'f', 2);
    }
    if (value >= kb) {
        return QStringLiteral("%1 KB").arg(value / kb, 0, 'f', 1);
    }
    return QStringLiteral("%1 B").arg(bytes);
}

QString MainWindow::formatExitCode(int code)
{
    if (code < 0) {
        // NTSTATUS-style code (e.g. 0xC0000005) that did not fit an int.
        return QStringLiteral("0x%1")
            .arg(static_cast<quint32>(code), 8, 16, QLatin1Char('0'))
            .toUpper();
    }
    return QString::number(code);
}
