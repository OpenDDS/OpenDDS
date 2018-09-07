
define_property(SOURCE PROPERTY DDS_IDL_FLAGS
  BRIEF_DOCS "sets additional opendds_idl compiler flags used to build sources within the target"
  FULL_DOCS "sets additional opendds_idl compiler flags used to build sources within the target"
)

define_property(SOURCE PROPERTY DDS_IDL_MAIN_TARGET
  BRIEF_DOCS "the main target to process the IDL file"
  FULL_DOCS "the main target to process the IDL file"
)


set(DDS_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})

if (NOT DEFINED DDS_BASE_IDL_FLAGS)
  if (OPENDDS_SUPPRESS_ANYS)
    list(APPEND TAO_BASE_IDL_FLAGS -Sa -St)
    list(APPEND DDS_BASE_IDL_FLAGS -Sa -St)
  endif()
  foreach(definition ${DCPS_COMPILE_DEFINITIONS})
    list(APPEND TAO_BASE_IDL_FLAGS -D${definition})
    list(APPEND DDS_BASE_IDL_FLAGS -D${definition})
  endforeach()
endif()

if (WIN32 AND NOT TAO_INSTALL_DIR)
  ## we use TAO_INSTALL_DIR to check if tao_idl is imported or not;
  ## TAO_INSTALL_DIR is only defined when TAO is not imported
  get_target_property(tao_idl_fe_loc TAO_IDL_FE LOCATION)
  get_filename_component(tao_idl_fe_dir ${tao_idl_fe_loc} DIRECTORY)

  set(IDL_PATH_ENV "PATH=\"${tao_idl_fe_dir}\;%PATH%\"")
endif()



set(FACE_TAO_IDL_FLAGS -SS -Wb,no_fixed_err)
set(FACE_DDS_IDL_FLAGS -GfaceTS -Lface)

