cmake_minimum_required(VERSION 3.16)
project(WiresharkClone VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6
find_package(Qt6 COMPONENTS Widgets REQUIRED)

set(MYSQL_CONNECTOR_C_ROOT "C:/Program Files/MySQL/MySQL Connector C 6.1")
find_path(MYSQL_INCLUDE_DIR mysql.h
    PATHS "${MYSQL_CONNECTOR_C_ROOT}/include"
    REQUIRED)
find_library(MYSQL_LIBRARY libmysql
    PATHS "${MYSQL_CONNECTOR_C_ROOT}/lib"
    REQUIRED)
set(MYSQL_RUNTIME_DLL "${MYSQL_CONNECTOR_C_ROOT}/lib/libmysql.dll")

add_subdirectory(third_party)

# Executable targets
add_executable(WiresharkClone
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/PacketCapture.cpp
    src/PacketCapture.h
    src/PacketData.h
    src/controller/packet_controller.cpp
    src/controller/packet_controller.hpp
    src/model/database/db_manager.cpp
    src/model/database/db_manager.hpp
    src/model/database/models/packet_entity.hpp
    src/model/database/repositories/packet_repository.cpp
    src/model/database/repositories/packet_repository.hpp
    src/services/database_service.cpp
    src/services/database_service.hpp
)

target_link_libraries(WiresharkClone
    PRIVATE
        Qt6::Widgets
        ${MYSQL_LIBRARY}
        pcap
        sqlite3
        spdlog
        fmt
        json
)

target_include_directories(WiresharkClone
    PRIVATE
        ${MYSQL_INCLUDE_DIR}
)

add_custom_command(TARGET WiresharkClone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MYSQL_RUNTIME_DLL}"
        "$<TARGET_FILE_DIR:WiresharkClone>/libmysql.dll")

