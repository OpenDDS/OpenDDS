#ifndef FACE_TEST_UTILS
#define FACE_TEST_UTILS

#include "FACE/common.hpp"

namespace TestUtils {

inline
FACE::TIMEOUT_TYPE seconds_to_timeout(int sec)
{
  return ACE_INT64(sec) * 1000 * 1000 * 1000;
}

}

#endif
