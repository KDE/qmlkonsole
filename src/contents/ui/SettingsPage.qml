// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.19 as Kirigami
import QMLTermWidget 1.0

import org.kde.qmlkonsole 1.0
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

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
