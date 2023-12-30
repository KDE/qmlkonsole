// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "KonsoleQML.h"

#include "TerminalDisplay.h"
#include "TerminalSession.h"

#include <QQmlEngine>

using namespace Konsole;

void KonsoleQML::registerTypes(const char *uri)
{
    // @uri org.kde.konsoleqml
    qmlRegisterType<TerminalDisplay>(uri, 1, 0, "TerminalEmulator");
    qmlRegisterType<TerminalSession>(uri, 1, 0, "TerminalSession");
}
