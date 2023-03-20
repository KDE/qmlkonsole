// SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "util.h"

#include <QQuickWindow>

#include <KWindowEffects>

Util::Util(QObject *parent) 
    : QObject{ parent }
{}

uint Util::getKeyFromString(QString key)
{
    QKeySequence seq = QKeySequence(key);
    return seq.count() > 0 ? seq[0] : 0;
}

void Util::setBlur(QQuickItem *item, bool blur)
{
    auto setWindows = [item, blur]() {
        auto reg = QRect(QPoint(0, 0), item->window()->size());
        KWindowEffects::enableBackgroundContrast(item->window(), blur, 1, 1, 1, reg);
        KWindowEffects::enableBlurBehind(item->window(), blur, reg);
    };

    disconnect(item->window(), &QQuickWindow::heightChanged, this, nullptr);
    disconnect(item->window(), &QQuickWindow::widthChanged, this, nullptr);
    connect(item->window(), &QQuickWindow::heightChanged, this, setWindows);
    connect(item->window(), &QQuickWindow::widthChanged, this, setWindows);
    setWindows();
}

void Util::setInitialWorkDir(QString &&initialWorkDir)
{
    m_initialWorkDir = std::move(initialWorkDir);
    Q_EMIT initialWorkDirChanged();
}

void Util::setInitialCommand(QString &&command)
{
    m_initialCommand = std::move(command);
    Q_EMIT initialCommandChanged();
}
