cmake_minimum_required(VERSION 3.16)

project(app VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core HttpServer)
qt_standard_project_setup()

qt_add_executable(app
    src/main.cpp
)

target_link_libraries(app PRIVATE
    Qt6::Core
    Qt6::HttpServer
)

install(TARGETS app RUNTIME DESTINATION bin)
