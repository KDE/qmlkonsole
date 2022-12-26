// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shellcommand.h"

#include <KSandbox>
#include <KShell>

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
    if (KSandbox::isFlatpak()) {
        return QStringLiteral(R"(bash -c "flatpak-spawn --env=TERM=${TERM} --host sh -c 'exec $SHELL'")");
    } else {
        return qEnvironmentVariable("SHELL");
    }
}
