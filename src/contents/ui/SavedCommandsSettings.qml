// SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.konsoleqml

import org.kde.qmlkonsole

Kirigami.ScrollablePage {
    id: root
    title: i18n("Saved Commands")

    property TerminalEmulator terminal
    property bool editMode: false

    actions: [
        Kirigami.Action {
            text: i18n("Add Command")
            icon.name: "contact-new"
            onTriggered: actionDialog.open()
        },
        Kirigami.Action {
            text: i18n("Edit")
            icon.name: 'entry-edit'
            checkable: true
            onCheckedChanged: root.editMode = checked
            visible: listView.count > 0
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

            helpfulAction: Kirigami.Action {
                icon.name: "list-add"
                text: i18n("Add command")
                onTriggered: actionDialog.open()
            }
        }

        delegate: Control {
            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.largeSpacing
            topPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.largeSpacing
            width: ListView.view.width

            contentItem: RowLayout {
                id: row

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
                ToolButton {
                    visible: root.editMode
                    icon.name: 'delete'
                    onClicked: {
                        SavedCommandsModel.removeRow(index);
                        showPassiveNotification(i18n("Command %1 removed", label.text));
                    }
                }
            }
        }
    }

    Kirigami.Dialog {
        id: actionDialog
        title: i18n("Add Command")
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
                Layout.preferredWidth: Kirigami.Units.gridUnit * 30
            }
        }
    }
}
