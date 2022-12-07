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
    pageStack.popHiddenPages: true
    
    color: "transparent"
    
    Component.onCompleted: {
        if (TerminalSettings.blurWindow) {
            Util.setBlur(pageStack, true);
        }
    }
}
