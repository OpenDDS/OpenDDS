## Get the base directory
get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

## Set values expected by OpenDDS
set(ACE_ROOT "${PACKAGE_PREFIX_DIR}/ACE")
set(ACE_BIN_DIR ${ACE_ROOT}/bin)
set(ACE_INCLUDE_DIRS ${ACE_ROOT})
set(ACE_FOR_OPENDDS_LIB_DIR ${ACE_ROOT}/lib)

## These should be removed for OpenDDS v4.0
set(TAO_ROOT "${PACKAGE_PREFIX_DIR}/TAO")
set(TAO_BIN_DIR ${ACE_ROOT}/bin)
set(TAO_LIB_DIR ${ACE_ROOT}/lib)
set(TAO_INCLUDE_DIR ${TAO_ROOT})
set(TAO_INCLUDE_DIRS "${TAO_INCLUDE_DIR}" "${TAO_INCLUDE_DIR}/orbsvcs")

add_library(ACE::ACE INTERFACE IMPORTED)
set_target_properties(ACE::ACE PROPERTIES INTERFACE_LINK_LIBRARIES
                      "$<IF:$<CONFIG:Debug>,ACE_for_OpenDDSd,ACE_for_OpenDDS>")

## Set values for use within projects
set(ACE_LIB ACE::ACE)
