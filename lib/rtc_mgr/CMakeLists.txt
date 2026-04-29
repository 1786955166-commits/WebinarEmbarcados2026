zephyr_library_sources_ifdef(CONFIG_GZM_RTC_MGR src/rtc_mgr.c)

if(CONFIG_GZM_RTC_MGR)
    zephyr_include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
