# ltr_hotkey_gui - Native hotkey controller GUI

find_package(Qt5 COMPONENTS Widgets QUIET)
if(NOT Qt5_FOUND)
    find_package(Qt6 COMPONENTS Widgets REQUIRED)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(ltr_hotkey_gui
    main.cpp
    ltr_hotkey_gui.cpp
    ltr_hotkey_gui.h
)

if(Qt5_FOUND)
    target_link_libraries(ltr_hotkey_gui Qt5::Widgets)
else()
    target_link_libraries(ltr_hotkey_gui Qt6::Widgets)
endif()

install(TARGETS ltr_hotkey_gui DESTINATION bin)
