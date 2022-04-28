// SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.0
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.2

import org.kde.kirigami 2.19 as Kirigami

import QMLTermWidget 1.0

import org.kde.qmlkonsole 1.0

Kirigami.ScrollablePage {
    title: i18n("Saved Commands")
    property QMLTermWidget terminal

    actions {
        main: Kirigami.Action {
            text: i18n("Add Command")
            icon.name: "contact-new"
            onTriggered: actionDialog.open()
        }
    }

    ListView {
        id: listView
        model: SavedCommandsModel
        width: parent.width
        height: contentHeight
        
        Kirigami.PlaceholderMessage {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Kirigami.Units.largeSpacing
            anchors.rightMargin: Kirigami.Units.largeSpacing
            
            visible: listView.count === 0
            icon.name: "dialog-scripts"
            text: i18n("No saved commands")
            explanation: i18n("Save commands to quickly run them without typing them out.")
            
            helpfulAction: Kirigami.Action {
                icon.name: "list-add"
                text: i18n("Add command")
                onTriggered: actionDialog.open()
            }
        }
        
        delegate: Kirigami.SwipeListItem {
            RowLayout {
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

            actions: [
                Kirigami.Action {
                    icon.name: "delete"
                    onTriggered: {
                        SavedCommandsModel.removeRows(index, 1);
                        showPassiveNotification(i18n("Action %1 removed", label.text));
                    }
                }
            ]
        }
    }

    Kirigami.PromptDialog {
        id: actionDialog
        title: i18n("Add Command")
        preferredWidth: Kirigami.Units.gridUnit * 30
        padding: Kirigami.Units.largeSpacing
        standardButtons: Dialog.Save | Dialog.Cancel
        
        onAccepted: {
            if (textField.text != "") {
                SavedCommandsModel.addAction(textField.text);
                textField.text = ""
            }

            actionDialog.close();
        }

        contentItem: RowLayout {
            TextField {
                id: textField
                font.family: "Monospace"
                Layout.fillWidth: true
            }
        }
    }
}
