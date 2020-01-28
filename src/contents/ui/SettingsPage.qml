import QtQuick 2.0
import QtQuick.Controls 2.0

import org.kde.kirigami 2.7 as Kirigami
import org.kde.qmlkonsole 0.1
import QMLTermWidget 1.0

Kirigami.Page {
    title: i18n("Settings")
    property QMLTermWidget terminal
    property Settings settings

    onIsCurrentPageChanged: {
        if (isCurrentPage)
            applicationWindow().pageStack.globalToolBar.style = Kirigami.ApplicationHeaderStyle.ToolBar
        else
            applicationWindow().pageStack.globalToolBar.style = Kirigami.ApplicationHeaderStyle.None
    }


    Kirigami.FormLayout {
        anchors.fill: parent

        ComboBox {
            model: terminal.availableColorSchemes
            onCurrentValueChanged: {
                settings.setValue("colorScheme", currentValue)
                terminal.colorScheme = currentValue
            }

            Kirigami.FormData.label: i18n("Color Scheme")
        }
    }
}
