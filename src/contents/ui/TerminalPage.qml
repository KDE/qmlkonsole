// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Devin Lin <espidev@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.15

import QMLTermWidget 1.0
import org.kde.kirigami 2.7 as Kirigami

import org.kde.qmlkonsole 1.0

Kirigami.Page {
    id: root
    property var currentTerminal: tabSwipeView.contentChildren[tabSwipeView.currentIndex]

    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    function forceTerminalFocus() {
        let wasVisible = Qt.inputMethod.visible;
        currentTerminal.forceActiveFocus();
        if (!wasVisible) {
            Qt.inputMethod.hide();
        }
    }
    
    // switch tab button
    titleDelegate: ToolButton {
        Layout.fillHeight: true
        onClicked: selectTabDialog.open()
        contentItem: RowLayout {
            spacing: 0
            Kirigami.Heading {
                text: currentTerminal.tabName
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
            }
            Kirigami.Icon {
                Layout.alignment: Qt.AlignVCenter
                Layout.rightMargin: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
                implicitHeight: Kirigami.Units.iconSizes.small
                implicitWidth: Kirigami.Units.iconSizes.small
                source: "go-down"
            }
        }
    }
    
    contextualActions: [
        Kirigami.Action {
            icon.name: "list-add"
            text: i18n("New Tab")
            onTriggered: {
                terminalTabModel.newTab();
                tabSwipeView.currentIndex = tabSwipeView.contentChildren.length - 1;
            }
            shortcut: "Ctrl+Shift+T"
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "search"
            text: i18n("reverse-i-search")
            onTriggered: {
                currentTerminal.pressKey(Qt.Key_R, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "dialog-cancel"
            text: i18n("Cancel current command")
            onTriggered: {
                currentTerminal.pressKey(Qt.Key_C, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "document-close"
            text: i18n("Send EOF")
            onTriggered: {
                currentTerminal.pressKey(Qt.Key_D, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "bboxprev"
            text: i18n("Cursor to line start")
            onTriggered: {
                currentTerminal.pressKey(Qt.Key_A, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "bboxnext"
            text: i18n("Cursor to line end")
            onTriggered: {
                currentTerminal.pressKey(Qt.Key_E, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "edit-cut"
            text: i18n("Kill to line end")
            onTriggered: {
                currentTerminal.pressKey(Qt.Key_K, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "edit-paste"
            text: i18n("Paste from kill buffer")
            onTriggered: {
                currentTerminal.pressKey(Qt.Key_Y, Qt.ControlModifier, true)
            }
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "edit-copy"
            text: i18n("Copy")
            onTriggered: {
                currentTerminal.copyClipboard();
                onTriggered: root.currentTerminal.scrollMouseArea.enabled = true
            }
            shortcut: "Ctrl+Shift+C"
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            icon.name: "edit-paste"
            text: i18n("Paste")
            onTriggered: terminal.pasteClipboard();
            shortcut: "Ctrl+Shift+V"
        },
        Kirigami.Action {
            displayHint: Kirigami.Action.AlwaysHide
            text: i18n("Settings")
            icon.name: "settings-configure"
            onTriggered: pageStack.layers.push("qrc:/SettingsPage.qml",
                {
                    "terminal": currentTerminal
                }
            )
        }
    ]

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        PopupDialog {
            id: selectTabDialog
            title: i18n("Select Tab")
            standardButtons: Dialog.Close
            
            ListView {
                implicitWidth: Kirigami.Units.gridUnit * 16
                implicitHeight: Kirigami.Units.gridUnit * 18
                Kirigami.Theme.inherit: false
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                model: terminalTabModel
                
                delegate: Kirigami.SwipeListItem {
                    topPadding: Kirigami.Units.smallSpacing
                    bottomPadding: Kirigami.Units.smallSpacing
                    alwaysVisibleActions: true
                    onClicked: {
                        tabSwipeView.currentIndex = index;
                        selectTabDialog.close();
                    }
                    
                    RowLayout {
                        Label {
                            text: model.name
                        }
                        RadioButton {
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                            checked: tabSwipeView.currentIndex == index
                            onClicked: {
                                tabSwipeView.currentIndex = index;
                                selectTabDialog.close();
                            }
                        }
                    }

                    actions: [
                        Kirigami.Action {
                            iconName: "delete"
                            text: i18n("Close")
                            onTriggered: terminalTabModel.removeTab(index)
                        }
                    ]
                }
            }
        }

        Kirigami.InlineMessage {
            visible: !root.currentTerminal.scrollMouseArea.enabled
            implicitWidth: parent.width
            text: i18n("selection mode")

            onVisibleChanged: root.forceTerminalFocus();

            type: Kirigami.MessageType.Information

            actions: [
                Kirigami.Action {
                    text: i18n("Disable")
                    onTriggered: root.currentTerminal.scrollMouseArea.enabled = true
                }
            ]
        }

        // tabs
        SwipeView {
            id: tabSwipeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            Repeater {
                id: terminalRepeater
                model: terminalTabModel
                
                delegate: QMLTermWidget {
                    id: terminal
                    property string tabName: model.name
                    property bool isCurrentItem: SwipeView.isCurrentItem
                    
                    property alias scrollMouseArea: scrollMouseArea
                    
                    onIsCurrentItemChanged: {
                        if (isCurrentItem) {
                            root.forceTerminalFocus();
                        }
                    }
                    
                    font.family: "Monospace"
                    colorScheme: TerminalSettings.colorScheme
                    font.pixelSize: 12

                    Component.onCompleted: {
                        mainsession.startShellProgram();                        
                        root.forceTerminalFocus();
                    }

                    function pressKey(key, modifiers, pressed, nativeScanCode, text) {
                        terminal.simulateKeyPress(key, modifiers, pressed, nativeScanCode, text);
                        root.forceTerminalFocus();
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
                        
                        TapHandler {
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
                            onTapped: root.forceTerminalFocus();
                        }
                    }
                }
            }
        }
    }

    footer: ToolBar {
        visible: Kirigami.Settings.isMobile
        height: visible ? implicitHeight : 0
        contentItem: RowLayout {
            spacing: Kirigami.Units.smallSpacing
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
                            currentTerminal.pressKey(Qt.Key_Escape, 0, true)
                        }
                    }
                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: i18nc("Tab character key", "Tab")
                        onClicked: currentTerminal.pressKey(Qt.Key_Tab, 0, true, 0, "")
                    }

                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: "↑"
                        onClicked: currentTerminal.pressKey(Qt.Key_Up, 0, true, 0, "")
                        autoRepeat: true
                    }
                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: "↓"
                        onClicked: currentTerminal.pressKey(Qt.Key_Down, 0, true, 0, "")
                        autoRepeat: true
                    }
                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: "←"
                        onClicked: currentTerminal.pressKey(Qt.Key_Left, 0, true, 0, "")
                        autoRepeat: true
                    }
                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: "→"
                        onClicked: currentTerminal.pressKey(Qt.Key_Right, 0, true, 0, "")
                        autoRepeat: true
                    }
                    
                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: "|"
                        onClicked: currentTerminal.pressKey(Qt.Key_Bar, 0, true, 0, "|")
                    }
                    TerminalKeyButton {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                        text: "~"
                        onClicked: currentTerminal.pressKey(Qt.Key_AsciiTilde, 0, true, 0, "~")
                    }
                }
            }
            Kirigami.Separator { Layout.fillHeight: true }
            ToolButton {
                Layout.preferredWidth: height
                Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                icon.name: "input-keyboard-virtual"
                text: i18n("Toggle Virtual Keyboard")
                display: AbstractButton.IconOnly
                onClicked: {
                    if (Qt.inputMethod.visible) {
                        Qt.inputMethod.hide();
                    } else {
                        currentTerminal.forceActiveFocus();
                        Qt.inputMethod.show();
                    }
                }
            }
        }
    }
}
