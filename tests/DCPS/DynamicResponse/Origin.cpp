#include "Common.h"
#include "DynamicResponseTypeSupportImpl.h"
#include "DynamicResponseNotCompleteTypeSupportImpl.h"

#include <cstring>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  Test t("origin");
  if (!t.init(argc, argv)) {
    return 1;
  }

  Topic<SimpleStruct> ss_topic(t);
  if (!ss_topic.init()) {
    return 1;
  }

  Topic<SimpleUnion> su_topic(t);
  if (!su_topic.init()) {
    return 1;
  }

  // This is just here to test that the responder can't get this type's
  // DynamicType because it's missing a complete TypeObject.
  Topic<SimpleStructNotComplete> ssnc_topic(t);
  if (!ssnc_topic.init()) {
    return 1;
  }

  t.post("ready");

  // TODO: Write samples to be changed by responder

  t.wait_for("responder", "done");

  SimpleStruct ss;
  if (ss_topic.read_one(ss)) {
    ACE_DEBUG((LM_DEBUG, "ss: string_value: %C\n", ss.string_value.in()));
    if (std::strcmp(ss.string_value.in(), "Hello struct")) {
      ACE_ERROR((LM_ERROR, "ERROR: Didn't get expected value from SimpleStruct\n"));
      t.exit_status = 1;
    }
  }

  SimpleUnion su;
  if (su_topic.read_one(su)) {
    ACE_DEBUG((LM_DEBUG, "su: string_value: %C\n", ss.string_value.in()));
    if (std::strcmp(su.string_value(), "Hello union")) {
      ACE_ERROR((LM_ERROR, "ERROR: Didn't get expected value from SimpleUnion\n"));
      t.exit_status = 1;
    }
  }

  t.post("done");

  return t.exit_status;
}
