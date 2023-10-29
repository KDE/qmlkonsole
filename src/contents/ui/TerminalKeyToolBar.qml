// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Devin Lin <espidev@gmail.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QMLTermWidget
import org.kde.kirigami as Kirigami

import org.kde.qmlkonsole

ToolBar {
    id: root
    
    property var terminal
    
    height: visible ? implicitHeight : 0
    
    function pressKeyWithModifier(key, modifier) {
        terminal.pressKey(key, modifier, true);
        modifierHolder.forceActiveFocus();
    }
    
    function pressModifierButton(modifier) {
        modifierHolder.text = "";
        modifierHolder.forceActiveFocus();
    }
    
    function pressKey(key, keyString) {
        if (ctrlButton.checked) {
            pressKeyWithModifier(key, Qt.ControlModifier);
        } else if (altButton.checked) {
            pressKeyWithModifier(key, Qt.AltModifier);
        } else {
            terminal.pressKey(key, 0, true, 0, keyString);
        }
    }
    
    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing
        
        // HACK: in order to add ctrl and alt button functionality, we have the keyboard enter the character into this invisible
        //       textfield, and then once a character is entered, send it to the terminal as a keypress.
        TextField {
            id: modifierHolder
            visible: false
            inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            
            onTextChanged: {
                if (text.length == 1) {
                    root.pressKey(Util.getKeyFromString(text), text);
                }
                text = "";
            }
        }
        
        // modifier buttons row
        ScrollView {
            Layout.fillWidth: true
            clip: true
            
            RowLayout {
                property real delegateWidth: root.width 
                
                TerminalKeyButton {
                    id: ctrlButton
                    Layout.preferredWidth: Math.max(implicitWidth + Kirigami.Units.smallSpacing * 2, Math.round(Kirigami.Units.gridUnit * 2))
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                    text: i18nc("Control Key (should match the key on the keyboard of the locale)", "Ctrl")
                    checkable: true
                    
                    onClicked: {
                        if (checked) {
                            altButton.checked = false;
                            root.pressModifierButton(Qt.ControlModifier);
                        } else {
                            terminal.forceActiveFocus();
                        }
                        Qt.inputMethod.show();
                    }
                }
                
                TerminalKeyButton {
                    id: altButton
                    Layout.preferredWidth: Math.max(implicitWidth + Kirigami.Units.smallSpacing * 2, Math.round(Kirigami.Units.gridUnit * 1.75))
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 2
                    text: i18nc("Alt Key (should match the key on the keyboard of the locale)", "Alt")
                    checkable: true
                    
                    onClicked: {
                        if (checked) {
                            ctrlButton.checked = false;
                            root.pressModifierButton(Qt.AltModifier);
                        } else {
                            terminal.forceActiveFocus();
                        }
                        Qt.inputMethod.show();
                    }
                }

                TerminalKeyButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                    text: i18nc("Escape key (should match the key on the keyboard of the locale)", "Esc")
                    onClicked: {
                        terminal.pressKey(Qt.Key_Escape, 0, true)
                    }
                }
                TerminalKeyButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                    text: i18nc("Tab character key (should match the key on the keyboard of the locale)", "Tab")
                    onClicked: terminal.pressKey(Qt.Key_Tab, 0, true, 0, "")
                }

                TerminalKeyButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                    text: "↑"
                    onClicked: root.pressKey(Qt.Key_Up, "")
                    autoRepeat: true
                }
                TerminalKeyButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                    text: "↓"
                    onClicked: root.pressKey(Qt.Key_Down, "")
                    autoRepeat: true
                }
                TerminalKeyButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                    text: "←"
                    onClicked: root.pressKey(Qt.Key_Left, "")
                    autoRepeat: true
                }
                TerminalKeyButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                    text: "→"
                    onClicked: root.pressKey(Qt.Key_Right, "")
                    autoRepeat: true
                }
                
                TerminalKeyButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                    text: "|"
                    onClicked: root.pressKey(Qt.Key_Bar, "|")
                }
                TerminalKeyButton {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 2
                    text: "~"
                    onClicked: root.pressKey(Qt.Key_AsciiTilde, "~")
                }
            }
        }
        
        Kirigami.Separator { Layout.fillHeight: true }
        
        // control vkbd visibility
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
                    terminal.forceActiveFocus();
                    Qt.inputMethod.show();
                }
            }
        }
    }
}
