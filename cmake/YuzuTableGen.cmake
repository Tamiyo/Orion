# Root of the TableGen include tree. All `.td` files live under
# `yuzu/include/yuzu/...`; `include "..."` directives are resolved against
# this directory so cross-references like `include "yuzu/DSL/TreeBase.td"`
# work uniformly.
set(YUZU_TD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/yuzu/include")

# Every .td under the include root is a build-time dependency: any change
# triggers a rerun of yuzu-tblgen for downstream targets.
file(GLOB_RECURSE YUZU_TD_SOURCES "${YUZU_TD_INCLUDE_DIR}/*.td")

# Run yuzu-tblgen with `backend_flag` on `input_relpath` (relative to
# YUZU_TD_INCLUDE_DIR) and write the generated header to
# `${CMAKE_BINARY_DIR}/include/${output_relpath}`.
function(yuzu_tablegen target_name output_relpath backend_flag input_relpath)
  set(input "${YUZU_TD_INCLUDE_DIR}/${input_relpath}")
  set(output "${CMAKE_BINARY_DIR}/include/${output_relpath}")

  get_filename_component(output_dir "${output}" DIRECTORY)

  add_custom_command(
    OUTPUT "${output}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
    COMMAND $<TARGET_FILE:yuzu-tblgen> ${backend_flag} -I "${YUZU_TD_INCLUDE_DIR}" -o "${output}" "${input}"
    DEPENDS yuzu-tblgen ${YUZU_TD_SOURCES}
    COMMENT "yuzu-tblgen ${backend_flag} -> ${output_relpath}"
    VERBATIM
  )

  add_custom_target(${target_name} DEPENDS "${output}")
endfunction()
