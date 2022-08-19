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

    contextDrawer: Kirigami.ContextDrawer {}

    pageStack.initialPage: "qrc:/TerminalPage.qml"
    
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton;
    
    pageStack.columnView.columnResizeMode: Kirigami.ColumnView.SingleColumn
    
    color: "transparent"
    
    // pop pages when not in use
    Connections {
        target: applicationWindow().pageStack
        function onCurrentIndexChanged() {
            // wait for animation to finish before popping pages
            timer.restart();
        }
    }
    
    Timer {
        id: timer
        interval: 300
        onTriggered: {
            let currentIndex = applicationWindow().pageStack.currentIndex;
            while (applicationWindow().pageStack.depth > (currentIndex + 1) && currentIndex >= 0) {
                applicationWindow().pageStack.pop();
            }
        }
    }
}
