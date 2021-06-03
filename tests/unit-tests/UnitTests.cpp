#include <gtest/gtest.h>

#include "dds/DCPS/security/OpenSSL_init.h"

#include <ace/Init_ACE.h>

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  openssl_init();
  ACE::init();
  int result = RUN_ALL_TESTS();
  ACE::fini();
  return result;
}
