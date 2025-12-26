# src/mickey/CMakeLists.txt

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets Gui Help REQUIRED)
find_package(X11 REQUIRED)

# Common headers
set(MICKEY_HEADERS
    mickey.h
    ../utils.h
    ../math_utils.h
    ../ipc_utils.h
    ../ltlib.h
    ../linuxtrack.h
    uinput_ifc.h
    piper.h
    keyb.h
    keyb_x11.h
    transform.h
    ../qt_gui/help_view.h
    ../qt_gui/help_viewer.h
    ../qt_gui/ltr_gui_prefs.h
    mouse.h
    hotkey.h
    my_line_edit.h
    hotkey_setup_dlg.h
)

# Common forms
set(MICKEY_FORMS
    mickey.ui
    calibration.ui
    chsettings.ui
    ../qt_gui/logview.ui
    hotkey.ui
    hotkey_setup.ui
)

# Common sources
set(MICKEY_SOURCES
    main.cpp
    mickey.cpp
    ../ltlib.c
    ../math_utils.c
    uinput_ifc.c
    piper.c
    keyb.cpp
    transform.cpp
    ../qt_gui/help_view.cpp
    ../qt_gui/help_viewer.cpp
    ../qt_gui/ltr_gui_prefs.cpp
    keyb_x11.cpp
    ../linuxtrack.c
    hotkey.cpp
    my_line_edit.cpp
    hotkey_setup_dlg.cpp
    mouse_linux.cpp
)

# Resources
set(MICKEY_RESOURCES mickey.qrc)

add_executable(mickey ${MICKEY_SOURCES} ${MICKEY_FORMS} ${MICKEY_RESOURCES})

target_include_directories(mickey PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../qt_gui
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
)

target_compile_definitions(mickey PRIVATE 
    HAVE_CONFIG_H
    HELP_BASE="mickey/"
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT6_OVERRIDES
)

target_link_libraries(mickey PRIVATE 
    Qt6::Widgets 
    Qt6::Gui 
    Qt6::Help 
    ltr 
    linuxtrack 
    ${X11_LIBRARIES}
    ${LTR_LIBM}
    ${LTR_LIBDL}
)

# Install executable
install(TARGETS mickey DESTINATION bin)

# Install help files
install(FILES help.qhc help.qch DESTINATION share/linuxtrack/help/mickey)
