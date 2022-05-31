// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.6
import QtQuick.Controls 2.0 as Controls

import org.kde.kirigami 2.5 as Kirigami

Controls.Button {
    id: button
    implicitHeight: Kirigami.Units.gridUnit * 2
    implicitWidth: Math.round(Kirigami.Units.gridUnit * 1.5)
    activeFocusOnTab: false
    focusPolicy: Qt.NoFocus
    
    contentItem: Controls.Label {
        text: button.text
        font: Kirigami.Theme.smallFont
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    
    background: Rectangle {
        Kirigami.Theme.inherit: false
        Kirigami.Theme.colorSet: Kirigami.Theme.View
        
        border.color: Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g, Kirigami.Theme.textColor.b, 0.3)
        border.width: 1
        color: Qt.darker(Kirigami.Theme.alternateBackgroundColor, button.down ? 1.5 : 1)
        radius: Kirigami.Units.smallSpacing
    }
}
