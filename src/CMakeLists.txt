# SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
# SPDX-License-Identifier: GPL-2.0-or-later

set(qmlkonsole_SRCS
    main.cpp
    abouttype.cpp
    savedcommandsmodel.cpp
    terminaltabmodel.cpp
    util.cpp
    fontlistmodel.cpp
)

qt5_add_resources(RESOURCES resources.qrc)
add_executable(qmlkonsole ${qmlkonsole_SRCS} ${RESOURCES})

kconfig_add_kcfg_files(qmlkonsole GENERATE_MOC terminalsettings.kcfgc)
target_include_directories(qmlkonsole PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR})

target_link_libraries(qmlkonsole Qt5::Core Qt5::Qml Qt5::Quick Qt5::Svg KF5::I18n KF5::ConfigGui KF5::CoreAddons)
install(TARGETS qmlkonsole ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES terminalsettings.kcfg DESTINATION ${KCFG_INSTALL_DIR})
