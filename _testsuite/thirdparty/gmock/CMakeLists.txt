list ( APPEND incldirs include/gmock include/gmock/internal include)
list ( APPEND incldirs gtest/include/ gtest/include/gtest gtest/include/gtest/internal)

foreach( dir ${incldirs} )
  include_directories( ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock/${incldirs} )
endforeach( )
include_directories( ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock/ )
include_directories( ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock/gtest/ )

set( sources "" )
list(APPEND sources
  ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock/src/gmock-all.cc
  ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock/gtest/src/gtest-all.cc
  ${CMAKE_HOME_DIRECTORY}/_testsuite/thirdparty/gmock/src/gmock_main.cc
)


#file( GLOB_RECURSE srcs "${CMAKE_HOME_DIRECTORY}/mdp_msdk-contrib/gmock/*.cc" "${CMAKE_HOME_DIRECTORY}/mdp_msdk-contrib/gmock/*.cpp" )
#list( APPEND sources ${srcs})

make_library( gmock none static )
