cmake_minimum_required(VERSION 2.8.12)

project("libhlc")

#
# Source files and library
#
file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
file(GLOB_RECURSE LIB_HEADERS src/*.h)
files_to_dirs(LIB_HEADERS LIB_INCLUDE_DIRS)

#Add the version file to the sources
list(APPEND LIB_SOURCES ${VTR_VERSION_FILE_OUT})

#Create the library
add_library(libhlc STATIC
             ${LIB_HEADERS}
             ${LIB_SOURCES})
target_include_directories(libhlc PUBLIC ${LIB_INCLUDE_DIRS})
set_target_properties(libhlc PROPERTIES PREFIX "") #Avoid extra 'lib' prefix

#Ensure version is always up to date by requiring version to be run first
add_dependencies(libhlc version)

#Specify link-time dependancies

install(TARGETS libhlc DESTINATION bin)

#
# Unit Tests
#
file(GLOB_RECURSE TEST_SOURCES test/*.cpp)
add_executable(test_hlc ${TEST_SOURCES})
target_link_libraries(test_hlc libhlc libcatch)

add_test(NAME test_hlc COMMAND test_hlc --use-colour=yes)
