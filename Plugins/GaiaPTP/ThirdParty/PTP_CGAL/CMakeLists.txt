cmake_minimum_required(VERSION 3.20)
project(ptp_cgal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CGAL CONFIG REQUIRED)

add_library(ptp_cgal STATIC
    src/ptp_cgal.cpp
)
target_include_directories(ptp_cgal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(ptp_cgal PUBLIC CGAL::CGAL)
