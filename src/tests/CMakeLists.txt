set(MY_PROJ_NAME dsn.tests)

# Source files under CURRENT project directory will be automatically included.
# You can manually set MY_PROJ_SRC to include source files under other directories.
set(MY_PROJ_SRC "")

# Search mode for source files under CURRENT project directory?
# "GLOB_RECURSE" for recursive search
# "GLOB" for non-recursive search
set(MY_SRC_SEARCH_MODE "GLOB")

message(STATUS "GTEST_INCLUDE_DIRS = ${GTEST_INCLUDE_DIRS}")
set(MY_PROJ_INC_PATH
	${GTEST_INCLUDE_DIRS} 
	../dist/failure_detector 
	../apps/replication/client_lib 
	../apps/replication/lib 
	../apps/replication/meta_server
	)

if (UNIX)
    set(MY_PROJ_LIBS gtest pthread)
else()
    set(MY_PROJ_LIBS gtest)
endif()

set(MY_PROJ_LIBS 
	dsn.replication.meta_server
	dsn.replication
	dsn.replication.clientlib
	dsn.failure_detector	
	${MY_PROJ_LIBS}
	)

set(MY_BOOST_PACKAGES system)

set(MY_PROJ_LIB_PATH "")

# Extra files that will be installed
set(MY_BINPLACES
    "${CMAKE_CURRENT_SOURCE_DIR}/run.sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clear.sh"
    "${CMAKE_CURRENT_SOURCE_DIR}/config-test.ini"
    "${CMAKE_CURRENT_SOURCE_DIR}/log.1.0"
    "${CMAKE_CURRENT_SOURCE_DIR}/log.1.1"
    "${CMAKE_CURRENT_SOURCE_DIR}/log.1.2"
    "${CMAKE_CURRENT_SOURCE_DIR}/log.1.3"
    "${CMAKE_CURRENT_SOURCE_DIR}/log.1.4"
)

dsn_add_executable()
