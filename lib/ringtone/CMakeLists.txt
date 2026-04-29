zephyr_library_sources_ifdef(CONFIG_GZM_RINGTONE
    src/ringtone.c
    src/rtttl_player.c
)

if(CONFIG_GZM_RINGTONE AND CONFIG_SHELL)
    zephyr_library_sources(src/ringtone_shell.c)
endif()

if(CONFIG_GZM_RINGTONE)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
    zephyr_library_include_directories(src)
endif()
