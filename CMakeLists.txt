cmake_minimum_required(VERSION 3.8.0)
project(hl-bsp-viewer)

set(PRODUCT_NAME "BSP Viewer")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# main

if(BUILD_DEVELOPER)
	add_definitions(-DBUILD_DEVELOPER)
endif()

add_definitions(-DPROJECT_NAME="${PROJECT_NAME}")
add_definitions(-DPRODUCT_NAME="${PRODUCT_NAME}")

# main

file(GLOB MAIN_SRC
	src/*.cpp
	src/*.h
)

source_group("all" FILES ${MAIN_SRC})

file(GLOB ALL_SRC
	${MAIN_SRC}
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
add_definitions(-DNOMINMAX)

add_executable(${PROJECT_NAME} 
	${ALL_SRC}
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

target_include_directories(${PROJECT_NAME} PUBLIC src)

# sky

add_subdirectory(sky)
target_link_libraries(${PROJECT_NAME} sky)
copy_required_libs(${PROJECT_NAME})

# hl

add_subdirectory(hl)
target_link_libraries(${PROJECT_NAME} hl)