// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.10
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import QMLTermWidget 1.0
import org.kde.kirigami 2.7 as Kirigami

import org.kde.qmlkonsole 1.0

Kirigami.ApplicationWindow {
    title: i18n("Terminal")
    
    property var terminal: pageStack.items[0].currentTerminal
    
    // fix layers not showing actions
    pageStack.globalToolBar.style: pageStack.layers.depth > 1 ? Kirigami.ApplicationHeaderStyle.Auto : Kirigami.ApplicationHeaderStyle.ToolBar

    contextDrawer: Kirigami.ContextDrawer {}
    globalDrawer: Kirigami.GlobalDrawer {
        id: globalDrawer
        enabled: pageStack.layers.depth === 1

        actions: [
            Kirigami.Action {
                text: i18n("Quick Actions")
                icon.name: "new-command-alarm"
                onTriggered: pageStack.layers.push("qrc:/QuickActionSettings.qml")
            }
        ]

        ListView {
            model: quickActionModel
            delegate: Kirigami.AbstractListItem {
                Text {
                    text: model.display
                    wrapMode: Text.Wrap
                }
                onClicked: {
                    terminal.simulateKeyPress(0,0,0,0,model.display);
                    globalDrawer.close();
                }
            }
            width: parent.width
            height: contentHeight
        }
    }

    pageStack.initialPage: "qrc:/TerminalPage.qml"
}
