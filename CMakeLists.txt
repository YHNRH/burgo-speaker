cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS
    components
    )

set(ADF_PATH "/home/lasas/Desktop/esp-adf")
list(APPEND EXTRA_COMPONENT_DIRS "${ADF_PATH}/components")

include($ENV{ADF_PATH}/CMakeLists.txt)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wake_word_detection)
