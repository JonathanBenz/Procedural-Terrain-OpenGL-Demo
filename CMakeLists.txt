cmake_minimum_required(VERSION 3.10)

project(BenzJonathan_AcerolaDirtjam_Submission)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.c
)

file(GLOB_RECURSE HEADERS
    includes/*.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
    includes
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

set(GLFW_LIB_PATH "${CMAKE_SOURCE_DIR}/lib/glfw3.lib")

IF(WIN32)
    target_link_libraries(${PROJECT_NAME} ${GLFW_LIB_PATH} opengl32)
ELSEIF(APPLE)
    find_library(OpenGL_LIBRARY OpenGL)
    target_link_libraries(${PROJECT_NAME} ${OpenGL_LIBRARY})
ELSEIF(UNIX)
    target_link_libraries(${PROJECT_NAME} glfw GL X11 pthread dl)
ENDIF()


install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY shaders DESTINATION bin)
install(DIRECTORY textures DESTINATION bin)
install(FILES README.md DESTINATION bin)