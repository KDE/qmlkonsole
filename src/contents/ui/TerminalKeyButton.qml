// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.6
import QtQuick.Controls 2.0 as Controls

import org.kde.kirigami 2.5 as Kirigami

Controls.ToolButton {
    id: button
    implicitHeight: Kirigami.Units.gridUnit * 2
    implicitWidth: Math.round(Kirigami.Units.gridUnit * 1.5)
    activeFocusOnTab: false
    focusPolicy: Qt.NoFocus
}
