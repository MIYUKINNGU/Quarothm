cmake_minimum_required(VERSION 3.12)
project(OpenSiv3D_Linux_App CXX)

if (NOT CMAKE_CONFIGURATION_TYPES AND 
    NOT CMAKE_NO_BUILD_TYPE AND
    NOT CMAKE_BUILD_TYPE AND
    CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(STATUS "[!] Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

add_executable(Siv3DTest
  Main.cpp
  src/scenes/InitializeScene.cpp
  src/scenes/TitleScene.cpp
  src/scenes/SongSelectScene.cpp
  src/scenes/PlayScene.cpp
  src/scenes/ResultScene.cpp
  src/utilities/ResponsiveValue.cpp
  src/types/AbstractScene.cpp
  )

find_package(Siv3D)
target_link_libraries(Siv3DTest PUBLIC Siv3D::Siv3D)

target_link_directories(Siv3DTest PRIVATE
    /usr/lib/opencv4
)

target_compile_features(Siv3DTest PRIVATE cxx_std_20)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(Siv3DTest PRIVATE __DEBUG__=1)
else()
    target_compile_definitions(Siv3DTest PRIVATE __DEBUG__=0)
endif()

if(BUILD_TESTING)
enable_testing()
add_test(
  NAME Test
  COMMAND Siv3DTest
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endif()
