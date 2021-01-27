// SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.6
import QtQuick.Controls 2.0

import org.kde.kirigami 2.5 as Kirigami
Button {
    property int modifier: 0
    id: button
    implicitHeight: Kirigami.Units.gridUnit * 2.5
    activeFocusOnTab: false
    flat: true
    //FIXME: Qt needs more sophisticated input method protocol, this mousearea is to not give the button the focus on click (closing the keyboard)
    MouseArea {
        anchors.fill: parent
        onClicked: button.clicked()
    }
}
