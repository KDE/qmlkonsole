// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Devin Lin <devin@kde.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import org.kde.kirigami as Kirigami

import org.kde.qmlkonsole

Kirigami.ApplicationWindow {
    title: i18n("Terminal")

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

    onClosing: {
        close.accepted = false;
        pageStack.currentItem.closeWindow();
    }
}
