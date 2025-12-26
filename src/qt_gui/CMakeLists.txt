# src/qt_gui/CMakeLists.txt

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets Gui Network Help OpenGL OpenGLWidgets REQUIRED)

# Common headers
set(LTR_GUI_HEADERS
    ltr_gui.h ltr_show.h ltr_dev_help.h help_view.h
    ltr_gui_prefs.h prefs_link.h tir_prefs.h device_setup.h
    ltr_model.h window.h glwidget.h objreader.h ltr_profiles.h
    log_view.h ltr_state.h scp_form.h buffering.h progress.h 
    scurve.h scview.h wiimote_prefs.h tracker.h plugin_install.h 
    profile_setup.h profile_selector.h guardian.h xplugin.h wine_warn.h 
    extractor.h hashing.h downloading.h wine_launcher.h 
    macps3eye_prefs.h macwebcam_info.h macps3eyeft_prefs.h 
    help_viewer.h prefix_discovery.h prefix_discovery_dialog.h
    webcam_prefs.h webcam_info.h webcam_ft_prefs.h joy_prefs.h
)

# Common forms
set(LTR_GUI_FORMS
    ltr_gui.ui dev_help.ui ltr.ui model_creation.ui scurve.ui scp_form.ui 
    logview.ui wii_setup.ui tir_setup.ui 
    cap_edit.ui clip_edit.ui single_edit.ui model_edit.ui 
    device_setup.ui profile_setup.ui profile_selector.ui cap_tweaking.ui clip_tweaking.ui 
    extractor.ui progress.ui xplugin.ui wine_warn.ui m_ps3eye_setup.ui m_ps3eye_ft_setup.ui 
    prefix_discovery_dialog.ui
    l_wc_setup.ui l_wcft_setup.ui joy_setup.ui
)

# Common sources
set(LTR_GUI_SOURCES
    main.cpp ltr_gui.cpp ltr_show.cpp ltr_dev_help.cpp 
    ltr_gui_prefs.cpp tir_prefs.cpp help_view.cpp buffering.cpp 
    ltr_model.cpp window.cpp glwidget.cpp objreader.cpp ltr_profiles.cpp 
    log_view.cpp ltr_state.cpp scp_form.cpp guardian.cpp 
    scurve.cpp scview.cpp wiimote_prefs.cpp 
    tracker.cpp device_setup.cpp 
    plugin_install.cpp profile_setup.cpp 
    profile_selector.cpp xplugin.cpp wine_warn.cpp progress.cpp 
    extractor.cpp hashing.cpp downloading.cpp wine_launcher.cpp 
    macps3eye_prefs.cpp macwebcam_info.cpp macps3eyeft_prefs.cpp 
    help_viewer.cpp prefix_discovery.cpp 
    prefix_discovery_dialog.cpp
    webcam_info.cpp webcam_prefs.cpp webcam_ft_prefs.cpp joy_prefs.cpp
    lal_dialog.cpp
    
    # Files from parent directory
    ../ltr_srv_master.cpp
    ../ltr_srv_slave.c
    ../ltr_srv_comm.c
    ../game_data.c
    ../extract.c
    ../digest.c
)

# Resources
set(LTR_GUI_RESOURCES ltr_rc.qrc)

add_executable(ltr_gui ${LTR_GUI_SOURCES} ${LTR_GUI_FORMS} ${LTR_GUI_RESOURCES})

target_include_directories(ltr_gui PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../lal
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/src
)

target_compile_definitions(ltr_gui PRIVATE 
    HAVE_CONFIG_H
    LTR_GUI
    HELP_BASE="ltr_gui/"
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

target_link_libraries(ltr_gui PRIVATE 
    Qt6::Widgets 
    Qt6::Gui 
    Qt6::Network 
    Qt6::Help 
    Qt6::OpenGL 
    Qt6::OpenGLWidgets
    ltr 
    linuxtrack
    lal 
    ${GLU_LIBRARY}
    ${MXML_LIBRARIES}
    ${LTR_LIBM}
)

# Install executable
install(TARGETS ltr_gui DESTINATION bin)

# Install data files (as per .pro file)
set(LTR_GUI_DATA_FILES
    sparow_opaq.obj sparow_glass.obj xm8_detail.png sources.txt spec.txt 
    sphere.obj sphere.png sources_mfc.txt win7.reg
)
install(FILES ${LTR_GUI_DATA_FILES} DESTINATION share/linuxtrack)

# Install help files
install(FILES help.qhc help.qch DESTINATION share/linuxtrack/help/ltr_gui)
