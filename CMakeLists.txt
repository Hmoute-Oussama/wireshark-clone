cmake_minimum_required(VERSION 3.16)
project(WiresharkClone VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_subdirectory(third_party)

add_executable(WiresharkClone
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/PacketCapture.cpp
    src/PacketCapture.h
    src/PacketData.h
    src/services/FilterEngine.cpp
    src/services/FilterEngine.h
    src/services/capture_service.cpp
    src/services/capture_service.hpp
    src/services/filter_service.cpp
    src/services/filter_service.hpp
    src/services/database_service.cpp
    src/services/database_service.hpp
)

target_link_libraries(WiresharkClone
    PRIVATE
        Qt6::Widgets
        pcap
        sqlite3
        spdlog
        fmt
        json
)
