// SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "util.h"

Util::Util(QObject *parent) 
    : QObject{ parent }
{}

uint Util::getKeyFromString(QString key)
{
    QKeySequence seq = QKeySequence(key);
    return seq.count() > 0 ? seq[0] : 0;
}
