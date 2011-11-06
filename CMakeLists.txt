cmake_minimum_required(VERSION 2.8)

project( Gibbon )	
FILE( GLOB_RECURSE PROJ_SOURCES src/*.cpp )
FILE( GLOB_RECURSE PROJ_HEADERS src/*.h )
find_package( OpenCV REQUIRED )
#find_package( Boost REQUIRED )
find_package( Boost 1.42 COMPONENTS program_options regex system REQUIRED )
include_directories( "/usr/include/flycapture" ) 
include_directories( ${Gibbon_SOURCE_DIR}/TUIO_CPP/TUIO )
include_directories( ${BOOST_INCLUDE_DIR} )
#include_directories( ${flycapture2_INCLUDE_DIRS} )

link_directories( ${Gibbon_SOURCE_DIR}/TUIO_CPP/ )
link_directories( ${Boost_LIBRARY_DIRS} )

add_executable( Gibbon ${PROJ_SOURCES} )

#target_link_libraries( Gibbon ${} )
target_link_libraries( Gibbon ${OpenCV_LIBS} )
target_link_libraries( Gibbon ${Boost_LIBRARIES} )
target_link_libraries( Gibbon flycapture )
target_link_libraries( Gibbon TUIO )

