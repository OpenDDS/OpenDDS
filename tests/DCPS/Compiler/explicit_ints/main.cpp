#include <testTypeSupportImpl.h>

#include <gtest/gtest.h>

#if CPP11_MAPPING
#  define REF(WHAT) (WHAT)()
#else
#  define REF(WHAT) (WHAT)
#endif

/* TEST(EscapedNonKeywords, struct_topic_type) */
/* { */
/* } */

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
