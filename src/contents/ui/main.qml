import QtQuick 2.10
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import QMLTermWidget 1.0
import org.kde.qmlkonsole 0.1
import org.kde.kirigami 2.7 as Kirigami

Kirigami.ApplicationWindow {
    contextDrawer: Kirigami.ContextDrawer {}

    Settings {
        id: settings
    }

    globalDrawer: Kirigami.GlobalDrawer {
        enabled: pageStack.layers.depth === 1

        actions: [
            Kirigami.Action {
                text: i18n("Settings")
                icon.name: "settings-configure"
                onTriggered: pageStack.layers.push("qrc:/SettingsPage.qml",
                    {
                        "terminal": pageStack.items[0].terminal,
                        "settings": settings
                    }
                )
            }
        ]
    }

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None
    Component.onCompleted: pageStack.push("qrc:/TerminalPage.qml",
        { "settings": settings }
    )
}
