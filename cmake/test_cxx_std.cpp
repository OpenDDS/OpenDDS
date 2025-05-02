#define OPENDDS_STR_(x) #x
#define OPENDDS_STR(x) OPENDDS_STR_(x)

#ifdef OPENDDS_TEST_CPLUSPLUS
#  ifdef _MSVC_LANG
#    if _MSVC_LANG < OPENDDS_TEST_CPLUSPLUS
#      error "_MSVC_LANG is less than requested value"
#    endif
#  elif __cplusplus < OPENDDS_TEST_CPLUSPLUS
#    ifdef __GNUC__
#      pragma GCC error "__cplusplus (" OPENDDS_STR(__cplusplus) \
  ") is less than requested value (" OPENDDS_STR(OPENDDS_TEST_CPLUSPLUS) ")"
#    else
#      error "__cplusplus is less than requested value"
#    endif
#  endif
#elif defined OPENDDS_TEST_ACE_CXX_STD
#  define ACE_DOESNT_DEFINE_MAIN 1
#  include <ace/Global_Macros.h>
#else
#  error "No macro was defined!"
#endif

int main()
{
  return 0;
}
