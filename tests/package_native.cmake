if(NOT DEFINED SE_ARCHIVE OR NOT DEFINED SE_ASSET OR NOT DEFINED SE_WORK)
  message(FATAL_ERROR "SE_ARCHIVE, SE_ASSET and SE_WORK are required")
endif()

# Exercise the actual archive after relocating it, through a symlink, with
# spaces in both the installation directory and the user's project directory.
file(MAKE_DIRECTORY "${SE_WORK}/extracted" "${SE_WORK}/user bin" "${SE_WORK}/SE code")
file(ARCHIVE_EXTRACT INPUT "${SE_ARCHIVE}" DESTINATION "${SE_WORK}/extracted")
set(package "${SE_WORK}/SE installation with spaces")
file(RENAME "${SE_WORK}/extracted/${SE_ASSET}" "${package}")
set(launcher "${SE_WORK}/user bin/se")
file(CREATE_LINK "${package}/bin/se" "${launcher}" SYMBOLIC RESULT linked)
if(NOT linked STREQUAL "0")
  message(FATAL_ERROR "Could not create installer-style symlink: ${linked}")
endif()
set(project "${SE_WORK}/SE code")
file(WRITE "${project}/hello.se" "say \"relocated native build OK\"\n")
execute_process(COMMAND "${launcher}" build "${project}/hello.se"
  WORKING_DIRECTORY "${project}" RESULT_VARIABLE built OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT built EQUAL 0)
  message(FATAL_ERROR "Relocated package native build failed:\n${out}\n${err}")
endif()
execute_process(COMMAND "${project}/hello"
  WORKING_DIRECTORY "${project}" RESULT_VARIABLE ran OUTPUT_VARIABLE out ERROR_VARIABLE err)
if(NOT ran EQUAL 0 OR NOT out STREQUAL "relocated native build OK\n")
  message(FATAL_ERROR "Relocated native program failed:\n${out}\n${err}")
endif()

# A broken package must report missing runtime files, even while the original
# CI source checkout remains available. It must not silently build against it.
file(RENAME "${package}/share/se/native" "${package}/share/se/native.saved")
execute_process(COMMAND "${launcher}" build "${project}/hello.se"
  WORKING_DIRECTORY "${project}" RESULT_VARIABLE missing OUTPUT_VARIABLE out ERROR_VARIABLE err)
file(RENAME "${package}/share/se/native.saved" "${package}/share/se/native")
if(missing EQUAL 0 OR NOT err MATCHES "SE native runtime files are missing")
  message(FATAL_ERROR "Missing-runtime diagnostic failed:\n${out}\n${err}")
endif()
message(STATUS "Relocated package native build and missing-runtime checks passed")
