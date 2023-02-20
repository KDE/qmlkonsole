// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shellcommand.h"

#include <KSandbox>
#include <KShell>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

ShellCommand::ShellCommand(QObject *parent)
    : QObject(parent)
    , m_cmd(KShell::splitArgs(command()))
{
}

QString ShellCommand::executable() const
{
    return m_cmd.front();
}

QStringList ShellCommand::args() const
{
    return m_cmd.mid(1);
}

QString ShellCommand::command() const
{
    // Signal to running processes that we can handle colors
    qputenv("TERM", "xterm-256color");

    if (KSandbox::isFlatpak()) {
        auto shell = []() {
            // From Konsole: https://github.com/KDE/konsole/blob/c450e754c789915b6dd27514a18f311ba9249981/src/profile/Profile.cpp#L177
            auto pw = getpwuid(getuid());
            // pw: Do not pass the returned pointer to free.
            if (pw != nullptr) {
                QProcess proc;
                proc.setProgram(QStringLiteral("getent"));
                proc.setArguments({QStringLiteral("passwd"), QString::number(pw->pw_uid)});
                KSandbox::startHostProcess(proc);
                proc.waitForFinished();
                return QString::fromUtf8(proc.readAllStandardOutput().simplified().split(':').at(6));
            } else {
                return QStringLiteral("bash");
            }
        }();

        return QStringLiteral("host-spawn %1").arg(shell);
    } else {
        return qEnvironmentVariable("SHELL");
    }
}
