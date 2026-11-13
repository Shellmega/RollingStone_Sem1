# Keep Ninja's dependency scanner in the compiler's actual output encoding.
# CMake's cached prefix can be mojibake on mixed UTF-8/CP936 Windows consoles.
if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
  set(probe "${CMAKE_CURRENT_BINARY_DIR}/msvc_includes_probe.cpp")
  file(WRITE "${probe}" "#include <stddef.h>\n")
  execute_process(COMMAND "${CMAKE_CXX_COMPILER}" /nologo /showIncludes /c "${probe}" "/Fo${CMAKE_CURRENT_BINARY_DIR}/msvc_includes_probe.obj"
    OUTPUT_VARIABLE includes_out ERROR_VARIABLE includes_err RESULT_VARIABLE includes_rc ENCODING NONE)
  set(includes_text "${includes_out}\n${includes_err}")
  string(REGEX MATCH "(^|[\r\n])([^:\r\n]+: [^:\r\n]+:)[ ]+" includes_match "${includes_text}")
  if(includes_rc EQUAL 0 AND includes_match)
    set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_MATCH_2} ")
  else()
    message(FATAL_ERROR "Unable to detect the MSVC /showIncludes prefix. Run build_3d.cmd in a Visual Studio C++ environment.")
  endif()
endif()
