#include <gtest/gtest.h>

#ifdef OPENDDS_SECURITY
#include "dds/DCPS/security/OpenSSL_init.h"
#endif

#include <ace/Init_ACE.h>

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
#ifdef OPENDDS_SECURITY
  openssl_init();
#endif
  ACE::init();
  int result = RUN_ALL_TESTS();
  ACE::fini();
  return result;
}
