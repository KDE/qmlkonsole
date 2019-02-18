import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as Controls

import org.kde.kirigami 2.3 as Kirigami

Kirigami.ApplicationWindow {
    Component {id: terminalPage; TerminalPage {}}

    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None
    pageStack.initialPage: terminalPage
}
