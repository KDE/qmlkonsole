// SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QKeySequence>

class Util : public QObject
{
    Q_OBJECT
    
public:
    Util(QObject *parent = nullptr);
    
    static Util *self()
    {
        static Util *singleton = new Util();
        return singleton;
    }
    
    Q_INVOKABLE uint getKeyFromString(QString key);
};

