// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import org.kde.kirigami 2.19 as Kirigami
import QMLTermWidge

import org.kde.qmlkonsole
import org.kde.kirigamiaddons.labs.mobileform as MobileForm

Kirigami.ScrollablePage {
    id: root
    title: i18n("Settings")
    property QMLTermWidget terminal
    
    topPadding: Kirigami.Units.gridUnit
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    
    SettingsComponent {
        terminal: root.terminal
        width: root.width
    }
}
