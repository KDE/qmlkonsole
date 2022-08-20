// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Devin Lin <devin@kde.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 1.1 as Dialogs

import org.kde.kirigami 2.19 as Kirigami
import QMLTermWidget 1.0

import org.kde.qmlkonsole 1.0
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

ColumnLayout {
    id: root
    property QMLTermWidget terminal
    property var dialog: null // dialog component if this is within a dialog
    
    spacing: 0
    
    // HACK: dialog switching requires some time between closing and opening
    Timer {
        id: dialogTimer
        interval: 1
        property var dialog
        onTriggered: {
            root.dialog.close();
            dialog.open();
        }
    }
    
    MobileForm.FormCard {
        Layout.alignment: Qt.AlignTop
        Layout.fillWidth: true
        
        contentItem: ColumnLayout {
            spacing: 0

            MobileForm.FormCardHeader {
                title: i18n("General")
            }
            
            MobileForm.FormButtonDelegate {
                id: aboutDelegate
                text: i18n("About")
                onClicked: {
                    if (root.dialog) {
                        root.dialog.close();
                    }
                    applicationWindow().pageStack.push("qrc:/AboutPage.qml")
                }
            }
        }
    }
    
    MobileForm.FormCard {
        Layout.alignment: Qt.AlignTop
        Layout.topMargin: Kirigami.Units.largeSpacing
        Layout.fillWidth: true
        
        contentItem: ColumnLayout {
            spacing: 0

            MobileForm.FormCardHeader {
                title: i18n("Appearance")
            }
            
            MobileForm.FormComboBoxDelegate {
                id: colorSchemeDropdown
                text: i18n("Color scheme")
                currentValue: TerminalSettings.colorScheme
                model: terminal.availableColorSchemes
                
                onClicked: {
                    if (root.dialog) {
                        dialogTimer.dialog = colorSchemeDropdown.dialog;
                        dialogTimer.restart();
                    }
                }
                
                Connections {
                    target: colorSchemeDropdown.dialog
                    function onClosed() {
                        if (root.dialog) {
                            root.dialog.open();
                        }
                    }
                }
                
                dialogDelegate: Controls.RadioDelegate {
                    implicitWidth: Kirigami.Units.gridUnit * 16
                    topPadding: Kirigami.Units.smallSpacing * 2
                    bottomPadding: Kirigami.Units.smallSpacing * 2
                    
                    text: modelData
                    checked: colorSchemeDropdown.currentValue == modelData
                    onCheckedChanged: {
                        if (checked) {
                            colorSchemeDropdown.currentValue = modelData;
                            TerminalSettings.colorScheme = modelData;
                            TerminalSettings.save();
                        }
                    }
                }
            }
            
            MobileForm.FormDelegateSeparator { above: colorSchemeDropdown; below: fontFamilyDelegate }
            
            MobileForm.AbstractFormDelegate {
                id: fontFamilyDelegate
                Layout.fillWidth: true
                text: i18n("Font Family")
                
                onClicked: {
                    if (fontFamilyPickerLoader.active) {
                        fontFamilyPickerLoader.item.open();
                    } else {
                        fontFamilyPickerLoader.active = true;
                        fontFamilyPickerLoader.requestOpen = true;
                    }
                }
                
                contentItem: RowLayout {
                    Controls.Label {
                        Layout.fillWidth: true
                        text: i18n("Font Family")
                        elide: Text.ElideRight
                    }
                    
                    Controls.Label {
                        Layout.alignment: Qt.AlignRight
                        Layout.rightMargin: Kirigami.Units.smallSpacing
                        color: Kirigami.Theme.disabledTextColor
                        text: TerminalSettings.fontFamily
                        font.family: TerminalSettings.fontFamily
                    }
                    
                    MobileForm.FormArrow {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        direction: MobileForm.FormArrow.Down
                    }
                }
                
                Loader {
                    id: fontFamilyPickerLoader
                    active: false
                    asynchronous: true
                    
                    property bool requestOpen: false
                    onLoaded: {
                        if (requestOpen) {
                            item.open();
                            requestOpen = false;
                        }
                    }
                    
                    sourceComponent: Kirigami.Dialog {
                        id: fontFamilyPicker
                        title: i18nc("@title:window", "Pick font")
                        
                        onClosed: {
                            if (root.dialog) {
                                root.dialog.open();
                            }
                        }
                        
                        onOpened: {
                            if (root.dialog) {
                                root.dialog.close();
                            }
                        }
                        
                        ListView {
                            implicitWidth: Kirigami.Units.gridUnit * 18
                            implicitHeight: Kirigami.Units.gridUnit * 24

                            reuseItems: true
                            model: FontListSearchModel
                            currentIndex: -1
                            clip: true
                            
                            header: Controls.Control {
                                topPadding: Kirigami.Units.smallSpacing
                                bottomPadding: Kirigami.Units.smallSpacing
                                rightPadding: Kirigami.Units.smallSpacing
                                leftPadding: Kirigami.Units.smallSpacing
                                width: fontFamilyPicker.width
                                
                                contentItem: Kirigami.SearchField {
                                    id: searchField
                                    onTextChanged: {
                                        FontListSearchModel.setFilterFixedString(text)
                                        searchField.forceActiveFocus()
                                    }
                                }
                            }
                            
                            delegate: Kirigami.BasicListItem {
                                text: model.name
                                onClicked: {
                                    TerminalSettings.fontFamily = model.name;
                                    TerminalSettings.save();
                                    fontFamilyPicker.close();
                                }
                            }
                        }
                    }
                }
            }
            
            MobileForm.FormDelegateSeparator { above: fontFamilyDelegate }
            
            MobileForm.AbstractFormDelegate {
                id: fontSizeDelegate
                Layout.fillWidth: true
                background: Item {}
                
                contentItem: RowLayout {
                    width: fontSizeDelegate.width
                    Controls.Label {
                        Layout.fillWidth: true
                        text: i18n("Font Size")
                    }
                    
                    Controls.SpinBox {
                        value: TerminalSettings.fontSize
                        onValueChanged: {
                            TerminalSettings.fontSize = value;
                            TerminalSettings.save();
                        }
                        from: 5
                        to: 100
                    }
                }
            }
            
            MobileForm.FormDelegateSeparator { below: fontFamilyDelegate }
            
            MobileForm.FormComboBoxDelegate {
                id: opacityDelegate
                text: i18n("Window Transparency")
                currentValue: i18n("%1\%", sliderValue.value)
                
                onClicked: {
                    dialog.open();
                    if (root.dialog) {
                        dialogTimer.dialog = opacityDelegate.dialog;
                        dialogTimer.restart();
                    }
                }
                
                Connections {
                    target: opacityDelegate.dialog
                    function onClosed() {
                        if (root.dialog) {
                            root.dialog.open();
                        }
                    }
                }
                
                dialog: Kirigami.PromptDialog {
                    showCloseButton: false
                    title: i18n("Window Transparency")
                    
                    RowLayout {
                        Controls.Slider {
                            id: sliderValue
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            value: (1 - TerminalSettings.windowOpacity) * 100
                            stepSize: 5
                            snapMode: Controls.Slider.SnapAlways
                            
                            onMoved: {
                                TerminalSettings.windowOpacity = 1 - (value / 100);
                                TerminalSettings.save();
                            }
                        }
                        Controls.Label {
                            text: opacityDelegate.currentValue
                        }
                    }
                }
            }
        }
    }
    
    Item { Layout.fillHeight: true }
}
