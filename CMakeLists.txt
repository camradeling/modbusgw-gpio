cmake_minimum_required(VERSION 3.6)
project(modbusgw-gpio)
option(DEBUG_BUILD "Option description" OFF)
set(CMAKE_CXX_STANDARD 17)
set(BIN_DIR ${CMAKE_BINARY_DIR}/Binaries)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_DIR})
set(PROJECT_NAME modbusgw-gpio)
link_directories(${BIN_DIR})
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
if(DEBUG_BUILD)
    message(STATUS "DEBUG_BUILD = ON")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fno-omit-frame-pointer -fasynchronous-unwind-tables")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fno-omit-frame-pointer -fasynchronous-unwind-tables")
else()
    message(STATUS "DEBUG_BUILD = OFF")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Os -s")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Os -s")
endif()

set(Sources
    main.cpp
    submodules/modbuslib/modbus_client.cpp
    submodules/modbuslib/modbus_crc_tables.c
    modbus_gpio_gateway.cpp
    modbus_slave_adapter.cpp
    gpio_backend.cpp
    ini_parser.cpp
    )

message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
include_directories(submodules/channellib)
include_directories(submodules/threadlib)
include_directories(submodules/threadlib/messagequeue)
include_directories(submodules/modbuslib)
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

add_subdirectory(submodules/channellib)
add_subdirectory(submodules/threadlib)
add_custom_target(static_libs)
add_dependencies(static_libs ProgramThread_static ChannelLib_static)

add_executable(${PROJECT_NAME}
        ${Sources}
        )

target_link_libraries(${PROJECT_NAME} libChannelLib_static.a)
target_link_libraries(${PROJECT_NAME} libProgramThread_static.a)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} mxml)

add_dependencies(${PROJECT_NAME} static_libs)

install(TARGETS ${PROJECT_NAME}
        DESTINATION /usr/bin)
install(FILES install/config.ini
        DESTINATION /etc/modbusgw-gpio)
install(FILES install/modbusgw-gpio.service
        DESTINATION /etc/systemd/system)
