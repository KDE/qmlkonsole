// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami 2.19 as Kirigami
import QMLTermWidget

import org.kde.qmlkonsole

Kirigami.ScrollablePage {
    id: root
    title: i18n("Settings")
    property QMLTermWidget terminal
    
    topPadding: 0
    bottomPadding: Kirigami.Units.gridUnit
    leftPadding: 0
    rightPadding: 0
    
    SettingsComponent {
        terminal: root.terminal
        width: root.width
    }
}
