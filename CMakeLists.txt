cmake_minimum_required(VERSION 2.8.3)
project(iiwa_hw_fri)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

find_package(Boost REQUIRED)

set(FRI_Client_SDK_Cpp_zip_PATH "${CMAKE_SOURCE_DIR}/external/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(${CMAKE_SOURCE_DIR}/cmake/FRI_Client_SDK_Cpp.cmake)
set(FRI_LIBS ${KukaFRIClient_LIBRARY} ${Nanopb_LIBRARIES} ${KukaFRIUDP_LIBRARY})

find_package(catkin REQUIRED COMPONENTS
  control_toolbox
  controller_interface
  controller_manager
  hardware_interface
  realtime_tools
  joint_limits_interface
  tf
  urdf
  cmake_modules
  iiwa_msgs
  iiwa_ros
)

catkin_package(
	INCLUDE_DIRS include
	LIBRARIES	 ${PROJECT_NAME}
	CATKIN_DEPENDS  controller_interface
			hardware_interface
			control_toolbox
)

## Specify additional locations of header files
include_directories(include
  ${Boost_INCLUDE_DIR}
  ${catkin_INCLUDE_DIRS}
)

## Declare a cpp library
add_library(${PROJECT_NAME}
  src/iiwa_hw.cpp
)

## Declare a cpp executable
add_executable(${PROJECT_NAME}-bin
    src/main.cpp
)

## Add dependence to the iiwa_msg module for the executable
add_dependencies(${PROJECT_NAME}-bin
				iiwa_msgs_generate_messages_cpp)

## Add dependence to the iiwa_msg module for the library
add_dependencies(${PROJECT_NAME}
				iiwa_msgs_generate_messages_cpp)

## Specify libraries to link a library or executable target against
target_link_libraries(${PROJECT_NAME}-bin
    ${PROJECT_NAME}
    ${catkin_LIBRARIES}
    ${FRI_LIBS}
    ${Boost_LIBRARIES}
)