cmake_minimum_required(VERSION 3.15)

project(RayTracer)

set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/External)

# Dependencies
# ------------

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/Include/*.h
)

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin )

add_executable(${PROJECT_NAME} ${SOURCES})


#target_link_libraries(${PROJECT_NAME} PRIVATE)
