import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as Controls

import org.kde.kirigami 2.3 as Kirigami

Kirigami.ApplicationWindow {
    id: root

    Component {id: terminalPage; TerminalPage {}}

    contextDrawer: Kirigami.ContextDrawer {}
    wideScreen: true

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.TabBar
    pageStack.initialPage: terminalPage

    // Display only one page at the time
    pageStack.defaultColumnWidth: Kirigami.Units.gridUnit * 100
}
