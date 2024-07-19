#include "Common.h"
#include "DynamicResponseTypeSupportImpl.h"
#include "DynamicResponseNotCompleteTypeSupportImpl.h"

#include <cstring>

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  Test t(ORIGIN);
  if (!t.init(argc, argv)) {
    return EXIT_FAILURE;
  }

  Topic<SimpleStruct> ss_topic(t);
  if (!ss_topic.init()) {
    return EXIT_FAILURE;
  }

  Topic<SimpleUnion> su_topic(t);
  if (!su_topic.init()) {
    return EXIT_FAILURE;
  }

  // This is just here to test that the responder can't get this type's
  // DynamicType because it's missing a complete TypeObject.
  Topic<SimpleStructNotComplete> ssnc_topic(t);
  if (!ssnc_topic.init()) {
    return EXIT_FAILURE;
  }

  t.post(TOPICS_CREATED);

  t.wait_for(RESPONDER, TOPICS_ANALYZED);

  if (!ss_topic.wait_dr_match(2)) {
    return EXIT_FAILURE;
  }

  if (!su_topic.wait_dr_match(2)) {
    return EXIT_FAILURE;
  }

  {
    SimpleStruct ss;
    ss.string_value = "initial";
    if (!ss_topic.write(ss)) {
      return EXIT_FAILURE;
    }

    SimpleUnion su;
    su.string_value("initial");
    if (!su_topic.write(su)) {
      return EXIT_FAILURE;
    }
  }

  t.post(ORIGIN_SAMPLES_AWAY);

  t.wait_for(RESPONDER, RESPONDER_SAMPLES_AWAY);

  SimpleStruct ss;
  if (!ss_topic.read_one(ss)) {
    return EXIT_FAILURE;
  }

  ACE_DEBUG((LM_DEBUG, "ss: string_value: %C\n", ss.string_value.in()));
  if (std::strcmp(ss.string_value.in(), "Hello struct")) {
    ACE_ERROR((LM_ERROR, "ERROR: Didn't get expected value from SimpleStruct\n"));
    t.exit_status = 1;
  }

  SimpleUnion su;
  if (!su_topic.read_one(su)) {
    return EXIT_FAILURE;
  }

  ACE_DEBUG((LM_DEBUG, "su: string_value: %C\n", su.string_value()));
  if (std::strcmp(su.string_value(), "Hello union")) {
    ACE_ERROR((LM_ERROR, "ERROR: Didn't get expected value from SimpleUnion\n"));
    t.exit_status = 1;
  }

  t.post(DONE);

  return t.exit_status;
}
