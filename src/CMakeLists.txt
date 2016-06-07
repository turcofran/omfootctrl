cmake_minimum_required (VERSION 2.6)
project (cvOM)

set (CVOM_BIN_NAME "cvOM")

# The version number.
set (CVOM_VERSION_MAJOR 1)
set (CVOM_VERSION_MINOR 0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -llo -lpthread -ljack -lv4l2 -g") 

# configure a header file to pass some of the CMake settings to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/main_config.hpp.in"
  "${PROJECT_BINARY_DIR}/main_config.hpp"
)

# find boost packageBOOST_INCLUDEDIR
set (Boost_NO_SYSTEM_PATHS OFF) 
set (Boost_USE_MULTITHREADED OFF)
set (Boost_USE_STATIC_LIBS OFF)
set (BOOST_ROOT "/usr/lib/") #/usr/include/boost/
find_package( Boost REQUIRED COMPONENTS program_options system) 
if (Boost_FOUND)
  include_directories (SYSTEM ${Boost_INCLUDE_DIR})
endif ()
# fins Jack package
#~ find_package(lo REQUIRED )
# find OpenCV package
find_package( OpenCV REQUIRED )
# add the binary tree to the search path for include files so that we will find main_config.h
include_directories("${PROJECT_BINARY_DIR}")

# should we use wins
option (SHOW_WIN 
        "Use windows for debug" ON) 
option (DEBUG 
        "Use debug" OFF)

add_executable( ${CVOM_BIN_NAME} main_config.hpp ominputdev.hpp footcontroller.cpp footcontroller.hpp
                  ocv.cpp ocv.hpp cmdmap.hpp osc.cpp osc.hpp midi.cpp midi.hpp main.cpp )
target_link_libraries( ${CVOM_BIN_NAME} ${OpenCV_LIBS} ) 
target_link_libraries( ${CVOM_BIN_NAME} ${Boost_LIBRARIES} )
 


#~ # conditional math lib use
#~ if (USE_MYMATH)
  #~ include_directories ("${PROJECT_SOURCE_DIR}/MathFunctions")
  #~ add_subdirectory (MathFunctions)
  #~ set (EXTRA_LIBS ${EXTRA_LIBS} MathFunctions)
#~ endif (USE_MYMATH)
 #~ 
#~ # add the executable
#~ add_executable (Tutorial tutorial.cxx)
#~ target_link_libraries (Tutorial  ${EXTRA_LIBS})
#~ 
#~ # add the install targets
#~ install (TARGETS Tutorial DESTINATION bin)
#~ install (FILES "${PROJECT_BINARY_DIR}/TutorialConfig.h"        
         #~ DESTINATION include)
#~ 
#~ #######################################################
#~ # Add testing running
#~ include(CTest)
#~ 
#~ # does the application run
#~ add_test (TutorialRuns Tutorial 25)
 #~ 
 #~ #define a macro to simplify adding tests, then use it
#~ macro (do_test arg result)
  #~ add_test (TutorialComp${arg} Tutorial ${arg})
  #~ set_tests_properties (TutorialComp${arg}
    #~ PROPERTIES PASS_REGULAR_EXPRESSION ${result})
#~ endmacro (do_test)
 #~ 
#~ # do a bunch of result based tests
#~ do_test (25 "25 is 5")
#~ do_test (-25 "-25 is -nan")
#~ do_test (0.0001 "0.0001 is 0.01")
   #~ 
#~ # does the usage message work?
#~ add_test (TutorialUsage Tutorial)
#~ set_tests_properties (TutorialUsage
  #~ PROPERTIES 
  #~ PASS_REGULAR_EXPRESSION "Usage:.*number")
#~ #######################################################
