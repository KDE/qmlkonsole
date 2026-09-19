// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QCommandLineParser>

void configureCommandLineParser(QCommandLineParser &parser);
QString initialCommand(const QCommandLineParser &parser);
