set(qmlkonsole_SRCS
    main.cpp
    )

qt5_add_resources(RESOURCES resources.qrc)
add_executable(qmlkonsole ${qmlkonsole_SRCS} ${RESOURCES})
target_link_libraries(qmlkonsole Qt5::Core  Qt5::Qml Qt5::Quick Qt5::Svg)
install(TARGETS qmlkonsole ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
