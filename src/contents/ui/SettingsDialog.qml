// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.10
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.15

import QMLTermWidget 1.0
import org.kde.kirigami 2.19 as Kirigami

import org.kde.qmlkonsole 1.0

Kirigami.Dialog {
    id: root
    title: i18n("Settings")
    standardButtons: Kirigami.Dialog.NoButton
    
    property QMLTermWidget terminal
    preferredWidth: Kirigami.Units.gridUnit * 35
    
    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    
    Control {
        id: control
        
        leftPadding: 0
        rightPadding: 0
        topPadding: Kirigami.Units.gridUnit
        bottomPadding: Kirigami.Units.gridUnit
        
        background: Rectangle {
            Kirigami.Theme.inherit: false
            Kirigami.Theme.colorSet: Kirigami.Theme.Window
            color: Kirigami.Theme.backgroundColor
        }
        
        contentItem: SettingsComponent {
            dialog: root
            width: control.width
            terminal: root.terminal
        }
    }
}

