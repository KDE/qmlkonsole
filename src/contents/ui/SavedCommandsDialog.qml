// SPDX-FileCopyrightText: 2021-2022 Devin Lin <espidev@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import org.kde.konsoleqml
import org.kde.kirigami as Kirigami

import org.kde.qmlkonsole

Kirigami.Dialog {
    id: savedCommandsDialog
    
    property var terminal
    
    title: i18nc("@title:window", "Saved Commands")
    preferredHeight: Kirigami.Units.gridUnit * 25
    preferredWidth: Kirigami.Units.gridUnit * 16
    standardButtons: Kirigami.Dialog.NoButton
    
    customFooterActions: [
        Kirigami.Action {
            text: i18n("Configure")
            icon.name: "settings-configure"
            onTriggered: {
                pageStack.push("qrc:/SavedCommandsSettings.qml");
                savedCommandsDialog.close();
            }
        }
    ]
    
    ListView {
        id: listView
        model: SavedCommandsModel
        
        Kirigami.PlaceholderMessage {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Kirigami.Units.largeSpacing
            anchors.rightMargin: Kirigami.Units.largeSpacing
            
            visible: listView.count === 0
            icon.name: "dialog-scripts"
            text: i18n("No saved commands")
            explanation: i18n("Save commands to quickly run them without typing them out.")
        }
        
        delegate: ItemDelegate {
            width: tabListView.width
            
            onClicked: {
                terminal.simulateKeyPress(0, 0, 0, 0, model.display);
                savedCommandsDialog.close();
                terminal.forceActiveFocus()
            }
        
            contentItem: RowLayout {
                Kirigami.Icon {
                    source: "dialog-scripts"
                }
                Label {
                    id: label
                    text: model.display
                    font.family: "Monospace"
                }
                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
