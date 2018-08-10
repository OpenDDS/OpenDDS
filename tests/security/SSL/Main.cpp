
#include "gtest/gtest.h"
#include "dds/DCPS/security/OpenSSL_init.h"

int main(int argc, char* argv[])
{
  openssl_init();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
