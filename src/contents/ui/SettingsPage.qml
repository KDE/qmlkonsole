﻿// SPDX-FileCopyrightText: 2019-2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.19 as Kirigami
import QMLTermWidget 1.0

import org.kde.qmlkonsole 1.0
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

Kirigami.ScrollablePage {
    id: root
    title: i18n("Settings")
    property QMLTermWidget terminal
    
    topPadding: Kirigami.Units.gridUnit
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    
    ColumnLayout {
        spacing: 0
        width: root.width
        
        MobileForm.FormCard {
            Layout.fillWidth: true
            
            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: "General"
                }
                
                MobileForm.FormButtonDelegate {
                    id: aboutDelegate
                    text: i18n("About")
                    onClicked: applicationWindow().pageStack.push("qrc:/AboutPage.qml")
                }
            }
        }
        
        MobileForm.FormCard {
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.fillWidth: true
            
            contentItem: ColumnLayout {
                spacing: 0

                MobileForm.FormCardHeader {
                    title: "General"
                }
                
                MobileForm.FormComboBoxDelegate {
                    id: colorSchemeDropdown
                    text: i18n("Color scheme")
                    currentValue: TerminalSettings.colorScheme
                    model: terminal.availableColorSchemes
                    
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
                            }
                        }
                    }
                }
                
                MobileForm.FormDelegateSeparator { above: colorSchemeDropdown; below: fontFamilyDelegate }
                
                MobileForm.FormButtonDelegate {
                    id: fontFamilyDelegate
                    text: i18n("Font Family")
//                     onClicked: applicationWindow().pageStack.push("qrc:/AboutPage.qml")
                    
                    //Kirigami.Dialog {
                        //id: fontFamilyDelegate
                    //}
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
                            onValueChanged: TerminalSettings.fontSize = value
                        }
                    }
                }
            }
        }
    }
}
