import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.4 as Controls

import QMLTermWidget 1.0
import org.kde.kirigami 2.3 as Kirigami

Kirigami.Page {
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0
    topPadding: 0

    title: mainsession.title

    Kirigami.Theme.colorSet: Kirigami.Theme.Complementary

    background: Rectangle {
        color: Kirigami.Theme.backgroundColor
    }

    actions {
        main: Kirigami.Action {
            text: "New session"
            icon.name: "list-add-symbolic"
            shortcut: "Ctrl+Shift+T"
            onTriggered: pageStack.push(terminalPage)
        }
        contextualActions: [
            Kirigami.Action {
                icon.name: "edit-copy-symbolic"
                text: "Copy"
                onTriggered: terminal.copyClipboard()
                shortcut: "Ctrl+Shift+C"
            },
            Kirigami.Action {
                icon.name: "edit-paste-symbolic"
                text: "Paste"
                onTriggered: terminal.pasteClipboard()
                shortcut: "Ctrl+Shift+V"
            },
            Kirigami.Action {
                icon.name: "tab-close"
                text: "Close Tab"
                onTriggered: closeTab()
                shortcut: "Ctrl+Shift+W"
            }
        ]
    }

    ColumnLayout {
        anchors.fill: parent
        QMLTermWidget {
            id: terminal

            Layout.fillWidth: true
            Layout.fillHeight: true

            font.family: "Monospace"
            colorScheme: "DarkPastels"

            focus: true
            smooth: true

            session: QMLTermSession {
                id: mainsession
                initialWorkingDirectory: "$HOME"
                onFinished: closeTab()
                onMatchFound: {
                    console.log("found at: %1 %2 %3 %4".arg(startColumn).arg(
                                    startLine).arg(endColumn).arg(endLine))
                }
                onNoMatchFound: {
                    console.log("not found")
                }
            }

            onTerminalUsesMouseChanged: console.log(terminalUsesMouse)
            onTerminalSizeChanged: {
                console.log(terminalSize)
                terminal.forceActiveFocus()
            }

            QMLTermScrollbar {
                terminal: terminal
                width: Kirigami.Units.gridUnit
                Rectangle {
                    opacity: 0.2
                    anchors.margins: 5
                    radius: width * 0.5
                    anchors.fill: parent
                }
            }

            MouseArea {
                anchors.fill: parent
                property real oldY
                onPressed: oldY = mouse.y
                onPositionChanged: {
                    terminal.simulateWheel(0, 0, 0, 0, Qt.point(0, (mouse.y - oldY) * 2))
                    oldY = mouse.y
                }
                onClicked: {
                    terminal.forceActiveFocus()
                    Qt.inputMethod.show()
                }
            }
        }
    }

    Item {
        Layout.minimumHeight: Qt.inputMethod.keyboardRectangle.height
        Layout.fillWidth: true
    }

    footer: RowLayout {
        focus: false
        Controls.ToolButton {
            Kirigami.Theme.inherit: true
            Layout.maximumWidth: height
            text: "Tab"
            onClicked: simulateKeyPress(Qt.Key_Tab, 0, true, 0, "")
        }
        Controls.ToolButton {
            Kirigami.Theme.inherit: true
            Layout.maximumWidth: height
            text: "←"
            onClicked: simulateKeyPress(Qt.Key_Left, 0, true, 0, "")
        }
        Controls.ToolButton {
            Kirigami.Theme.inherit: true
            Layout.maximumWidth: height
            text: "↑"
            onClicked: simulateKeyPress(Qt.Key_Up, 0, true, 0, "")
        }
        Controls.ToolButton {
            Kirigami.Theme.inherit: true
            Layout.maximumWidth: height
            text: "→"
            onClicked: simulateKeyPress(Qt.Key_Right, 0, true, 0, "")
        }
        Controls.ToolButton {
            Kirigami.Theme.inherit: true
            Layout.maximumWidth: height
            text: "↓"
            onClicked: simulateKeyPress(Qt.Key_Down, 0, true, 0, "")
        }
        Controls.ToolButton {
            Kirigami.Theme.inherit: true
            Layout.maximumWidth: height
            text: "|"
            onClicked: simulateKeyPress(Qt.Key_Bar, 0, true, 0, "|")
        }
        Controls.ToolButton {
            Kirigami.Theme.inherit: true
            Layout.maximumWidth: height
            text: "~"
            onClicked: simulateKeyPress(Qt.Key_AsciiTilde, 0, true, 0, "~")
        }
    }

    // Wrapper function that allows us to force active focus to recieve
    // non-simulated key events
    function simulateKeyPress(key, modifiers, pressed, nativeScanCode, text) {
        terminal.simulateKeyPress(key, modifiers, pressed, nativeScanCode, text)
        terminal.forceActiveFocus()
    }

    function closeTab() {
        pageStack.pop()

        if (pageStack.depth < 1)
            pageStack.push(terminalPage)
    }

    Component.onCompleted: {
        mainsession.startShellProgram()
        terminal.forceActiveFocus()
    }
}
