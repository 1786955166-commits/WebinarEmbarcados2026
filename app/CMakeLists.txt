# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(WebinarEmbarcados2026)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Add all source files
file(GLOB APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c)
target_sources(app PRIVATE ${APP_SOURCES})

# Generate gzip-compressed web resources as C includes at build time
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(web_resource index.html app.js style.css)
    generate_inc_file_for_target(
        app
        www/${web_resource}
        ${gen_dir}/${web_resource}.gz.inc
        --gzip
    )
endforeach()

# HTTP server linker sections for web_service resources
zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section(NAME http_resource_desc_web_service
                      KVMA RAM_REGION GROUP RODATA_REGION)
