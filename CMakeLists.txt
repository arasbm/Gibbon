cmake_minimum_required(VERSION 2.8)
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS -Dx86_64)

project( Gibbon )	
FILE( GLOB_RECURSE PROJ_SOURCES src/*.cpp)
FILE( GLOB_RECURSE TUIO_SOURCES TUIO_CPP/*.cpp )
FILE( GLOB_RECURSE PROJ_HEADERS src/*.h )
find_package( OpenCV REQUIRED )
find_package( Boost 1.42 COMPONENTS program_options regex system REQUIRED )

include_directories( "/usr/include/flycapture" ) 
include_directories( "/usr/include/oscpack/ip" )
include_directories( "/usr/include/oscpack/osc" )
include_directories( ${Gibbon_SOURCE_DIR}/TUIO_CPP )
include_directories( ${BOOST_INCLUDE_DIR} )
include_directories( ${oscpack_INCLUDE_DIR} )

link_directories( ${Boost_LIBRARY_DIRS} )
#link_directories( ${Gibbon_SOURCE_DIR}/TUIO_32bit_Linux )
#link_directories( "/usr/lib" )

add_executable( Gibbon ${PROJ_SOURCES} ${TUIO_SOURCES} )
#add_executable( Gibbon ${PROJ_SOURCES} )

#Link libraries
target_link_libraries( Gibbon ${OpenCV_LIBS} )
target_link_libraries( Gibbon ${Boost_LIBRARIES} )
target_link_libraries( Gibbon flycapture )
target_link_libraries( Gibbon oscpack )
#target_link_libraries( Gibbon TUIO )


