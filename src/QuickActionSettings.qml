// SPDX-FileCopyrightText: 2020 Han Young <hanyoung@protonmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.0
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.2
import org.kde.kirigami 2.7 as Kirigami
import QMLTermWidget 1.0

import org.kde.qmlkonsole 1.0

Kirigami.ScrollablePage {
    title: i18n("Quick Action Settings")
    property QMLTermWidget terminal

    actions {
        main: Kirigami.Action {
            text: i18n("Add Actions")
            icon.name: "contact-new"
            onTriggered: actionDialog.open()
        }
    }

    ListView {
        id: listView
        model: quickActionModel
        width: parent.width
        height: contentHeight
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
                        quickActionModel.removeRows(index, 1);
                        showPassiveNotification(i18n("Action %1 removed", label.text));
                    }
                }
            ]
        }
    }

    Kirigami.OverlaySheet {
        id: actionDialog
        header: Text {
            text: i18n("Add action")
        }
        showCloseButton: true
        contentItem: RowLayout{
            TextField {
                id: textField
                Layout.fillWidth: true
            }
            Button {
                Layout.alignment: Qt.AlignRight
                text: i18n("Save")
                icon.name: "dialog-ok"
                onClicked: {
                    if(textField.text != "") {
                        quickActionModel.addAction(textField.text);
                        textField.text = ""
                    }

                    actionDialog.close();
                }
            }
        }
    }
}
