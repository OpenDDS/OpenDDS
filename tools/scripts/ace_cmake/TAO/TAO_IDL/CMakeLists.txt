cmake_minimum_required(VERSION 3.12.0)

if(WIN32)
  link_libraries(iphlpapi)
endif()

project(TAO_IDL_FE CXX)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(ACE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../ACE")
set(ACE_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/../../ACE/ace")
set(TAO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(HEADER_FILES_TAO_IDL_FE fe/idl.tab.hpp include/TAO_IDL_FE_Export.h include/ast.h include/ast_annotation_appl.h include/ast_annotation_appls.h include/ast_annotation_decl.h include/ast_annotation_member.h include/ast_argument.h include/ast_array.h include/ast_attribute.h include/ast_component.h include/ast_component_fwd.h include/ast_concrete_type.h include/ast_connector.h include/ast_constant.h include/ast_consumes.h include/ast_decl.h include/ast_emits.h include/ast_enum.h include/ast_enum_val.h include/ast_eventtype.h include/ast_eventtype_fwd.h include/ast_exception.h include/ast_expression.h include/ast_extended_port.h include/ast_extern.h include/ast_factory.h include/ast_field.h include/ast_finder.h include/ast_fixed.h include/ast_generator.h include/ast_home.h include/ast_interface.h include/ast_interface_fwd.h include/ast_mirror_port.h include/ast_module.h include/ast_native.h include/ast_operation.h include/ast_param_holder.h include/ast_porttype.h include/ast_predefined_type.h include/ast_provides.h include/ast_publishes.h include/ast_root.h include/ast_sequence.h include/ast_string.h include/ast_structure.h include/ast_structure_fwd.h include/ast_template_module.h include/ast_template_module_inst.h include/ast_template_module_ref.h include/ast_type.h include/ast_typedef.h include/ast_union.h include/ast_union_branch.h include/ast_union_fwd.h include/ast_union_label.h include/ast_uses.h include/ast_valuebox.h include/ast_valuetype.h include/ast_valuetype_fwd.h include/ast_visitor.h include/ast_visitor_context.h include/ast_visitor_reifying.h include/ast_visitor_tmpl_module_inst.h include/ast_visitor_tmpl_module_ref.h include/drv_extern.h include/fe_component_header.h include/fe_declarator.h include/fe_extern.h include/fe_home_header.h include/fe_interface_header.h include/fe_obv_header.h include/fe_private.h include/fe_utils.h include/global_extern.h include/idl_defines.h include/idl_global.h include/idl_version.h include/nr_extern.h include/utl_decllist.h include/utl_err.h include/utl_exceptlist.h include/utl_exprlist.h include/utl_identifier.h include/utl_idlist.h include/utl_indenter.h include/utl_labellist.h include/utl_list.h include/utl_namelist.h include/utl_scope.h include/utl_scoped_name.h include/utl_stack.h include/utl_string.h include/utl_strlist.h)

set(TEMPLATE_FILES_TAO_IDL_FE include/utl_scope_T.cpp)

set(SOURCE_FILES_TAO_IDL_FE ast/ast_annotation_appl.cpp ast/ast_annotation_appls.cpp ast/ast_annotation_decl.cpp ast/ast_annotation_member.cpp ast/ast_argument.cpp ast/ast_array.cpp ast/ast_attribute.cpp ast/ast_check.cpp ast/ast_component.cpp ast/ast_component_fwd.cpp ast/ast_concrete_type.cpp ast/ast_connector.cpp ast/ast_constant.cpp ast/ast_consumes.cpp ast/ast_decl.cpp ast/ast_emits.cpp ast/ast_enum.cpp ast/ast_enum_val.cpp ast/ast_eventtype.cpp ast/ast_eventtype_fwd.cpp ast/ast_exception.cpp ast/ast_expression.cpp ast/ast_extended_port.cpp ast/ast_factory.cpp ast/ast_field.cpp ast/ast_finder.cpp ast/ast_fixed.cpp ast/ast_generator.cpp ast/ast_home.cpp ast/ast_interface.cpp ast/ast_interface_fwd.cpp ast/ast_mirror_port.cpp ast/ast_module.cpp ast/ast_native.cpp ast/ast_operation.cpp ast/ast_param_holder.cpp ast/ast_porttype.cpp ast/ast_predefined_type.cpp ast/ast_provides.cpp ast/ast_publishes.cpp ast/ast_recursive.cpp ast/ast_root.cpp ast/ast_sequence.cpp ast/ast_string.cpp ast/ast_structure.cpp ast/ast_structure_fwd.cpp ast/ast_template_module.cpp ast/ast_template_module_inst.cpp ast/ast_template_module_ref.cpp ast/ast_type.cpp ast/ast_typedef.cpp ast/ast_union.cpp ast/ast_union_branch.cpp ast/ast_union_fwd.cpp ast/ast_union_label.cpp ast/ast_uses.cpp ast/ast_valuebox.cpp ast/ast_valuetype.cpp ast/ast_valuetype_fwd.cpp ast/ast_visitor.cpp ast/ast_visitor_context.cpp ast/ast_visitor_reifying.cpp ast/ast_visitor_tmpl_module_inst.cpp ast/ast_visitor_tmpl_module_ref.cpp fe/fe_component_header.cpp fe/fe_declarator.cpp fe/fe_extern.cpp fe/fe_global.cpp fe/fe_home_header.cpp fe/fe_init.cpp fe/fe_interface_header.cpp fe/fe_lookup.cpp fe/fe_obv_header.cpp fe/fe_private.cpp fe/fe_utils.cpp fe/idl.tab.cpp fe/idl.yy.cpp narrow/narrow.cpp util/idl_version.cpp util/utl_decllist.cpp util/utl_err.cpp util/utl_exceptlist.cpp util/utl_exprlist.cpp util/utl_global.cpp util/utl_identifier.cpp util/utl_idlist.cpp util/utl_indenter.cpp util/utl_labellist.cpp util/utl_list.cpp util/utl_namelist.cpp util/utl_scope.cpp util/utl_stack.cpp util/utl_string.cpp util/utl_strlist.cpp)
if(CMAKE_CONFIGURATION_TYPES)
  set(TARGET_LINK_LIBRARIES_TAO_IDL_FE
      $<$<CONFIG:Debug>:ACE_for_OpenDDS${LIBRARY_DECORATOR}d>
      $<$<CONFIG:Release>:ACE_for_OpenDDS${LIBRARY_DECORATOR}>)
else()
  set(TARGET_LINK_LIBRARIES_TAO_IDL_FE ACE_for_OpenDDS${LIBRARY_DECORATOR})
endif()
set(PROJECT_TARGET_TAO_IDL_FE TAO_IDL_FE${LIBRARY_DECORATOR})
if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS
                          _CRT_SECURE_NO_DEPRECATE
                          _CRT_NONSTDC_NO_DEPRECATE)
  add_compile_options(/wd4355)
  add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()


add_library(${PROJECT_TARGET_TAO_IDL_FE} ${SOURCE_FILES_TAO_IDL_FE})
if(BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_TARGET_TAO_IDL_FE} PUBLIC
                             TAO_IDL_FE_BUILD_DLL)
else()
  target_compile_definitions(${PROJECT_TARGET_TAO_IDL_FE} PUBLIC
                             ACE_AS_STATIC_LIBS)
endif()
target_link_libraries(${PROJECT_TARGET_TAO_IDL_FE}
                      ${TARGET_LINK_LIBRARIES_TAO_IDL_FE})
set_target_properties(${PROJECT_TARGET_TAO_IDL_FE} PROPERTIES DEBUG_POSTFIX "d")

if(CMAKE_CONFIGURATION_TYPES)
  target_compile_definitions(${PROJECT_TARGET_TAO_IDL_FE} PUBLIC
                             $<$<CONFIG:Debug>:MPC_LIB_MODIFIER="${LIBRARY_DECORATOR}d">
                             $<$<CONFIG:Release>:MPC_LIB_MODIFIER="${LIBRARY_DECORATOR}">)
endif()

target_include_directories(${PROJECT_TARGET_TAO_IDL_FE} PRIVATE
                           ${ACE_ROOT} ${TAO_ROOT} include fe)

target_link_directories(${PROJECT_TARGET_TAO_IDL_FE} PRIVATE ${ACE_LIB_DIR})

target_compile_definitions(${PROJECT_TARGET_TAO_IDL_FE} PUBLIC
                           ACE_LACKS_ACE_TOKEN ACE_LACKS_MSG_WFMO)

install(FILES ${HEADER_FILES_TAO_IDL_FE}
              ${TEMPLATE_FILES_TAO_IDL_FE}
        DESTINATION "TAO/TAO_IDL/include")

install(TARGETS ${PROJECT_TARGET_TAO_IDL_FE}
        DESTINATION "ACE/lib")
