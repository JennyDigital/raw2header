cmake_minimum_required( VERSION 3.22 )
project( raw2header )
set( SOURCES raw2header.c )

add_executable( ${PROJECT_NAME} ${SOURCES} ${HEADERS} )
install( TARGETS ${PROJECT_NAME} DESTINATION /usr/local/bin/ )
