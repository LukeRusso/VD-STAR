cmake_minimum_required(VERSION 3.25)
project(DynamicStrClu_ours)
find_package(pybind11 CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)

set(VDSTAR_ENGINE_SOURCES
    graph/Graph.cpp graph/Vertex.cpp graph/Jaccard.cpp
    dt/DTBucket.cpp dt/DTInstance.cpp dt/DTManager.cpp
    MyLib/MyTimer.cpp)

add_executable(DynamicStrClu_ours main.cpp ${VDSTAR_ENGINE_SOURCES} MyLib/ParaReader.cpp)
pybind11_add_module(vdstar_core bindings.cpp ${VDSTAR_ENGINE_SOURCES})
install(TARGETS vdstar_core DESTINATION ${SKBUILD_PLATLIB_DIR})