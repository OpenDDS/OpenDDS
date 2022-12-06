#include "Common.h"
#include "DynamicResponseTypeSupportImpl.h"
#include "DynamicResponseNotCompleteTypeSupportImpl.h"

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

  t.wait_for("responder", "done");

  SimpleStruct ss;
  if (ss_topic.read_one(ss)) {
    ACE_DEBUG((LM_DEBUG, "string_value: %C\n", ss.string_value.in()));
  }

  t.post("done");

  return 0;
}
