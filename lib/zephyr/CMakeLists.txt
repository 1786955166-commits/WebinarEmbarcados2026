# SPDX-License-Identifier: Apache-2.0

# Sub-libraries (conditionally compiled via their own Kconfig guards)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/common   common)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/rtc_mgr  rtc_mgr)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/wifi_mgr wifi_mgr)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/fs_mgr   fs_mgr)
add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/ringtone ringtone)
