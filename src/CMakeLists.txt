include_directories(${CMAKE_SOURCE_DIR}/include)
link_directories(${CMAKE_BINARY_DIR}/lib)

add_executable(fake-ipmistack fake-ipmistack.c)
target_link_libraries(fake-ipmistack ${CORELIBS} netfn_app)
target_link_libraries(fake-ipmistack ${CORELIBS} netfn_chassis)
target_link_libraries(fake-ipmistack ${CORELIBS} netfn_transport)

foreach(program ${PROGRAMS})
  add_executable(${program} ${program}.c)
  target_link_libraries(${program} ${CORELIBS})
endforeach(program)

