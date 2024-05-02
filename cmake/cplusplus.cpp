// msvc doesn't define __cplusplus by default for some reason
#ifdef _MSVC_LANG
#  if _MSVC_LANG < OPENDDS_TEST_CPLUSPLUS
#    error "Less then requested value"
#  endif
#elif __cplusplus < OPENDDS_TEST_CPLUSPLUS
#  error "Less then requested value"
#endif

int main()
{
  return 0;
}
