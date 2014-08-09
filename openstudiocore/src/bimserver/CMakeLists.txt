set(target_name openstudio_bimserver)

set(${target_name}_src
  BIMserverAPI.hpp
  mainpage.hpp
  ReverseTranslator.hpp
  ReverseTranslator.cpp
)

set(${target_name}_test_src
  Test/BIMserverFixture.hpp
  Test/BIMserverFixture.cpp
  Test/ReverseTranslator_GTest.cpp
)

set(${target_name}_swig_src
  BIMserver.i
)

set(${target_name}_depends
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS}
  ${QT_LIBS}
  openstudio_utilities
  openstudio_model
  openstudio_energyplus
  zkexpat
)

add_library(${target_name} SHARED ${${target_name}_src})
target_link_libraries(${target_name} ${${target_name}_depends})
AddPCH(${target_name})

#if(NOT APPLE)
install(TARGETS ${target_name}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)
#endif()

CREATE_SRC_GROUPS("${${target_name}_src}")
CREATE_SRC_GROUPS("${${target_name}_test_src}")
CREATE_SRC_GROUPS("${${target_name}_swig_src}")

CREATE_TEST_TARGETS(${target_name} "${${target_name}_test_src}" "${${target_name}_depends}")
if(BUILD_TESTING)
  add_dependencies(${target_name}_tests openstudio_bimserver_resources)
endif()

#MAKE_SWIG_TARGET(OpenStudioGBXML gbXML "${CMAKE_CURRENT_SOURCE_DIR}/gbXML.i" "${${target_name}_swig_src}" ${target_name} OpenStudioEnergyPlus)

