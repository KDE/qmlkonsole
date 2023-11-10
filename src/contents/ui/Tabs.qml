// SPDX-FileCopyrightText: 2021 Felipe Kinoshita <kinofhek@gmail.com>
// SPDX-FileCopyrightText: 2022 Devin Lin <espidev@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QMLTermWidget
import org.kde.kirigami as Kirigami

import org.kde.qmlkonsole

// Taken from Angelfish's desktop ListView

ListView {
    id: root

    signal switchToTabRequested(int index)
    signal closeTabRequested(int index)
    signal addTabRequested()

    implicitHeight: Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2
    model: TerminalTabModel
    orientation: ListView.Horizontal

    Shortcut {
        sequence: "Ctrl+Shift+W"
        onActivated: {
            root.closeTabRequested(root.currentIndex);
        }
    }

    Shortcut {
        sequences: ["Ctrl+Tab", "Ctrl+PgDown"]
        onActivated: {
            if (root.currentIndex != root.count -1) {
                root.switchToTabRequested(root.currentIndex + 1);
            } else {
                root.switchToTabRequested(0);
            }
        }
    }

    Shortcut {
        sequences: ["Ctrl+Shift+Tab", "Ctrl+PgUp"]
        onActivated: {
            if (root.currentIndex === 0) {
                root.switchToTabRequested(root.count - 1);
            } else {
                root.switchToTabRequested(root.currentIndex - 1);
            }
        }
    }

    delegate: ItemDelegate {
        id: control

        highlighted: ListView.isCurrentItem

        width: Kirigami.Units.gridUnit * 15
        height: tabControl.height

        background: Rectangle {
            Kirigami.Theme.colorSet: Kirigami.Theme.Button
            Kirigami.Theme.inherit: false
            implicitHeight: Kirigami.Units.gridUnit * 3 + Kirigami.Units.smallSpacing * 2
            color: control.highlighted ? Kirigami.Theme.backgroundColor
                : (control.hovered ? Qt.darker(Kirigami.Theme.backgroundColor, 1.05)
                : Qt.darker(Kirigami.Theme.backgroundColor, 1.1))
            Behavior on color { ColorAnimation { duration: Kirigami.Units.shortDuration } }

            ToolSeparator {
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                orientation: Qt.Vertical
            }

            ToolSeparator {
                visible: index === 0
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                orientation: Qt.Vertical
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.AllButtons
                onClicked: mouse => {
                    if (mouse.button === Qt.LeftButton) {
                        root.switchToTabRequested(model.index);
                    } else if (mouse.button === Qt.MiddleButton) {
                        root.closeTabRequested(model.index);
                    }
                }
            }
        }

        contentItem: RowLayout {
            id: layout
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                id: tabIcon
                Layout.alignment: Qt.AlignLeft
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: width
                source: "tab-detach"
            }

            Label {
                id: titleLabel
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignLeft
                Layout.leftMargin: Kirigami.Units.smallSpacing
                Layout.rightMargin: Kirigami.Units.smallSpacing
                text: model.name
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignLeft
            }

            ToolButton {
                Layout.alignment: Qt.AlignRight
                Layout.preferredWidth: Kirigami.Units.iconSizes.small
                Layout.preferredHeight: width
                icon.name: "tab-close"
                onClicked: root.closeTabRequested(model.index)

                opacity: Kirigami.Settings.tabletMode ? 1 : (control.hovered ? 1 : 0);
                Behavior on opacity {
                    OpacityAnimator {
                        duration: Kirigami.Units.shortDuration
                        easing.type: Easing.InOutQuad
                    }
                }
            }
        }
    }
    footer: ToolButton {
        action: Kirigami.Action {
            icon.name: "list-add"
            onTriggered: root.addTabRequested()
        }
    }
    add: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Kirigami.Units.longDuration; easing.type: Easing.InQuad }
    }
    remove: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: Kirigami.Units.longDuration; easing.type: Easing.OutQuad }
    }
}
