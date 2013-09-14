include_directories(${CMAKE_SOURCE_DIR}/include)

foreach(program ${PROGRAMS})
  add_executable(${program} ${program}.c)
  target_link_libraries(${program} ${CORELIBS})
endforeach(program)

#building just a library. 
add_library(netfn_app netfn_app.c)
add_library(netfn_chassis netfn_chassis.c)
add_library(netfn_transport netfn_transport.c)

