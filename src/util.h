// SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QKeySequence>
#include <QQuickItem>

class Util : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString initialCommand MEMBER m_initialCommand NOTIFY initialCommandChanged)
    Q_PROPERTY(QString initialWorkDir MEMBER m_initialWorkDir NOTIFY initialWorkDirChanged)

public:
    Util(QObject *parent = nullptr);
    
    static Util *self()
    {
        static Util *singleton = new Util();
        return singleton;
    }

    Q_INVOKABLE uint getKeyFromString(QString key);
    Q_INVOKABLE void setBlur(QQuickItem *item, bool blur);

    void setInitialCommand(QString &&command);
    Q_SIGNAL void initialCommandChanged();

    void setInitialWorkDir(QString &&initialWorkDir);
    Q_SIGNAL void initialWorkDirChanged();

private:
    QString m_initialCommand;
    QString m_initialWorkDir;
};

