#ifdef OPENDDS_TEST_CPLUSPLUS
#  if __cplusplus < OPENDDS_TEST_CPLUSPLUS
#    error "Less then requested value"
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
