CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED TRUE)
SET(CMAKE_BUILD_TYPE Debug)

if(WIN32)
	# 0x061 = _WIN32_WINNT_WIN7, see sdkddver.h
	add_compile_definitions(_WIN32_WINNT=0x601)
endif()

project(ArduSerialStatsServer)

include_directories(
	"${PROJECT_SOURCE_DIR}/include/"
	"${PROJECT_SOURCE_DIR}/include/rs232/"
	"${PROJECT_SOURCE_DIR}/include/resources/"
	)

add_library(rs232 SHARED "${PROJECT_SOURCE_DIR}/include/rs232/rs232.c")
add_library(resources SHARED "${PROJECT_SOURCE_DIR}/include/resources/resources.cpp")

add_executable(server "server.cpp")
target_link_libraries(server rs232 resources)
