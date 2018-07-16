cmake_minimum_required(VERSION 2.8)
project(lfstack)

set(PROJNAME lfstack)

IF (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
SET(CMAKE_INSTALL_INCLUDEDIR /usr/local/include)
ENDIF(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)

file(GLOB_RECURSE sources lfstack.c lfstack.h)
file(GLOB_RECURSE sources_test test.cpp)
file(GLOB_RECURSE example_test lfstack.c lfstack.h example.c)


add_executable(lfstack-example ${example_test})
target_link_libraries(lfstack-example PUBLIC pthread atomic)

include_directories(src /usr/local/include)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wstrict-prototypes -Wmissing-prototypes")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-qual")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare -std=c11 -pedantic")


###############################################################################
## packaging ##################################################################
###############################################################################
add_library(${PROJNAME}.static STATIC ${sources})
target_link_libraries(${PROJNAME}.static atomic)
set_target_properties(${PROJNAME}.static PROPERTIES OUTPUT_NAME ${PROJNAME})
add_library(${PROJNAME} SHARED ${sources})
target_link_libraries(${PROJNAME} atomic)

###############################################################################
## testing ####################################################################
###############################################################################

# this is for our testing framework
# we don't add REQUIRED because it's just for testing
find_package(GTest)

if(GTEST_FOUND)
  
  if(NOT ${CMAKE_VERSION} LESS 3.2)
      set(CMAKE_CXX_STANDARD 11)
      set(CMAKE_CXX_STANDARD_REQUIRED ON)
  else()
      message(STATUS "Checking compiler flags for C++11 support.")
      # Set C++11 support flags for various compilers
      include(CheckCXXCompilerFlag)
      check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
      check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
      if(COMPILER_SUPPORTS_CXX11)
          message(STATUS "C++11 is supported.")
          if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
              set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
          else()
              set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
          endif()
      elseif(COMPILER_SUPPORTS_CXX0X)
          message(STATUS "C++0x is supported.")
          if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
              set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -stdlib=libc++")
          else()
              set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
          endif()
      else()
          message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
      endif()
  endif()
  message(STATUS "GTEST Available.")
  enable_testing()

  add_executable(lfstack-concurr-test ${sources_test})

  # we add this define to prevent collision with the main
  # this might be better solved by not adding the source with the main to the
  # testing target
  target_compile_definitions(lfstack-concurr-test PUBLIC UNIT_TESTS)

  # this allows us to use our executable as a link library
  # therefore we can inherit all compiler options and library dependencies
  set_target_properties(${PROJ} PROPERTIES ENABLE_EXPORTS on)

  target_link_libraries(lfstack-concurr-test PUBLIC
    ${GTEST_BOTH_LIBRARIES} lfstack pthread
    # addr_svc_cpp
  )

  target_include_directories(lfstack-concurr-test PUBLIC
    ${GTEST_INCLUDE_DIRS} # doesn't do anything on Linux
  )

  add_test(lfstacktest lfstack-concurr-test)

endif()


# link_directories(/usr/local/lib /usr/lib)

## For .a library
install(TARGETS ${PROJNAME}.static EXPORT ${PROJNAME}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

## For .so library
install(TARGETS ${PROJNAME} EXPORT ${PROJNAME}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
install(FILES lfstack.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/lfsaq)

install(CODE "execute_process(COMMAND ldconfig)")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

set(CPACK_PACKAGE_NAME "lfstack")
set(CPACK_PACKAGE_VERSION "1.0.0")

# we don't want to split our program up into several things
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
