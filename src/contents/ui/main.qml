import QtQuick 2.10
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import QMLTermWidget 1.0
import org.kde.kirigami 2.7 as Kirigami

import org.kde.qmlkonsole 1.0

Kirigami.ApplicationWindow {
    contextDrawer: Kirigami.ContextDrawer {}

    globalDrawer: Kirigami.GlobalDrawer {
        enabled: pageStack.layers.depth === 1

        actions: [
            Kirigami.Action {
                text: i18n("Settings")
                icon.name: "settings-configure"
                onTriggered: pageStack.layers.push("qrc:/SettingsPage.qml",
                    {
                        "terminal": pageStack.items[0].terminal
                    }
                )
            }
        ]
    }

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None
    pageStack.initialPage: "qrc:/TerminalPage.qml"
}
