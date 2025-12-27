# libltr (internal core library)
add_library(ltr SHARED
    cal.c cal.h list.c list.h dyn_load.c dyn_load.h
    math_utils.c math_utils.h pose.c pose.h pref.cpp 
    modern_prefs.cpp modern_prefs.h mini_ini.h pref.hpp pref.h
    pref_global.c pref_global.h utils.c utils.h 
    image_process.c image_process.h tracking.c tracking.h
    ltlib_int.c ltlib_int.h spline.c spline.h axis.c axis.h 
    wii_driver_prefs.c wii_driver_prefs.h tir_driver_prefs.c tir_driver_prefs.h 
    wc_driver_prefs.c wc_driver_prefs.h ipc_utils.c ipc_utils.h 
    com_proc.c com_proc.h wii_com.c wii_com.h
    joy_driver_prefs.c joy_driver_prefs.h ps3_prefs.c ps3_prefs.h
)
target_include_directories(ltr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ..)
target_link_libraries(ltr PRIVATE ${LTR_LIBM} ${LTR_LIBPTHREAD} ${LTR_LIBDL})

add_library(linuxtrack SHARED
    ltlib.c linuxtrack.h utils.c utils.h ipc_utils.c
)
set_target_properties(linuxtrack PROPERTIES 
    SOVERSION 0
    VERSION 0
)
target_include_directories(linuxtrack PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ..)
target_link_libraries(linuxtrack PRIVATE ltr)

# liblinuxtrack32 (32-bit Public API for Wine compatibility)
if(HAS_LINUX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    # We compile it with -m32.
    add_library(linuxtrack32 SHARED
        ltlib.c linuxtrack.h utils.c utils.h ipc_utils.c
    )
    set_target_properties(linuxtrack32 PROPERTIES 
        COMPILE_FLAGS "-m32"
        LINK_FLAGS "-m32"
        OUTPUT_NAME "linuxtrack32"
        SOVERSION 0
        VERSION 0
    )
    target_compile_definitions(linuxtrack32 PRIVATE LTR_32BIT)
    target_link_libraries(linuxtrack32 PRIVATE ${LTR_LIBM})
    # Install to lib/linuxtrack - prioritized search path for loaders
    install(TARGETS linuxtrack32 DESTINATION lib/linuxtrack)
endif()

# --- Drivers (built as modules/plugins) ---
set(DRIVER_LDFLAGS "-Wl,-rpath,$ORIGIN/../lib")

# libwc (Webcam Driver)
if(HAS_V4L2 AND LTR_LIBV4L2)
    add_library(wc MODULE webcam_driver.c webcam_driver.h runloop.c runloop.h)
    target_link_libraries(wc PRIVATE ltr ${LTR_LIBV4L2})
    set_target_properties(wc PROPERTIES PREFIX "lib" LINK_FLAGS ${DRIVER_LDFLAGS})
endif()

# libtir (TrackIR Driver)
if(LIBUSB10_FOUND AND ZLIB_FOUND)
    add_library(tir MODULE 
        tir_hw.c tir_hw.h sn4_com.h tir_img.c tir_img.h 
        usb_ifc.h tir_driver.h tir_driver.c runloop.c runloop.h
    )
    target_link_libraries(tir PRIVATE ltr ZLIB::ZLIB)
    set_target_properties(tir PROPERTIES PREFIX "lib" LINK_FLAGS ${DRIVER_LDFLAGS})

    add_library(ltusb1 MODULE libusb_ifc.c usb_ifc.h)
    target_include_directories(ltusb1 PRIVATE ${LIBUSB10_INCLUDE_DIRS})
    target_link_libraries(ltusb1 PRIVATE ltr ${LIBUSB10_LIBRARIES})
    set_target_properties(ltusb1 PROPERTIES PREFIX "lib")
endif()

# libft (Facetracker Plugin)
if(OPENCV_FOUND AND HAS_V4L2 AND LTR_LIBV4L2)
    add_library(ft MODULE webcam_driver.c webcam_driver.h runloop.c runloop.h facetrack.cpp facetrack.h)
    target_compile_definitions(ft PRIVATE OPENCV)
    target_include_directories(ft PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OPENCV_INCLUDE_DIRS})
    target_link_libraries(ft PRIVATE ltr ${LTR_LIBV4L2} ${LTR_LIBPTHREAD} ${OPENCV_LIBRARIES})
    set_target_properties(ft PROPERTIES PREFIX "lib" LINK_FLAGS ${DRIVER_LDFLAGS})
endif()

