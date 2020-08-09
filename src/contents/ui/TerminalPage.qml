import QtQuick 2.10
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2

import QMLTermWidget 1.0
import org.kde.kirigami 2.7 as Kirigami

import org.kde.qmlkonsole 1.0

Kirigami.Page {
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    property alias terminal: terminal

    globalToolBarStyle: Kirigami.Settings.isMobile ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.ToolBar

    contextualActions: [
        Kirigami.Action {
            icon.name: "edit-copy"
            text: i18n("Copy")
            onTriggered: {
                terminal.copyClipboard();
                onTriggered: scrollMouseArea.enabled = true
            }
            shortcut: "Ctrl+Shift+C"
        },
        Kirigami.Action {
            icon.name: "edit-paste"
            text: i18n("Paste")
            onTriggered: terminal.pasteClipboard();
            shortcut: "Ctrl+Shift+V"
        }
    ]

    ColumnLayout {
        anchors.fill: parent

        Kirigami.InlineMessage {
            visible: !scrollMouseArea.enabled
            implicitWidth: parent.width
            text: i18n("selection mode")

            onVisibleChanged: terminal.forceActiveFocus()

            type: Kirigami.MessageType.Information

            actions: [
                Kirigami.Action {
                    text: i18n("Disable")
                    onTriggered: scrollMouseArea.enabled = true
                }
            ]
        }

        QMLTermWidget {
            id: terminal
            Layout.fillWidth: true
            Layout.fillHeight: true
            font.family: "Monospace"
            colorScheme: TerminalSettings.colorScheme
            font.pixelSize: 12

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
                id: scrollMouseArea

                onPressAndHold: {
                    enabled = !enabled
                }

                onPositionChanged: {
                    terminal.simulateWheel(0, 0, 0, 0, Qt.point(0, (mouse.y - oldY)*2))
                    oldY = mouse.y
                }

                propagateComposedEvents: true
                anchors.fill: parent
                property real oldY
                onPressed: oldY = mouse.y
                onClicked: {
                    terminal.forceActiveFocus();
                    Qt.inputMethod.show();
                }
            }
        }
    }

    footer: ScrollView {
        visible: Kirigami.Settings.isMobile
        height: visible ? undefined : 0
        ToolBar {
            RowLayout {
                anchors.fill: parent

                TerminalKeyButton {
                    text: "Cancel"
                    onClicked: {
                        terminal.pressKey(Qt.Key_C, Qt.ControlModifier, true)
                    }
                }
                TerminalKeyButton {
                    text: "Esc"
                    onClicked: {
                        terminal.pressKey(Qt.Key_Escape, 0, true)
                    }
                }
                TerminalKeyButton {
                    Layout.maximumWidth: height
                    text: "Tab"
                    onClicked: terminal.pressKey(Qt.Key_Tab, 0, true, 0, "")
                }
                TerminalKeyButton {
                    Layout.maximumWidth: height
                    text: "←"
                    onClicked: terminal.pressKey(Qt.Key_Left, 0, true, 0, "")
                }
                TerminalKeyButton {
                    Layout.maximumWidth: height
                    text: "↑"
                    onClicked: terminal.pressKey(Qt.Key_Up, 0, true, 0, "")
                }
                TerminalKeyButton {
                    Layout.maximumWidth: height
                    text: "→"
                    onClicked: terminal.pressKey(Qt.Key_Right, 0, true, 0, "")
                }
                TerminalKeyButton {
                    Layout.maximumWidth: height
                    text: "↓"
                    onClicked: terminal.pressKey(Qt.Key_Down, 0, true, 0, "")
                }
                TerminalKeyButton {
                    Layout.maximumWidth: height
                    text: "|"
                    onClicked: terminal.pressKey(Qt.Key_Bar, 0, true, 0, "|")
                }
                TerminalKeyButton {
                    Layout.maximumWidth: height
                    text: "~"
                    onClicked: terminal.pressKey(Qt.Key_AsciiTilde, 0, true, 0, "~")
                }
            }
        }
    }

    Component.onCompleted: terminal.forceActiveFocus();
}
