cmake_minimum_required(VERSION 3.16)
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/main/offduty_version.h" od_version_line REGEX "^#define OD_APP_VERSION ")
string(REGEX REPLACE "^#define OD_APP_VERSION \"([^\"]+)\"$" "\\1" PROJECT_VER "${od_version_line}")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(FoloToy-AI-Passport)
