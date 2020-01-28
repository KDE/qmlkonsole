set(qmlkonsole_SRCS
    main.cpp
    settings.cpp
)

qt5_add_resources(RESOURCES resources.qrc)
add_executable(qmlkonsole ${qmlkonsole_SRCS} ${RESOURCES})
target_link_libraries(qmlkonsole Qt5::Core  Qt5::Qml Qt5::Quick Qt5::Svg KF5::I18n)
install(TARGETS qmlkonsole ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
