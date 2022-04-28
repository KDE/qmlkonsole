// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021-2022 Devin Lin <espidev@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.15

import QMLTermWidget 1.0
import org.kde.kirigami 2.19 as Kirigami

import org.kde.qmlkonsole 1.0

Kirigami.Page {
    id: root
    property var currentTerminal: tabSwipeView.contentChildren[tabSwipeView.currentIndex].termWidget

    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    function forceTerminalFocus() {
        const wasVisible = Qt.inputMethod.visible;
        currentTerminal.forceActiveFocus();
        if (!wasVisible) {
            Qt.inputMethod.hide();
        }
    }
    
    Component.onCompleted: {
        // focus terminal text input immediately after load
        forceTerminalFocus();
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
            text: i18nc("@action:intoolbar", "New Tab")
            onTriggered: {
                TerminalTabModel.newTab();
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
                onTriggered: root.currentTerminal.touchSelectionMode = false
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
                    terminal: currentTerminal
                }
            )
        }
    ]

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        Kirigami.Dialog {
            id: confirmDialog
            
            property int indexToClose: 0
            property var selectedTerminal: tabSwipeView.contentChildren[indexToClose]
            
            title: i18n("Confirm closing %1", selectedTerminal ? selectedTerminal.termWidget.tabName : "")
            standardButtons: Dialog.Yes | Dialog.Cancel
            padding: Kirigami.Units.gridUnit
            
            onAccepted: {
                TerminalTabModel.removeTab(indexToClose);
                selectTabDialog.open();
            }
            onRejected: {
                selectTabDialog.open();
            }
            
            RowLayout {
                Label {
                    Layout.maximumWidth: Kirigami.Units.gridUnit * 15
                    wrapMode: Text.Wrap
                    text: i18n("A process is currently running in this tab. Are you sure you want to close it?")
                }
            }
        }
        
        Kirigami.Dialog {
            id: selectTabDialog
            title: i18nc("@title:window", "Select Tab")
            standardButtons: Dialog.Close
            
            ListView {
                implicitWidth: Kirigami.Units.gridUnit * 16
                implicitHeight: Kirigami.Units.gridUnit * 18
                Kirigami.Theme.inherit: false
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                model: TerminalTabModel
                
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
                                tabSwipeView.setCurrentIndex(index);
                                selectTabDialog.close();
                            }
                        }
                    }

                    actions: [
                        Kirigami.Action {
                            iconName: "delete"
                            text: i18n("Close")
                            onTriggered: {
                                if (tabSwipeView.contentChildren[index].termWidget.session.hasActiveProcess) {
                                    selectTabDialog.close();
                                    confirmDialog.indexToClose = index;
                                    confirmDialog.open();
                                } else {
                                    TerminalTabModel.removeTab(index);
                                }
                            }
                        }
                    ]
                }
            }
        }

        Kirigami.InlineMessage {
            id: selectionModePopup
            visible: root.currentTerminal.touchSelectionMode
            implicitWidth: parent.width
            text: i18n("selection mode")

            onVisibleChanged: root.forceTerminalFocus();

            type: Kirigami.MessageType.Information

            actions: [
                Kirigami.Action {
                    text: i18n("Disable")
                    onTriggered: root.currentTerminal.touchSelectionMode = false
                }
            ]
        }

        // tabs
        SwipeView {
            id: tabSwipeView
            interactive: !selectionModePopup.visible // don't conflict with selection mode
            
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            Repeater {
                id: terminalRepeater
                model: TerminalTabModel
                
                delegate: Item {
                    property alias termWidget: terminal
                    
                    QMLTermWidget {
                        id: terminal
                        anchors.fill: parent
                        
                        property string tabName: model.name
                        property bool isCurrentItem: SwipeView.isCurrentItem
                        
                        // with touch, to select text we first require users to press-and-hold to enter the selection mode
                        property bool touchSelectionMode: false
                        
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
                        
                        // terminal focus on mouse click
                        TapHandler {
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
                            cursorShape: Qt.IBeamCursor
                            onTapped: root.forceTerminalFocus();
                        }
                        
                        // enter touch selection mode
                        TapHandler {
                            acceptedDevices: PointerDevice.TouchScreen
                            enabled: !terminal.touchSelectionMode
                            onLongPressed: terminal.touchSelectionMode = true
                        }
                        
                        // simulate scrolling for touch (TODO velocity)
                        DragHandler {
                            acceptedDevices: PointerDevice.TouchScreen
                            enabled: !terminal.touchSelectionMode
                            
                            property real previousY
                            onActiveChanged: {
                                previousY = 0;
                            }
                            onTranslationChanged: {
                                terminal.simulateWheel(0, 0, 0, 0, Qt.point(0, (translation.y - previousY) * 2));
                                previousY = translation.y;
                            }
                        }
                    }
                }
            }
        }
    }

    footer: TerminalKeyToolBar {
        visible: Kirigami.Settings.isMobile
        terminal: root.currentTerminal
    }
}
