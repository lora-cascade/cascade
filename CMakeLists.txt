cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS "components/arduino-esp32/" "components/arduino-LoRa")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(cascade)

