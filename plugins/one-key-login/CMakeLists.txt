find_package(QT NAMES Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

link_libraries(
    ${Qt_LIBS}
    Qt${QT_VERSION_MAJOR}::Widgets
    ${DtkCore_LIBRARIES}
    ${DtkWidget_LIBRARIES}
)

set(LIB_NAME one-key-login)

aux_source_directory(. SRCS)

add_library(${LIB_NAME} SHARED ${SRCS})

install(TARGETS ${LIB_NAME} LIBRARY DESTINATION lib/dde-session-shell/modules)
