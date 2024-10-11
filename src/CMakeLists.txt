# SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
# SPDX-License-Identifier: GPL-2.0-or-later

set(qmlkonsole_SRCS
    main.cpp
    savedcommandsmodel.cpp
    savedcommandsmodel.h
    terminaltabmodel.cpp
    terminaltabmodel.h
    util.cpp
    util.h
    fontlistmodel.cpp
    fontlistmodel.h
    shellcommand.cpp
    shellcommand.h
    resources.qrc
)

add_executable(qmlkonsole ${qmlkonsole_SRCS})

kconfig_add_kcfg_files(qmlkonsole GENERATE_MOC terminalsettings.kcfgc)
target_include_directories(qmlkonsole PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

target_link_libraries(qmlkonsole
    Qt::Core
    Qt::Qml
    Qt::Quick
    Qt::Svg
    Qt::Widgets
    KF6::I18n
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::WindowSystem
    KonsoleQML
)
install(TARGETS qmlkonsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES terminalsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
