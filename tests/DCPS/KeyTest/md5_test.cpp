#include "ace/OS_main.h"
#include "../common/TestSupport.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RTPS/md5.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

void print_hex(void* d)
{
  unsigned char* a = (unsigned char*) d;
  printf("0x");
  for (int j=0;j<16; j++){
    printf("%02x",a[j]);
  }
  printf("\n");
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  // Unit test of public domain MD5 Hash implementation
  MD5_CTX ctx;
  unsigned char d[16];

  {
    const char *msg = "The quick brown fox jumps over the lazy dog.";
    unsigned char expected[16] = { 0xe4, 0xd9, 0x09, 0xc2,
                                   0x90, 0xd0, 0xfb, 0x1c,
                                   0xa0, 0x68, 0xff, 0xad,
                                   0xdf, 0x22, 0xcb, 0xd0 };
    MD5_Init(&ctx);
    MD5_Update(&ctx, msg, static_cast<unsigned long>(strlen(msg)));
    MD5_Final(d, &ctx);
    print_hex(d);
    print_hex(expected);
    TEST_CHECK(memcmp(expected, d, 16) == 0);
  }

  {
    const char *msg = "";
    unsigned char expected[16] = { 0xd4, 0x1d, 0x8c, 0xd9,
                                   0x8f, 0x00, 0xb2, 0x04,
                                   0xe9, 0x80, 0x09, 0x98,
                                   0xec, 0xf8, 0x42, 0x7e };
    MD5_Init(&ctx);
    MD5_Update(&ctx, msg, static_cast<unsigned long>(strlen(msg)));
    MD5_Final(d, &ctx);
    print_hex(d);
    print_hex(expected);
    TEST_CHECK(memcmp(expected, d, 16) == 0);
  }

  {
    const char *msg = "The quick brown fox jumps over the lazy dog";
    unsigned char expected[16] = { 0x9e, 0x10, 0x7d, 0x9d,
                                   0x37, 0x2b, 0xb6, 0x82,
                                   0x6b, 0xd8, 0x1d, 0x35,
                                   0x42, 0xa4, 0x19, 0xd6 };
    MD5_Init(&ctx);
    MD5_Update(&ctx, msg, static_cast<unsigned long>(strlen(msg)));
    MD5_Final(d, &ctx);
    print_hex(d);
    print_hex(expected);
    TEST_CHECK(memcmp(expected, d, 16) == 0);
  }

  return 0;
}
