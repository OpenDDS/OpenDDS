#include "value_annotationTypeSupportImpl.h"

#include <cstdlib>
#include <cstring>

using namespace OpenDDS::DCPS;

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  bool failed = false;

  if (static_cast<uint32_t>(TestValueAnnotation::NON_ZERO) != 99) {
    ACE_ERROR((LM_ERROR, "ERROR: NON_ZERO wasn't 99\n"));
    failed = true;
  }
  if (static_cast<uint32_t>(TestValueAnnotation::SIX) != 6) {
    ACE_ERROR((LM_ERROR, "ERROR: SIX wasn't 6\n"));
    failed = true;
  }
  if (static_cast<uint32_t>(TestValueAnnotation::TEN) != 10) {
    ACE_ERROR((LM_ERROR, "ERROR: TEN wasn't 10\n"));
    failed = true;
  }

  const EnumHelper& helper = *gen_TestValueAnnotation_helper;
  if (helper.valid("invalid")) {
    ACE_ERROR((LM_ERROR, "ERROR: invalid was valid\n"));
    failed = true;
  }
  if (helper.valid(0)) {
    ACE_ERROR((LM_ERROR, "ERROR: 0 was valid\n"));
    failed = true;
  }

  if (helper.valid("FIVE")) {
    if (helper.get_value("FIVE") != 5) {
      ACE_ERROR((LM_ERROR, "ERROR: FIVE's value wasn't 5\n"));
      failed = true;
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: FIVE was invalid\n"));
    failed = true;
  }

  if (helper.valid(5)) {
    if (std::strcmp(helper.get_name(5), "FIVE")) {
      ACE_ERROR((LM_ERROR, "ERROR: 5's name wasn't FIVE\n"));
      failed = true;
    }
  } else {
    ACE_ERROR((LM_ERROR, "ERROR: 5 was invalid\n"));
    failed = true;
  }

  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
