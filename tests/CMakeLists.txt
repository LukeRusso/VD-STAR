include(Catch)
set(VDSTAR_SANITIZE -fsanitize=address,undefined -fno-omit-frame-pointer)

function(vdstar_test name)
  file(GLOB sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
  add_executable(${name} ${sources})
  target_link_libraries(${name} PRIVATE vdstar_engine Catch2::Catch2WithMain)
  target_compile_options(${name} PRIVATE ${VDSTAR_SANITIZE} -g)
  target_link_options(${name} PRIVATE ${VDSTAR_SANITIZE})
  catch_discover_tests(${name})
endfunction()

add_subdirectory(graph)
add_subdirectory(MyLib)
