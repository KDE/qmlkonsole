project(qmlkonsole)
set(PROJECT_VERSION "21.12")

cmake_minimum_required(VERSION 2.8.12)
set(KF5_MIN_VERSION "5.18.0")
set(QT_MIN_VERSION "5.5.0")

################# Disallow in-source build #################

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "This application requires an out of source build. Please create a separate build directory.")
endif()

include(FeatureSummary)

################# set KDE specific information #################

find_package(ECM 0.0.8 REQUIRED NO_MODULE)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

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

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Test Gui Svg QuickControls2)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Kirigami2 I18n Config)

ecm_find_qmlmodule(QMLTermWidget 1.0)
set_package_properties(QMLTermWidget-QMLModule PROPERTIES
    URL "https://github.com/Swordfish90/qmltermwidget")

################# Enable C++11 features for clang and gcc #################

if(UNIX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++0x")
endif()

################# build and install #################
add_subdirectory(src)

install(PROGRAMS org.kde.mobile.qmlkonsole.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES org.kde.mobile.qmlkonsole.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
