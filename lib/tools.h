// SPDX-FileCopyrightText: Jonah Brüchert <jbb@kaidan.im>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QStringList>
#include <QLoggingCategory>

QString kbLayoutDir();
void add_custom_color_scheme_dir(const QString& custom_dir);
const QStringList colorSchemesDirs();

Q_DECLARE_LOGGING_CATEGORY(qtermwidgetLogger)

#endif
