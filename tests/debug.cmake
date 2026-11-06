file(MAKE_DIRECTORY "${SE_WORK}")
set(program "${SE_WORK}/debug.se")
file(WRITE "${program}" "make twice n:Int -> Int\n    give n * 2\nsay twice 21\n")
execute_process(COMMAND "${SE_EXE}" trace "${program}"
  RESULT_VARIABLE traced OUTPUT_VARIABLE trace_out ERROR_VARIABLE trace_err)
if(NOT traced EQUAL 0 OR NOT trace_out STREQUAL "42\n" OR NOT trace_err MATCHES "SE trace")
  message(FATAL_ERROR "SE trace did not preserve stdout and show execution: ${trace_out} ${trace_err}")
endif()
file(WRITE "${SE_WORK}/commands.txt" "bt\nlocals\nc\n")
execute_process(COMMAND "${SE_EXE}" debug --break 2 "${program}"
  INPUT_FILE "${SE_WORK}/commands.txt" RESULT_VARIABLE debug_code
  OUTPUT_VARIABLE debug_out ERROR_VARIABLE debug_err)
if(NOT debug_code EQUAL 0 OR NOT debug_out STREQUAL "42\n"
   OR NOT debug_err MATCHES "\\(se-debug\\)" OR NOT debug_err MATCHES "twice")
  message(FATAL_ERROR "SE breakpoint/stack debug failed: ${debug_out} ${debug_err}")
endif()
