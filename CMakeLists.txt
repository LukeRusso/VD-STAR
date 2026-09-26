cmake_minimum_required(VERSION 3.25)
project(DynamicStrClu_ours)
find_package(pybind11 CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(BOOST_INCLUDE_LIBRARIES serialization)
add_subdirectory(boost_1_92_0 boost_build EXCLUDE_FROM_ALL)
set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
add_subdirectory(catch2 catch2_build EXCLUDE_FROM_ALL)

set(VDSTAR_ENGINE_SOURCES
    graph/Graph.cpp graph/Vertex.cpp
    dt/DTBucket.cpp dt/DTInstance.cpp dt/DTManager.cpp
    MyLib/MyTimer.cpp)

add_library(vdstar_engine STATIC ${VDSTAR_ENGINE_SOURCES})
set_target_properties(vdstar_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(vdstar_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vdstar_engine PUBLIC Boost::serialization)

add_executable(DynamicStrClu_ours main.cpp MyLib/ParaReader.cpp)
target_link_libraries(DynamicStrClu_ours PRIVATE vdstar_engine)

pybind11_add_module(vdstar_core bindings.cpp)
target_link_libraries(vdstar_core PRIVATE vdstar_engine)

if(DEFINED SKBUILD_PLATLIB_DIR)
    install(TARGETS vdstar_core LIBRARY DESTINATION ${SKBUILD_PLATLIB_DIR})
endif()

enable_testing()
add_subdirectory(tests)