function(dds_idl_command Name)
  set(dds_idl_command_usage "dds_idl_command(<Name> TAO_IDL_FLAGS flags DDS_IDL_FLAGS flags IDL_FILES Input1 Input2 ...]")

  set(multiValueArgs TAO_IDL_FLAGS DDS_IDL_FLAGS IDL_FILES USED_BY WORKING_DIRECTORY)
  cmake_parse_arguments(_arg "NO_TAO_IDL" "" "${multiValueArgs}" ${ARGN})

  if ((CMAKE_GENERATOR MATCHES "Visual Studio") AND (_arg_USED_BY MATCHES ";"))
    set(exclude_cpps_from_command_output ON)
  endif()

  if (NOT IS_ABSOLUTE "${_arg_WORKING_DIRECTORY}")
    set(_working_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/${_arg_WORKING_DIRECTORY})
    set(_working_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/${_arg_WORKING_DIRECTORY})
  else()
    set(_working_binary_dir ${_arg_WORKING_DIRECTORY})
    set(_working_source_dir ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  ## remove trailing slashes
  string(REGEX REPLACE "/$" "" _working_binary_dir ${_working_binary_dir})
  string(REGEX REPLACE "/$" "" _working_source_dir ${_working_source_dir})

  ## opendds_idl would generate different codes with the -I flag followed by absolute path
  ## or relative path, if it's a relatvie path we need to keep it a relative path to the binary tree
  file(RELATIVE_PATH _rel_path_to_source_tree ${_working_binary_dir} ${_working_source_dir})
  if (_rel_path_to_source_tree)
    string(APPEND _rel_path_to_source_tree "/")
  endif ()

  foreach(flag ${_arg_DDS_IDL_FLAGS})
    if ("${flag}" MATCHES "^-I(\\.\\..*)")
       list(APPEND _converted_dds_idl_flags -I${_rel_path_to_source_tree}${CMAKE_MATCH_1})
     else()
       list(APPEND _converted_dds_idl_flags ${flag})
    endif()
  endforeach()

  set(_ddsidl_flags ${DDS_BASE_IDL_FLAGS} ${_converted_dds_idl_flags})

  # cmake_parse_arguments(_ddsidl_cmd_arg "-SI;-GfaceTS" "-o" "" ${_ddsidl_flags})

  set(_dds_idl_outputs)
  set(_dds_idl_cpp_files)
  set(_dds_idl_headers)
  set(_type_support_idls)
  set(_type_support_javas)
  set(_taoidl_inputs)

  foreach(input ${_arg_IDL_FILES})
    unset(_ddsidl_cmd_arg_-SI)
    unset(_ddsidl_cmd_arg_-GfaceTS)
    unset(_ddsidl_cmd_arg_-o)
    unset(_ddsidl_cmd_arg_-Wb,java)

    get_property(file_dds_idl_flags SOURCE ${input} PROPERTY DDS_IDL_FLAGS)
    cmake_parse_arguments(_ddsidl_cmd_arg "-SI;-GfaceTS;-Wb,java" "-o" "" ${_ddsidl_flags} ${file_dds_idl_flags})

    get_filename_component(noext_name ${input} NAME_WE)
    get_filename_component(abs_filename ${input} ABSOLUTE)
    get_filename_component(file_ext ${input} EXT)

    if (_ddsidl_cmd_arg_-o)
      set(output_prefix ${_working_binary_dir}/${_ddsidl_cmd_arg_-o}/${noext_name})
    else()
      set(output_prefix ${_working_binary_dir}/${noext_name})
    endif()

    if (NOT _ddsidl_cmd_arg_-SI)
      set(_cur_type_support_idl ${output_prefix}TypeSupport.idl)
      list(APPEND _type_support_idls ${_cur_type_support_idl})
      list(APPEND _taoidl_inputs ${_cur_type_support_idl})
    else()
      unset(_cur_type_support_idl)
    endif()

    set(_cur_idl_headers ${output_prefix}TypeSupportImpl.h)
    set(_cur_idl_cpp_files ${output_prefix}TypeSupportImpl.cpp)

    if (_ddsidl_cmd_arg_-GfaceTS)
      list(APPEND _cur_idl_headers ${output_prefix}C.h ${output_prefix}_TS.hpp)
      list(APPEND _cur_idl_cpp_files ${output_prefix}_TS.cpp)
      ## if this is FACE IDL, do not reprocess the original idl file throught tao_idl
    else()
      list(APPEND _taoidl_inputs ${input})
    endif()

    list(APPEND _dds_idl_headers ${_cur_idl_headers})
    list(APPEND _dds_idl_cpp_files ${_cur_idl_cpp_files})

    if (_ddsidl_cmd_arg_-Wb,java)
      set(_cur_java_list "${output_prefix}${file_ext}.TypeSupportImpl.java.list")
      list(APPEND _type_support_javas "@${_cur_java_list}")
      list(APPEND file_dds_idl_flags -j)
    else()
      unset(_cur_java_list)
    endif()

    set(_cur_idl_outputs ${_cur_idl_headers})
    if (NOT exclude_cpps_from_command_output)
      list(APPEND _cur_idl_outputs ${_cur_idl_cpp_files})
    endif()

    add_custom_command(
      OUTPUT ${_cur_idl_outputs} ${_cur_type_support_idl} ${_cur_java_list}
      DEPENDS opendds_idl ${OpenDDS_INCLUDE_DIR}/dds/idl/IDLTemplate.txt
      MAIN_DEPENDENCY ${abs_filename}
      COMMAND ${CMAKE_COMMAND} -E env "DDS_ROOT=${OpenDDS_INCLUDE_DIR}"  "TAO_ROOT=${TAO_INCLUDE_DIR}" "${IDL_PATH_ENV}"
              $<TARGET_FILE:opendds_idl> -I${_working_source_dir}
              ${_ddsidl_flags} ${file_dds_idl_flags} ${abs_filename}
      WORKING_DIRECTORY ${_arg_WORKING_DIRECTORY}
    )

  endforeach(input)

  if (NOT _arg_NO_TAO_IDL)
    tao_idl_command(${Name}
      IDL_FLAGS -I${OpenDDS_INCLUDE_DIR} ${_arg_TAO_IDL_FLAGS}
      IDL_FILES ${_taoidl_inputs}
      USED_BY ${_arg_USED_BY}
    )
  endif()

  list(APPEND ${Name}_CPP_FILES ${_dds_idl_cpp_files})
  list(APPEND ${Name}_HEADER_FILES ${_dds_idl_headers})
  list(APPEND ${Name}_TYPESUPPORT_IDLS ${_type_support_idls})
  list(APPEND ${Name}_JAVA_OUTPUTS ${_type_support_javas})
  list(APPEND ${Name}_OUTPUT_FILES ${_dds_idl_cpp_files} ${_dds_idl_headers} ${_type_support_idls} ${_type_support_javas})

  if (EXCLUDE_CPPS_FROM_COMMAND_OUTPUT)
    add_custom_command(
      OUTPUT ${_dds_idl_cpp_files}
      COMMAND ${CMAKE_COMMAND} -E echo ""
    )
  endif()

  set(${Name}_OUTPUT_FILES ${${Name}_OUTPUT_FILES} PARENT_SCOPE)
  set(${Name}_CPP_FILES ${${Name}_CPP_FILES} PARENT_SCOPE)
  set(${Name}_HEADER_FILES ${${Name}_HEADER_FILES} PARENT_SCOPE)
  set(${Name}_TYPESUPPORT_IDLS ${${Name}_TYPESUPPORT_IDLS} PARENT_SCOPE)
  set(${Name}_JAVA_OUTPUTS ${${Name}_JAVA_OUTPUTS} PARENT_SCOPE)
endfunction()


function(dds_idl_sources)
  set(multiValueArgs TARGETS TAO_IDL_FLAGS DDS_IDL_FLAGS IDL_FILES)
  cmake_parse_arguments(_arg "NO_TAO_IDL" "" "${multiValueArgs}" ${ARGN})

  set(is_face OFF)

  tao_filter_valid_targets(_arg_TARGETS)

  if (NOT _arg_TARGETS)
    return()
  endif()

  foreach(target ${_arg_TARGETS})
    get_property(target_link_libs TARGET ${target} PROPERTY LINK_LIBRARIES)
    if ("OpenDDS_FACE" IN_LIST target_link_libs)
      set(is_face ON)
    endif()
    set(first_target ${target})
    break()
  endforeach()

  foreach(path ${_arg_IDL_FILES})
    get_property(main_target SOURCE ${path} PROPERTY DDS_IDL_MAIN_TARGET)

    if (main_target)
      message(FATAL_ERROR "${path} is added twice in ${CMAKE_CURRENT_LIST_FILE}")
    else()
      set_property(SOURCE ${path} PROPERTY DDS_IDL_MAIN_TARGET "${first_target}")
    endif()

    if (IS_ABSOLUTE ${path})
      list(APPEND _result ${path})
    else()
      list(APPEND _result ${CMAKE_CURRENT_LIST_DIR}/${path})
    endif()
  endforeach()

  set(_arg_IDL_FILES ${_result})

  file(RELATIVE_PATH rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR})

  if (_arg_NO_TAO_IDL)
    set(OPTIONAL_TAO_IDL NO_TAO_IDL)
  endif()

  if (is_face)
    list(APPEND _arg_TAO_IDL_FLAGS ${FACE_TAO_IDL_FLAGS})
    list(APPEND _arg_DDS_IDL_FLAGS ${FACE_DDS_IDL_FLAGS})
  endif()

  dds_idl_command(_idl
    ${OPTIONAL_TAO_IDL}
    TAO_IDL_FLAGS ${_arg_TAO_IDL_FLAGS}
    DDS_IDL_FLAGS ${_arg_DDS_IDL_FLAGS}
    IDL_FILES ${_arg_IDL_FILES}
    WORKING_DIRECTORY ${rel_path}
    USED_BY "${_arg_TARGETS}"
  )

  tao_setup_visual_studio_custom_command_fanout_dependencies(
    TARGETS "${_arg_TARGETS}"
    DEPENDS "${_arg_IDL_FILES};${_idl_TYPESUPPORT_IDLS};${_idl_HEADER_FILES}"
    OUTPUT  "${_idl_CPP_FILES}"
  )

  foreach(target ${_arg_TARGETS})
    ace_target_sources(${target} ${_idl_CPP_FILES})
    list(APPEND packages ${PACKAGE_OF_${target}})
  endforeach()


  if (_arg_DDS_IDL_FLAGS MATCHES "-Wb,export_macro=")
    set(CMAKE_INCLUDE_CURRENT_DIR ON PARENT_SCOPE)
  else()
    ## include only ${CMAKE_CURRENT_BINARY_DIR} when the IDL generated files do not include
    ## some export file. Usually, it's not a problem to include ${CMAKE_CURRENT_SOURCE_DIR} as well;
    ## however, it doesn't work for performance_tests/Bench/src on Windows. That directory
    ## contains a "Process.h" conflicting with the system "process.h" due to the case-insesitive
    ## file system on Windows.
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
  endif()

  source_group("Generated Files" FILES ${_idl_OUTPUT_FILES} )
  source_group("IDL Files" FILES ${_arg_IDL_FILES})
  set_source_files_properties(${_arg_IDL_FILES} ${_idl_HEADER_FILES} PROPERTIES HEADER_FILE_ONLY ON)
  set_property(SOURCE ${_idl_CPP_FILES} PROPERTY SKIP_AUTOGEN ON)

  set(DDS_IDL_TYPESUPPORT_IDLS ${_idl_TYPESUPPORT_IDLS} PARENT_SCOPE)
  set(DDS_IDL_JAVA_OUTPUTS ${_idl_JAVA_OUTPUTS} PARENT_SCOPE)

  if (packages)
    list(REMOVE_DUPLICATES packages)
  endif()

  foreach (package ${packages})
    set(package_root ${${package}_SOURCE_DIR})
    set(package_install_dir ${${package}_INSTALL_DIR})
    file(RELATIVE_PATH rel_path ${package_root} ${CMAKE_CURRENT_LIST_DIR})
    install(FILES ${_arg_IDL_FILES} ${_idl_HEADER_FILES}
            DESTINATION ${package_install_dir}/${rel_path})
  endforeach()
endfunction()
