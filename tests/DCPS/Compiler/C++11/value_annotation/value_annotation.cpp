#include "value_annotationTypeSupportImpl.h"

#include <cstdlib>
#include <cstring>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

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

  const UsesAnnotatedEnum stru{TestValueAnnotation::SIX};
  const Encoding enc(Encoding::KIND_XCDR2, ENDIAN_LITTLE);
  const size_t size = serialized_size(enc, stru);
  static const char expected[] = {6, 0, 0, 0};

  Message_Block_Ptr mb(new ACE_Message_Block(size));
  Serializer ser(mb.get(), enc);
  if (!(ser << stru)) {
    ACE_ERROR((LM_ERROR, "ERROR: struct serialization failed\n"));
    failed = true;
  } else if (size != 4 || mb->length() != size) {
    ACE_ERROR((LM_ERROR, "ERROR: struct serialization size invalid\n"));
    failed = true;
  } else if (0 != std::memcmp(expected, mb->rd_ptr(), size)) {
    ACE_ERROR((LM_ERROR, "ERROR: struct serialization contents invalid\n"));
    failed = true;
  }

  mb->rd_ptr()[0] = 10;
  Serializer ser2(mb.get(), enc);
  UsesAnnotatedEnum stru2{TestValueAnnotation::SIX};
  if (!(ser2 >> stru2)) {
    ACE_ERROR((LM_ERROR, "ERROR: struct deserialization failed\n"));
    failed = true;
  } else if (stru2.tva() != TestValueAnnotation::TEN) {
    ACE_ERROR((LM_ERROR, "ERROR: struct deserialization result invalid\n"));
    failed = true;
  }

  const TypeIdentifier& enum_cti = getCompleteTypeIdentifier<TestValueAnnotation_xtag>();
  const TypeMap& enum_ctm = getCompleteTypeMap<TestValueAnnotation_xtag>();
  if (enum_ctm.count(enum_cti) == 0) {
    ACE_ERROR((LM_ERROR, "ERROR: XTypes TypeMap (enum) invalid\n"));
    failed = true;
  } else {
    const TypeObject& enum_cto = enum_ctm.find(enum_cti)->second;
    if (enum_cto.kind != EK_COMPLETE) {
      ACE_ERROR((LM_ERROR, "ERROR: XTypes TypeObject (enum) invalid\n"));
      failed = true;
    } else if (enum_cto.complete.kind != TK_ENUM) {
      ACE_ERROR((LM_ERROR, "ERROR: XTypes CompleteTypeObject (enum) invalid\n"));
      failed = true;
    } else {
      const CompleteEnumeratedLiteralSeq& enumerators = enum_cto.complete.enumerated_type.literal_seq;
      if (enumerators.length() != 6) {
        ACE_ERROR((LM_ERROR, "ERROR: XTypes invalid number of enumerators\n"));
        failed = true;
      } else if (enumerators[0].common.value != 1 ||
                 enumerators[1].common.value != 2 ||
                 enumerators[2].common.value != 5 ||
                 enumerators[3].common.value != 6 ||
                 enumerators[4].common.value != 10 ||
                 enumerators[5].common.value != 99) {
        ACE_ERROR((LM_ERROR, "ERROR: XTypes invalid values of enumerators\n"));
        failed = true;
      } else if (enumerators[0].detail.name != "ONE" ||
                 enumerators[1].detail.name != "TWO" ||
                 enumerators[2].detail.name != "FIVE" ||
                 enumerators[3].detail.name != "SIX" ||
                 enumerators[4].detail.name != "TEN" ||
                 enumerators[5].detail.name != "NON_ZERO") {
        ACE_ERROR((LM_ERROR, "ERROR: XTypes invalid names of enumerators\n"));
        failed = true;
      }
    }
  }

  const TypeIdentifier& union_cti = getCompleteTypeIdentifier<DiscByAnnotatedEnum_xtag>();
  const TypeMap& union_ctm = getCompleteTypeMap<DiscByAnnotatedEnum_xtag>();
  if (union_ctm.count(union_cti) == 0) {
    ACE_ERROR((LM_ERROR, "ERROR: XTypes TypeMap (union) invalid\n"));
    failed = true;
  } else {
    const TypeObject& union_cto = union_ctm.find(union_cti)->second;
    if (union_cto.kind != EK_COMPLETE) {
      ACE_ERROR((LM_ERROR, "ERROR: XTypes TypeObject (union) invalid\n"));
      failed = true;
    } else if (union_cto.complete.kind != TK_UNION) {
      ACE_ERROR((LM_ERROR, "ERROR: XTypes CompleteTypeObject (union) invalid\n"));
      failed = true;
    } else {
      const CompleteUnionMemberSeq& members = union_cto.complete.union_type.member_seq;
      if (members.length() != 2) {
        ACE_ERROR((LM_ERROR, "ERROR: XTypes invalid number of union members\n"));
        failed = true;
      } else if (members[0].common.label_seq.length() != 1 ||
                 members[0].common.label_seq[0] != 5) {
        ACE_ERROR((LM_ERROR, "ERROR: XTypes union member 0 invalid\n"));
        failed = true;
      } else if (members[1].common.label_seq.length() != 0 ||
                 (members[1].common.member_flags & IS_DEFAULT) != IS_DEFAULT) {
        ACE_ERROR((LM_ERROR, "ERROR: XTypes union member 1 invalid\n"));
        failed = true;
      }
    }
  }

  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
