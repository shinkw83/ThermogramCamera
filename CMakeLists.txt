cmake_minimum_required(VERSION 3.8)

IF (POLICY CMP0077)
	cmake_policy(SET CMP0077 OLD)
ENDIF (POLICY CMP0077)

project(cam_agent C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wno-psabi")

find_package(Threads REQUIRED)
find_package(cppzmq REQUIRED)

include_directories(
	common/include
	source/App
)

link_directories(
	common/lib
)

file(GLOB _cpp_files "source/App/*.cpp")

add_executable(cam_agent
	${_cpp_files}
)

target_link_libraries(cam_agent
	cppzmq
	xthread
	xpacket
	jsoncpp
	common
	boost_system
	log4cplus
	raspicam
	raspicam_cv
	turbojpeg
	opencv_core
	opencv_highgui
	opencv_imgproc
	opencv_imgcodecs
	leptonSDK
	paho-mqtt3as
	paho-mqttpp3
)

set_target_properties(cam_agent PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/bin)
