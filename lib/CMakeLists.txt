# SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(KonsoleQML STATIC
    BlockArray.cpp
    ColorScheme.cpp
    Emulation.cpp
    Filter.cpp
    History.cpp
    HistorySearch.cpp
    KeyboardTranslator.cpp
    ProcessInfo.cpp
    Pty.cpp
    Screen.cpp
    ScreenWindow.cpp
    Session.cpp
    ShellCommand.cpp
    TerminalCharacterDecoder.cpp
    TerminalDisplay.cpp
    tools.cpp
    Vt102Emulation.cpp
    TerminalSession.cpp
    KonsoleQML.cpp
    terminal.qrc
)

target_link_libraries(KonsoleQML
    PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::Widgets
    KF${QT_MAJOR_VERSION}::CoreAddons
    KF${QT_MAJOR_VERSION}::Pty)

if(${QT_MAJOR_VERSION} EQUAL 6)
    target_link_libraries(KonsoleQML PUBLIC Qt::Core5Compat)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    # kinfo_getfile() is in libutil
    target_link_libraries(KonsoleQML PUBLIC util)
endif()

target_include_directories(KonsoleQML PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
