cmake_minimum_required(VERSION 3.20)
project(bareminimalrepro)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_executable(bareminimalrepro main.cpp)
target_link_directories(bareminimalrepro PRIVATE ${CMAKE_SOURCE_DIR}/NsightGraphicsSDK/0.8.0/lib/x64)
target_link_libraries(bareminimalrepro PRIVATE Vulkan::Vulkan NGFX_Injection.lib)
target_include_directories(bareminimalrepro PRIVATE ${CMAKE_SOURCE_DIR}/NsightGraphicsSDK/0.8.0/include)
target_link_options(bareminimalrepro PRIVATE "/DELAYLOAD:vulkan-1.dll")
add_custom_command(TARGET bareminimalrepro POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                   "${CMAKE_SOURCE_DIR}/NsightGraphicsSDK/0.8.0/lib/x64/NGFX_Injection.dll"
                   $<TARGET_FILE_DIR:bareminimalrepro>)