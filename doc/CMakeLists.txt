find_package(Doxygen REQUIRED dot)

set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
add_custom_target(doxygen ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generarting API documentation with Doxygen"
    VERBATIM)
set_property(TARGET doxygen PROPERTY PROJECT_LABEL "Documentation")
set_target_properties(doxygen PROPERTIES FOLDER "Support")
