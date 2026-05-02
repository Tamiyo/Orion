set(YUZU_TD_DIR "${CMAKE_SOURCE_DIR}/yuzu/lib/TableGen")

file(GLOB YUZU_TD_SOURCES "${YUZU_TD_DIR}/*.td")

function(yuzu_tablegen target_name relative_output backend_flag)
  set(input "${YUZU_TD_DIR}/Grammar.td")
  set(output "${CMAKE_BINARY_DIR}/include/${relative_output}")

  get_filename_component(output_dir "${output}" DIRECTORY)

  add_custom_command(
    OUTPUT "${output}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
    COMMAND $<TARGET_FILE:yuzu-tblgen> ${backend_flag} -I "${YUZU_TD_DIR}" -o "${output}" "${input}"
    DEPENDS yuzu-tblgen ${YUZU_TD_SOURCES}
    COMMENT "yuzu-tblgen ${backend_flag} -> ${relative_output}"
    VERBATIM
  )

  add_custom_target(${target_name} DEPENDS "${output}")
endfunction()