# libjoy (Joystick Driver)
add_library(joy MODULE runloop.c runloop.h joy.c)
target_include_directories(joy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(joy PRIVATE ltr)
set_target_properties(joy PROPERTIES PREFIX "lib" LINK_FLAGS ${DRIVER_LDFLAGS})

# --- X-Plane Plugins ---
if(EXISTS "/usr/include/xplane_sdk/XPLM")
    set(XPL_INCLUDE_DIRS "/usr/include/xplane_sdk/XPLM" "/usr/include/xplane_sdk/Widgets")
    
    # 64-bit X-Plane Plugin
    add_library(xlinuxtrack9 SHARED xlinuxtrack9.c linuxtrack.c)
    target_include_directories(xlinuxtrack9 PRIVATE ${XPL_INCLUDE_DIRS} ..)
    target_compile_definitions(xlinuxtrack9 PRIVATE XPLM200 LINUX)
    target_link_libraries(xlinuxtrack9 PRIVATE ${LTR_LIBM} ${LTR_LIBDL})
    set_target_properties(xlinuxtrack9 PROPERTIES 
        PREFIX ""
        OUTPUT_NAME "xlinuxtrack9"
    )

    # 32-bit X-Plane Plugin
    if(HAS_LINUX AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        add_library(xlinuxtrack9_32 SHARED xlinuxtrack9.c linuxtrack.c)
        target_include_directories(xlinuxtrack9_32 PRIVATE ${XPL_INCLUDE_DIRS} ..)
        target_compile_definitions(xlinuxtrack9_32 PRIVATE XPLM200 LINUX)
        set_target_properties(xlinuxtrack9_32 PROPERTIES 
            COMPILE_FLAGS "-m32"
            LINK_FLAGS "-m32"
            PREFIX ""
            OUTPUT_NAME "xlinuxtrack9_32"
        )
        target_link_libraries(xlinuxtrack9_32 PRIVATE ${LTR_LIBM} ${LTR_LIBDL})
    endif()
endif()

# --- Executables ---

# ltr_server1
add_executable(ltr_server1 
    ltr_srv_comm.c ltr_srv_comm.h ltr_srv_slave.c ltr_srv_slave.h 
    ltr_srv_master.cpp ltr_srv_master.h ltr_server1.c ipc_utils.c ipc_utils.h
)
target_include_directories(ltr_server1 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ..)
target_link_libraries(ltr_server1 ltr ${LTR_LIBPTHREAD})

# ltr_recenter
add_executable(ltr_recenter ltr_recenter.c ltr_srv_comm.c ipc_utils.c)
target_include_directories(ltr_recenter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ..)
target_link_libraries(ltr_recenter linuxtrack ltr ${LTR_LIBDL})

# ltr_pipe
add_executable(ltr_pipe ltr_pipe.c linuxtrack.c utils.c)
target_compile_definitions(ltr_pipe PRIVATE _GNU_SOURCE LINUX)
target_include_directories(ltr_pipe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ..)
target_link_libraries(ltr_pipe ${LTR_LIBDL})

# ltr_extractor
add_executable(ltr_extractor hashing.c digest.c game_data.c utils.c extract.c)
target_include_directories(ltr_extractor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} qt_gui ..)
target_link_libraries(ltr_extractor ${MXML_LIBRARIES} ${LTR_LIBPTHREAD})

# osc_server
if(LIBLO_FOUND)
    add_executable(osc_server osc_server.c linuxtrack.c)
    target_include_directories(osc_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ..)
    target_link_libraries(osc_server ${LIBLO_LIBRARIES} ${LTR_LIBPTHREAD} ${LTR_LIBDL})
endif()

# ltr_udp (Bridge for X4: Foundations / OpenTrack games)
add_executable(ltr_udp ltr_udp.c linuxtrack.c)
target_include_directories(ltr_udp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ..)
target_link_libraries(ltr_udp ${LTR_LIBPTHREAD} ${LTR_LIBDL})

# ltr_hotkeyd (Global hotkey daemon for native games - X11 only)
if(X11_FOUND)
    add_executable(ltr_hotkeyd ltr_hotkeyd.c linuxtrack.c)
    target_include_directories(ltr_hotkeyd PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ..)
    target_link_libraries(ltr_hotkeyd ${X11_LIBRARIES} ${LTR_LIBPTHREAD} ${LTR_LIBDL})
endif()

# LAL (Licensed Asset Loader)
add_subdirectory(lal)

# Native Hotkey GUI
add_subdirectory(ltr_hotkey_gui)

# --- Installation ---

# 1. Libraries (Core)
install(TARGETS ltr linuxtrack DESTINATION lib)

# 2. Drivers (Plugins)
foreach(DRV wc tir joy ft ltusb1 xlinuxtrack9 xlinuxtrack9_32)
    if(TARGET ${DRV})
        install(TARGETS ${DRV} DESTINATION lib/linuxtrack)
    endif()
endforeach()

# 3. Executables (Binaries)
set(BIN_TARGETS ltr_server1 ltr_recenter ltr_pipe ltr_extractor ltr_udp)
if(TARGET osc_server)
    list(APPEND BIN_TARGETS osc_server)
endif()
if(TARGET ltr_hotkeyd)
    list(APPEND BIN_TARGETS ltr_hotkeyd)
endif()
install(TARGETS ${BIN_TARGETS} DESTINATION bin)

# 4. Configuration and Data
install(FILES linuxtrack1.conf linuxtrack.c linuxtrack_hello_world.c linuxtrack_hello_world_adv.c 
        DESTINATION share/linuxtrack)
install(FILES 99-TIR.rules 99-Mickey.rules DESTINATION share/linuxtrack)
if(HAS_OPENCV)
    install(FILES haarcascade_frontalface_alt2.xml DESTINATION share/linuxtrack)
endif()

# 5. Headers
install(FILES linuxtrack.h DESTINATION include)

# 6. Man Pages
set(MAN_PAGES 
    ltr_gui.1 ltr_server1.1 mickey.1 
    ltr_extractor.1 ltr_pipe.1
)
install(FILES ${MAN_PAGES} DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
