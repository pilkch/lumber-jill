# Set the minimum cmake version
cmake_minimum_required(VERSION 3.10)

SET(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

IF(UNIX)
SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
ELSE()
# Set the C++ standard
#SET(CMAKE_CXX_STANDARD 14)
#SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD 20)
ENDIF()

# Set the project name
PROJECT(lumber-jill)

SET(CMAKE_SOURCE_DIR ./../../library/)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

SET(PROJECT_DIRECTORY "./")

# Warnings https://caiorss.github.io/C-Cpp-Notes/compiler-flags-options.html
SET(WARNING_FLAGS "-Wall -W -Werror -Wextra -Wpedantic -Wconversion -Wcast-align -Winline -Wunused -Wshadow -Wold-style-cast -Wpointer-arith -Wcast-qual -Wno-missing-braces")

SET(BASIC_WARNING_FLAGS "-Wformat -Wformat-y2k -Winit-self -Wstack-protector -Wunknown-pragmas -Wundef -Wwrite-strings -Wno-unused-parameter -Wno-switch -Woverloaded-virtual -Wsuggest-override -Wmissing-include-dirs -Wuninitialized")

# Hardening https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc
SET(HARDENING_FLAGS "-Werror=format-security -fno-exceptions -fno-rtti -D_FORTIFY_SOURCE=2 -fpie -Wl,-pie -Wl,-z,defs -Wl,-z,now -Wl,-z,relro -fplugin=annobin -fstack-clash-protection -fstack-protector-all -fstack-protector-strong -fcf-protection")


IF(UNIX)
# Add definitions, compiler switches, etc.
ADD_DEFINITIONS("-std=c++20 ${WARNING_FLAGS} ${BASIC_WARNING_FLAGS} ${HARDENING_FLAGS} -O2")
ENDIF()

IF(UNIX)
  ADD_DEFINITIONS("-D__LINUX__")
ENDIF()

IF(WIN32)
  ADD_DEFINITIONS("-DUNICODE -D_UNICODE")
  ADD_DEFINITIONS("-DNOMINMAX")

  ADD_DEFINITIONS("/std:c++latest")
ENDIF()

INCLUDE_DIRECTORIES(include)


# Source files
SET(SOURCE_FILES_COMMON src/btrfs.cpp src/run_command.cpp src/settings.cpp src/smartctl.cpp src/utils.cpp)

SET(SOURCE_FILES src/main.cpp ${SOURCE_FILES_COMMON})

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCE_FILES})


IF(WIN32)
target_compile_features(lumber-jill PUBLIC cxx_std_20)
ENDIF()

# HACK: Pass the C++ version to the compiler
if(MSVC)
    target_compile_options(lumber-jill PUBLIC "/Zc:__cplusplus")
endif()


INCLUDE(FindPkgConfig)

SET(LIBRARIES
  Threads
)


IF(UNIX)
SET(STANDARD_LIBRARIES
  stdc++fs
  pthread
  json-c
  m
)
SET(XDG_LIBRARIES
  xdg-basedir
)
ENDIF()

# Some of the libraries have different names than their Find*.cmake name
SET(LIBRARIES_LINKED
  ${STANDARD_LIBRARIES}
  ${XDG_LIBRARIES}
)

FOREACH(LIBRARY_FILE ${LIBRARIES})
  Find_Package(${LIBRARY_FILE} REQUIRED)
ENDFOREACH(LIBRARY_FILE)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBRARIES_LINKED})


# Unit test
SET(SOURCE_FILES_UNITTEST ${SOURCE_FILES_COMMON} test/src/main.cpp test/src/parse_command_output_unittest.cpp test/src/run_command_unittest.cpp)

SET(LIBRARIES_LINKED_UNITTEST
  ${LIBRARIES_LINKED}
  gtest
)

ADD_EXECUTABLE(${PROJECT_NAME}-unittest ${SOURCE_FILES_UNITTEST})
TARGET_LINK_LIBRARIES(${PROJECT_NAME}-unittest ${LIBRARIES_LINKED_UNITTEST})

