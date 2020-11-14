// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.0
import QtQuick.Controls 2.14

import org.kde.kirigami 2.7 as Kirigami
import QMLTermWidget 1.0

import org.kde.qmlkonsole 1.0

Kirigami.Page {
    title: i18n("Settings")
    property QMLTermWidget terminal

    Kirigami.FormLayout {
        anchors.fill: parent

        ComboBox {
            model: terminal.availableColorSchemes
            currentIndex: terminal.availableColorSchemes.indexOf(TerminalSettings.colorScheme)
            onCurrentValueChanged: TerminalSettings.colorScheme = currentValue
            Kirigami.FormData.label: i18n("Color Scheme")
        }
    }
}
