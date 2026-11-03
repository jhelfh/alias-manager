cmake_minimum_required(VERSION 3.22)

project(AliasManager VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(alias-manager
    main.cpp
)

target_link_libraries(alias-manager PRIVATE Qt6::Widgets)