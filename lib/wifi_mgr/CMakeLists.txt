zephyr_library_sources_ifdef(CONFIG_GZM_WIFI_MGR src/wifi_mgr.c)
if(CONFIG_GZM_WIFI_MGR)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
