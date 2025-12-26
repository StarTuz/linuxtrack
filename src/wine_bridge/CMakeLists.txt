# src/wine_bridge/CMakeLists.txt

# Common flags for winegcc
set(WINE_CPPFLAGS -DHAVE_CONFIG_H -I${CMAKE_SOURCE_DIR} -I${CMAKE_BINARY_DIR} -I${CMAKE_SOURCE_DIR}/src)
set(WINE_COMMON_LDFLAGS -Wl,--no-warn-search-mismatch)

# Ensure bitness is handled
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(HAS_WINE64 ON)
else()
    set(HAS_WINE64 OFF)
endif()

# Helper to build a wine dll/exe
function(add_wine_binary TARGET_NAME)
    set(options SHARED)
    set(oneValueArgs OUTPUT BITNESS SPEC TYPE)
    set(multiValueArgs SOURCES LIBS)
    cmake_parse_arguments(WINE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(WINE_ARCH_FLAG "")
    if(WINE_BITNESS EQUAL 32)
        set(WINE_ARCH_FLAG "-m32")
    endif()

    set(WINE_COMPILER ${WINEGCC})
    if(WINE_TYPE STREQUAL "CXX")
        set(WINE_COMPILER ${WINEGXX})
    endif()

    set(SPEC_FILE_ABS "")
    if(WINE_SPEC)
        set(SPEC_FILE_ABS "${CMAKE_CURRENT_SOURCE_DIR}/${WINE_SPEC}")
    endif()

    set(OBJ_FILES "")
    foreach(SRC ${WINE_SOURCES})
        get_filename_component(SRC_EXT ${SRC} EXT)
        get_filename_component(SRC_NAME ${SRC} NAME_WLE)
        # Avoid collisions for same source name in different bitness or types
        set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_${SRC_NAME}_${WINE_BITNESS}.o")
        
        # Determine source path
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
            set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SRC}")
        elseif(EXISTS "${CMAKE_SOURCE_DIR}/src/${SRC}")
            set(SRC_PATH "${CMAKE_SOURCE_DIR}/src/${SRC}")
        else()
             message(FATAL_ERROR "Source ${SRC} not found for ${TARGET_NAME}")
        endif()

        if(SRC_EXT STREQUAL ".rc")
            add_custom_command(
                OUTPUT ${OBJ_FILE}
                COMMAND ${WRC} "-o" ${OBJ_FILE} "-I" "${CMAKE_CURRENT_SOURCE_DIR}" ${SRC_PATH}
                DEPENDS ${SRC_PATH}
                COMMENT "Compiling Wine resource ${SRC} for ${TARGET_NAME}"
            )
        else()
            set(COMP ${WINEGCC})
            if(SRC_EXT MATCHES "\\.(cpp|cxx|C)$")
                set(COMP ${WINEGXX})
            endif()
            
            set(EXTRA_DEF "")
            if(WINE_BITNESS EQUAL 64)
                set(EXTRA_DEF "-DFOR_WIN64=1")
            endif()

            add_custom_command(
                OUTPUT ${OBJ_FILE}
                COMMAND ${COMP} -c ${WINE_ARCH_FLAG} ${WINE_CPPFLAGS} ${EXTRA_DEF} -o ${OBJ_FILE} ${SRC_PATH}
                DEPENDS ${SRC_PATH} ${CMAKE_BINARY_DIR}/config.h
                COMMENT "Compiling Wine object ${SRC} (${WINE_BITNESS}-bit) for ${TARGET_NAME}"
            )
        endif()
        list(APPEND OBJ_FILES ${OBJ_FILE})
    endforeach()

    set(SHARED_FLAG "")
    if(WINE_SHARED)
        set(SHARED_FLAG "-shared")
    endif()

    set(RPATH_FLAGS "-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib/linuxtrack:-Wl,-rpath,${CMAKE_INSTALL_PREFIX}/lib")

    add_custom_command(
        OUTPUT ${WINE_OUTPUT}
        COMMAND ${WINE_COMPILER} ${WINE_ARCH_FLAG} ${SHARED_FLAG} -o ${WINE_OUTPUT} ${OBJ_FILES} ${SPEC_FILE_ABS} ${WINE_COMMON_LDFLAGS} ${RPATH_FLAGS} ${WINE_LIBS}
        DEPENDS ${OBJ_FILES} ${SPEC_FILE_ABS}
        COMMENT "Linking Wine binary ${WINE_OUTPUT}"
    )
    add_custom_target(${TARGET_NAME} ALL DEPENDS ${WINE_OUTPUT})
    
    # Installation
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${WINE_OUTPUT} DESTINATION lib/linuxtrack)
endfunction()

# 1. NPClient
add_wine_binary(NPClient32
    OUTPUT NPClient.dll.so
    BITNESS 32
    SHARED
    SPEC client/NPClient.spec
    SOURCES client/NPClient_main.c linuxtrack.c client/rest.c
    LIBS -ldl
)

if(HAS_WINE64)
    add_wine_binary(NPClient64
        OUTPUT NPClient64.dll.so
        BITNESS 64
        SHARED
        SPEC client/NPClient.spec
        SOURCES client/NPClient_main.c linuxtrack.c client/rest.c
        LIBS -ldl
    )
endif()

# 2. FreeTrackClient
add_wine_binary(FreeTrackClient32
    OUTPUT FreeTrackClient.dll.so
    BITNESS 32
    SHARED
    SPEC ft_client/FreeTrackClient.spec
    SOURCES ft_client/FreeTrackClient_main.c linuxtrack.c
    LIBS -ldl
)

# 3. Controller
add_wine_binary(Controller
    TYPE CXX
    OUTPUT Controller.exe.so
    BITNESS 32
    SOURCES controller/main.cpp controller/resources.rc controller/dinput.cpp linuxtrack.c
    LIBS -lshell32 -ldxguid -ldinput -ldinput8 -luuid -ldl
)

# 4. Tester
add_wine_binary(Tester32
    TYPE CXX
    OUTPUT Tester.exe.so
    BITNESS 32
    SOURCES tester/main.cpp tester/npview.rc tester/rest.c tester/npifc.c
    LIBS -ldl
)

if(HAS_WINE64)
    add_wine_binary(Tester64
        TYPE CXX
        OUTPUT Tester64.exe.so
        BITNESS 64
        SOURCES tester/main.cpp tester/npview.rc tester/rest.c tester/npifc.c
        LIBS -ldl
    )
endif()

# 5. TrackIR (views stub)
add_wine_binary(TrackIR_stub
    TYPE CXX
    OUTPUT TrackIR.exe.so
    BITNESS 32
    SOURCES views/main.cpp views/rest.c
)

# 6. FTC (FreeTrack Tester)
add_wine_binary(FTC
    TYPE CXX
    OUTPUT ftc.exe.so
    BITNESS 32
    SOURCES ft_tester/main.cpp ft_tester/fttester.rc
)

# 7. CheckData
add_wine_binary(CheckData
    OUTPUT check_data.exe.so
    BITNESS 32
    SOURCES client/check_data.c client/rest.c
)

# Documentation and icons
install(FILES README.wine DESTINATION share/doc/linuxtrack)
install(FILES linuxtrack.ico linuxtrack1.ico DESTINATION share/doc/linuxtrack)
