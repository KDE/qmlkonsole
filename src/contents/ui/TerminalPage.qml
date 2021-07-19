// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Devin Lin <espidev@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
            icon.name: "search"
            text: i18n("reverse-i-search")
            onTriggered: {
                terminal.pressKey(Qt.Key_R, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            icon.name: "dialog-cancel"
            text: i18n("Cancel current command")
            onTriggered: {
                terminal.pressKey(Qt.Key_C, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            icon.name: "document-close"
            text: i18n("Send EOF")
            onTriggered: {
                terminal.pressKey(Qt.Key_D, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            icon.name: "bboxprev"
            text: i18n("Cursor to line start")
            onTriggered: {
                terminal.pressKey(Qt.Key_A, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            icon.name: "bboxnext"
            text: i18n("Cursor to line end")
            onTriggered: {
                terminal.pressKey(Qt.Key_E, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            icon.name: "edit-cut"
            text: i18n("Kill to line end")
            onTriggered: {
                terminal.pressKey(Qt.Key_K, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            icon.name: "edit-paste"
            text: i18n("Paste from kill buffer")
            onTriggered: {
                terminal.pressKey(Qt.Key_Y, Qt.ControlModifier, true)
            }
        },
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

            session: QMLTermSession {
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

            Keys.onPressed: {
                if(ctrlButton.modifier | altButton.modifier)
                {
                    pressKey(event.key, ctrlButton.modifier | altButton.modifier, true);
                    ctrlButton.down = false;
                    altButton.down = false;
                    event.accepted = true;
                    ctrlButton.modifier = 0;
                    altButton.modifier = 0;
                }
            }

            onTerminalUsesMouseChanged: console.log(terminalUsesMouse);
            Component.onCompleted: mainsession.startShellProgram();

            ScrollBar {
                Kirigami.Theme.colorSet: Kirigami.Theme.Complementary // text color of terminal is also complementary
                Kirigami.Theme.inherit: false
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                }
                visible: true
                orientation: Qt.Vertical
                size: (terminal.lines / (terminal.lines + terminal.scrollbarMaximum - terminal.scrollbarMinimum))
                position: terminal.scrollbarCurrentValue / (terminal.lines + terminal.scrollbarMaximum)
                interactive: false
            }

            MouseArea {
                id: scrollMouseArea
                property bool scrolling: false

                onPressAndHold: {
                    enabled = scrolling ? enabled : !enabled
                }

                onPositionChanged: {
                    scrolling = scrolling ? true : Math.abs(mouse.y - oldY) >= 1
                    terminal.simulateWheel(0, 0, 0, 0, Qt.point(0, (mouse.y - oldY)*2))
                    oldY = mouse.y
                }

                propagateComposedEvents: true
                anchors.fill: parent
                property real oldY
                onPressed: {
                    scrolling = false
                    oldY = mouse.y
                }
                onClicked: {
                    terminal.forceActiveFocus();
                    Qt.inputMethod.show();
                    scrolling = false
                }
            }
        }
    }

    footer: ToolBar {
        visible: Kirigami.Settings.isMobile
        height: visible ? implicitHeight : 0
        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing
            ToolButton {
                Layout.preferredWidth: height
                Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                icon.name: "application-menu"
                onClicked: applicationWindow().globalDrawer.open()
            }
            Kirigami.Separator { Layout.fillHeight: true }
            ScrollView {
                Layout.fillWidth: true
                clip: true
                RowLayout {
                    TerminalModifierButton {
                        id: ctrlButton
                        Layout.preferredWidth: Math.round(Kirigami.Units.gridUnit * 2)
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                        text: i18nc("Control Key", "Ctrl")
                        onClicked: {
                            modifier = modifier ^ Qt.ControlModifier
                            down = !down;
                        }
                    }

                    TerminalModifierButton {
                        id: altButton
                        Layout.preferredWidth: Math.round(Kirigami.Units.gridUnit * 1.75)
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                        text: i18nc("Alt Key", "Alt")
                        onClicked: {
                            modifier = modifier ^ Qt.AltModifier
                            down = !down;
                        }
                    }

                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: i18nc("Escape key", "Esc")
                        onClicked: {
                            terminal.pressKey(Qt.Key_Escape, 0, true)
                        }
                    }
                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: i18nc("Tab character key", "Tab")
                        onClicked: terminal.pressKey(Qt.Key_Tab, 0, true, 0, "")
                    }
                    TerminalKeyButton {
                        text: "←"
                        onClicked: terminal.pressKey(Qt.Key_Left, 0, true, 0, "")
                        autoRepeat: true
                    }
                    TerminalKeyButton {
                        text: "↑"
                        onClicked: terminal.pressKey(Qt.Key_Up, 0, true, 0, "")
                        autoRepeat: true
                    }
                    TerminalKeyButton {
                        text: "→"
                        onClicked: terminal.pressKey(Qt.Key_Right, 0, true, 0, "")
                        autoRepeat: true
                    }
                    TerminalKeyButton {
                        text: "↓"
                        onClicked: terminal.pressKey(Qt.Key_Down, 0, true, 0, "")
                        autoRepeat: true
                    }
                    TerminalKeyButton {
                        text: "|"
                        onClicked: terminal.pressKey(Qt.Key_Bar, 0, true, 0, "|")
                    }
                    TerminalKeyButton {
                        text: "~"
                        onClicked: terminal.pressKey(Qt.Key_AsciiTilde, 0, true, 0, "~")
                    }
                }
            }
            Kirigami.Separator { Layout.fillHeight: true }
            ToolButton {
                Layout.preferredWidth: height
                Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                icon.name: "overflow-menu"
                onClicked: applicationWindow().contextDrawer.open()
            }
        }
    }

    Component.onCompleted: terminal.forceActiveFocus();
}
