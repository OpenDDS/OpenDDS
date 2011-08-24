/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/Definitions.h"
#include "../common/TestSupport.h"
#include "BoundTestTypeSupportImpl.h"
#include "BoundTest2TypeSupportImpl.h"

#include <iostream>

using namespace OpenDDS::DCPS;

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  {
    Bound::SimpleBoundedMessage message;

    TEST_CHECK(gen_is_bounded_size(message));
    TEST_CHECK(gen_max_marshaled_size(message) == 65);
  }

  {
    Bound::StringMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::WStringMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::SimpleBoundedArrayMessage message;

    TEST_CHECK(gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 650);
  }

  {
    Bound::StringArrayMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::WStringArrayMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::BoundedNestedMessage message;

    TEST_CHECK(gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 10);
  }

  {
    Bound::UnboundedNestedMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::BoundedSequenceOfBoundedMessage message;

    TEST_CHECK(gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 84);
  }

  {
    Bound::UnboundedSequenceOfBoundedMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::BoundedSequenceOfUnboundedMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::UnboundedSequenceOfUnboundedMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::BoundedUnionMessage message;

    TEST_CHECK(gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 6);
  }

  {
    Bound::UnboundedUnionMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  {
    Bound::RecursiveMessage message;

    TEST_CHECK(!gen_is_bounded_size(message));
    std::cout << gen_max_marshaled_size(message) << std::endl;
    TEST_CHECK(gen_max_marshaled_size(message) == 0);
  }

  return 0;
}
