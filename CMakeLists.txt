# SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.16)

set(PROJECT_VERSION "23.01")
project(qmlkonsole VERSION ${PROJECT_VERSION})

set(KF5_MIN_VERSION "5.101.0")
set(QT_MIN_VERSION "5.5.0")

################# Disallow in-source build #################

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "This application requires an out of source build. Please create a separate build directory.")
endif()

include(FeatureSummary)

################# set KDE specific information #################

find_package(ECM 0.0.8 REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(ECMPoQmTools)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQMLModules)

# setup version
ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX QMLKONSOLE
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/version.h
)
################# Find dependencies #################

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS 
    Core 
    Quick 
    Test 
    Gui 
    Svg 
    QuickControls2
)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS 
    Kirigami2 
    I18n 
    Config 
    CoreAddons 
    WindowSystem
)
find_package(KF5KirigamiAddons 0.6 REQUIRED)

ecm_find_qmlmodule(QMLTermWidget 1.0)
set_package_properties(QMLTermWidget-QMLModule PROPERTIES
    URL "https://github.com/Swordfish90/qmltermwidget")

################# Enable C++11 features for clang and gcc #################

if(UNIX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++0x")
endif()

################# build and install #################
add_subdirectory(src)

ki18n_install(po)

install(PROGRAMS org.kde.qmlkonsole.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.qmlkonsole.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES org.kde.qmlkonsole.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
