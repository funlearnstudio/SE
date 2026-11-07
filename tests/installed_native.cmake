# Verify that 'se build' works outside the source checkout after installing
# its native runtime bundle. This deliberately uses an unrelated working dir.
if(NOT DEFINED SE_BUILD_DIR OR NOT DEFINED SE_WORK)
  message(FATAL_ERROR "SE_BUILD_DIR and SE_WORK are required")
endif()
file(REMOVE_RECURSE "${SE_WORK}")
file(MAKE_DIRECTORY "${SE_WORK}/unrelated project")
execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${SE_BUILD_DIR}" --prefix "${SE_WORK}/SE installation"
  RESULT_VARIABLE install_status OUTPUT_VARIABLE install_out ERROR_VARIABLE install_err)
if(NOT install_status EQUAL 0)
  message(FATAL_ERROR "SE installation failed:\n${install_out}\n${install_err}")
endif()
set(se "${SE_WORK}/SE installation/bin/se")
set(source "${SE_WORK}/unrelated project/hello.se")
file(WRITE "${source}" "say \"native installation OK\"\n")
execute_process(COMMAND "${se}" build "${source}"
  WORKING_DIRECTORY "${SE_WORK}/unrelated project"
  RESULT_VARIABLE build_status OUTPUT_VARIABLE build_out ERROR_VARIABLE build_err)
if(NOT build_status EQUAL 0)
  message(FATAL_ERROR "Installed SE failed to build outside its source tree:\n${build_out}\n${build_err}")
endif()
execute_process(COMMAND "${SE_WORK}/unrelated project/hello"
  WORKING_DIRECTORY "${SE_WORK}/unrelated project"
  RESULT_VARIABLE run_status OUTPUT_VARIABLE run_out ERROR_VARIABLE run_err)
if(NOT run_status EQUAL 0 OR NOT run_out STREQUAL "native installation OK\n")
  message(FATAL_ERROR "Native executable failed:\n${run_out}\n${run_err}")
endif()
# A broken package must fail clearly rather than silently relying on a
# development source checkout.
file(RENAME "${SE_WORK}/SE installation/share/se/native"
  "${SE_WORK}/SE installation/share/se/native.off")
execute_process(COMMAND "${se}" doctor RESULT_VARIABLE doctor_status
  OUTPUT_VARIABLE doctor_out ERROR_VARIABLE doctor_err)
if(NOT doctor_status EQUAL 0 OR NOT doctor_out MATCHES "native runtime: unavailable")
  message(FATAL_ERROR "Missing native runtime was not diagnosed:\n${doctor_out}\n${doctor_err}")
endif()
message(STATUS "Installed SE native build outside checkout passed")
