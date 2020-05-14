#include <dds/DCPS/Serializer.h>

const OpenDDS::DCPS::Encoding encodings[] = {
  OpenDDS::DCPS::Encoding(
    OpenDDS::DCPS::Encoding::KIND_CDR_UNALIGNED, OpenDDS::DCPS::ENDIAN_LITTLE),
  OpenDDS::DCPS::Encoding(
    OpenDDS::DCPS::Encoding::KIND_CDR_UNALIGNED, OpenDDS::DCPS::ENDIAN_BIG),
  OpenDDS::DCPS::Encoding(
    OpenDDS::DCPS::Encoding::KIND_CDR_PLAIN, OpenDDS::DCPS::ENDIAN_LITTLE),
  OpenDDS::DCPS::Encoding(
    OpenDDS::DCPS::Encoding::KIND_CDR_PLAIN, OpenDDS::DCPS::ENDIAN_BIG),
  OpenDDS::DCPS::Encoding(
    OpenDDS::DCPS::Encoding::KIND_XCDR2_PLAIN, OpenDDS::DCPS::ENDIAN_LITTLE),
  OpenDDS::DCPS::Encoding(
    OpenDDS::DCPS::Encoding::KIND_XCDR2_PLAIN, OpenDDS::DCPS::ENDIAN_BIG),
};
const size_t encoding_count = sizeof(encodings) / sizeof(encodings[0]);

bool runAlignmentTest();
bool runAlignmentResetTest();
bool runAlignmentOverrunTest();
