// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>

class ShellCommand : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString executable READ executable CONSTANT)
    Q_PROPERTY(QStringList args READ args CONSTANT)

public:
    ShellCommand(QObject *parent = nullptr);

    QString executable() const;
    QStringList args() const;

private:
    QString command() const;

    QStringList m_cmd;
};

