cmake_minimum_required( VERSION 3.22 )
project( raw2header )

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )
	message( FATAL_ERROR "In-source builds are not supported. Use: cmake --preset dev" )
endif()

include( GNUInstallDirs )

if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
		OR CMAKE_INSTALL_PREFIX STREQUAL "/usr/local" )
	set( CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install path prefix" FORCE )
endif()

set( SOURCES raw2header.c adpcm.c )
set( ADPCM_SOURCES adpcm.c )

add_executable( ${PROJECT_NAME} ${SOURCES} ${HEADERS} )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

# Enable testing
enable_testing()

# Add test executable
add_executable( test_adpcm test_adpcm.c ${ADPCM_SOURCES} )
target_link_libraries( test_adpcm m )
add_test( NAME ADPCM COMMAND test_adpcm )
