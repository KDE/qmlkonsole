import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import QMLTermWidget 1.0
import org.kde.kirigami 2.3 as Kirigami

Kirigami.ApplicationWindow {
    Kirigami.Action {
        onTriggered: terminal.copyClipboard();
        shortcut: "Ctrl+Shift+C"
    }

    Kirigami.Action {
        onTriggered: terminal.pasteClipboard();
        shortcut: "Ctrl+Shift+V"
    }

    ColumnLayout {
        anchors.fill: parent
        QMLTermWidget {
            id: terminal
            Layout.fillWidth: true
            Layout.fillHeight: true
            font.family: "Monospace"
            font.pointSize: 7
            colorScheme: "cool-retro-term"

            function pressKey(key, modifiers, pressed, nativeScanCode, text) {
                terminal.simulateKeyPress(key, modifiers, pressed, nativeScanCode, text)
                terminal.forceActiveFocus()
            }

            session: QMLTermSession{
                id: mainsession
                initialWorkingDirectory: "$HOME"
                onFinished: Qt.quit()
                onMatchFound: {
                    console.log("found at: %1 %2 %3 %4".arg(startColumn).arg(startLine).arg(endColumn).arg(endLine));
                }
                onNoMatchFound: {
                    console.log("not found");
                }
            }

            onTerminalUsesMouseChanged: console.log(terminalUsesMouse);
            onTerminalSizeChanged: console.log(terminalSize);
            Component.onCompleted: mainsession.startShellProgram();

            QMLTermScrollbar {
                terminal: terminal
                width: 20
                Rectangle {
                    opacity: 0.4
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
                    terminal.simulateWheel(0, 0, 0, 0, Qt.point(0, (mouse.y - oldY)*2))
                    oldY = mouse.y
                }
                onClicked: {
                    Qt.inputMethod.show();
                }
            }
        }
        RowLayout {
            ToolButton {
                text: "Cancel"
                onClicked: {
                    terminal.pressKey(Qt.Key_C, Qt.ControlModifier, true)
                }
            }
            ToolButton {
                Layout.maximumWidth: height
                text: "Tab"
                onClicked: terminal.pressKey(Qt.Key_Tab, 0, true, 0, "")
            }
            ToolButton {
                Layout.maximumWidth: height
                text: "←"
                onClicked: terminal.pressKey(Qt.Key_Left, 0, true, 0, "")
            }
            ToolButton {
                Layout.maximumWidth: height
                text: "↑"
                onClicked: terminal.pressKey(Qt.Key_Up, 0, true, 0, "")
            }
            ToolButton {
                Layout.maximumWidth: height
                text: "→"
                onClicked: terminal.pressKey(Qt.Key_Right, 0, true, 0, "")
            }
            ToolButton {
                Layout.maximumWidth: height
                text: "↓"
                onClicked: terminal.pressKey(Qt.Key_Down, 0, true,0, "")
            }
            ToolButton {
                Layout.maximumWidth: height
                text: "|"
                onClicked: terminal.pressKey(Qt.Key_Bar, 0, true, 0, "|")
            }
            ToolButton {
                Layout.maximumWidth: height
                text: "~"
                onClicked: terminal.pressKey(Qt.Key_AsciiTilde, 0, true, 0, "~")
            }
        }
        Item {
            Layout.minimumHeight: Qt.inputMethod.keyboardRectangle.height
            Layout.fillWidth: true
        }
    }


    Component.onCompleted: terminal.forceActiveFocus();
}
