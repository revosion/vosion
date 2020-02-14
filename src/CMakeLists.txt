cmake_minimum_required(VERSION 3.5)
project(vosion)
add_definitions(-std=c++11)
###############################################################
# Version info
###############################################################
set(MAJOR_VERSION 0)
set(MINOR_VERSION 1)
set(PATCH_VERSION 0)
set(PROJECT_VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

###############################################################
# External dependencies
###############################################################
find_package(OpenSSL REQUIRED)
#find_package(PahoMqttCpp REQUIRED)
find_library(json11 NAMES libjson11.a REQUIRED)

find_library(paho-mqttpp3 NAMES libpaho-mqttpp3.so REQUIRED)
find_library(paho-mqtt3a NAMES libpaho-mqtt3a.so REQUIRED)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()
###############################################################
# Create libraries
###############################################################
set(SOURCES
    ./main.cpp
    ./vfd.cpp
    ./socketcan_sender.cpp
    ./adc.cpp
    ./iio_utils.c
    ./pid.cpp
    ./adc.cpp
    ./sensor.cpp
    ./cloud_adapter.cpp
    ./socketcan_raw_receiver.cpp
)
#-------------
# Build static library
#-------------
#add_library(${PROJECT_NAME}_static STATIC ${SOURCES})
#target_link_libraries(${PROJECT_NAME}_static)

#-------------
# Build shared library
#-------------
#add_library(${PROJECT_NAME} ${SOURCES})
#target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES} ${CBOR_LIBRARIES})
#set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${MAJOR_VERSION})

#-------------
# Build executable
#-------------
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(
    ${PROJECT_SOURCE_DIR}
    ../include
    /usr/local/include/mqtt
)
target_link_libraries(${PROJECT_NAME} ${json11} ${paho-mqtt3a} ${paho-mqttpp3} pthread)

###############################################################
# Submodules
###############################################################
#add_subdirectory(example)

###############################################################
# Testbench
###############################################################
#add_subdirectory(test)

###############################################################
# Install
###############################################################
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
