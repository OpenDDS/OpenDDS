#include <no_init_before_deserializeTypeSupportImpl.h>

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/Message_Block_Ptr.h>

#include <ace/OS_main.h>

#include <iostream>

using namespace OpenDDS::DCPS;

#if CPP11_MAPPING
#  define SEQ_LEN_TYPE size_t
#  define SEQ_LEN(seq) seq.size()
#  define SEQ_RESIZE(seq, n) seq.resize(n)
#  define FIELD(WHAT) WHAT()
#else
#  define SEQ_LEN_TYPE DDS::UInt32
#  define SEQ_LEN(seq) seq.length()
#  define SEQ_RESIZE(seq, n) seq.length(n)
#  define FIELD(WHAT) WHAT
#endif

const SEQ_LEN_TYPE seq_len = 250;
const SEQ_LEN_TYPE bounded_seq_len = 5;

template <typename Seq>
void fill_seq(Seq& seq, SEQ_LEN_TYPE len)
{
  SEQ_RESIZE(seq, len);
  for (SEQ_LEN_TYPE i = 0; i < len; ++i) {
    seq[i] = static_cast<float>(i);
  }
}

bool success = true;

template <typename Seq>
void verify_seq(const char* name, const Seq& seq, SEQ_LEN_TYPE len)
{
  const SEQ_LEN_TYPE actual_len = SEQ_LEN(seq);
  if (actual_len != len) {
    std::cerr << "ERROR: For " << name << " expected len " << len << " but got " << actual_len << std::endl;
    success = false;
    return;
  }
  for (SEQ_LEN_TYPE i = 0; i < len; ++i) {
    const float actual = seq[i];
    if (actual != static_cast<float>(i)) {
      std::cerr << "ERROR: For " << name << "[" << i << "] expected " << i << " but got " << actual << std::endl;
      success = false;
      return;
    }
  }
}

#if CPP11_MAPPING
template <typename FromSeq, typename ToSeq = std::vector<float>>
void copy_vector(const FromSeq& from_seq)
{
  const ToSeq copy = from_seq;
  ACE_UNUSED_ARG(copy);
}

template <typename Seq>
void copy_no_init_vector(const Seq& seq)
{
  copy_vector<Seq, OptionalInitVector<float>>(seq);

  // Can't directly copy to normal std::vector
  std::vector<float> copy;
  copy.assign(seq.begin(), seq.end());
}
#endif

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  NoInitBeforeDeserializeStruct original;
  fill_seq(FIELD(original.float_seq), seq_len);
  fill_seq(FIELD(original.no_init_float_seq), seq_len);
  fill_seq(FIELD(original.bounded_float_seq), bounded_seq_len);
  fill_seq(FIELD(original.bounded_no_init_float_seq), bounded_seq_len);
  fill_seq(FIELD(original.anon_float_seq), seq_len);
  fill_seq(FIELD(original.anon_no_init_float_seq), seq_len);
  fill_seq(FIELD(original.anon_bounded_float_seq), bounded_seq_len);
  fill_seq(FIELD(original.anon_bounded_no_init_float_seq), bounded_seq_len);

  const Encoding encoding(Encoding::KIND_XCDR2);
  Message_Block_Ptr buffer(new ACE_Message_Block(serialized_size(encoding, original)));
  {
    Serializer serializer(buffer.get(), encoding);
    if (!(serializer << original)) {
      ACE_ERROR((LM_ERROR, "ERROR: Serialization failed\n"));
      return 1;
    }
  }

  NoInitBeforeDeserializeStruct deserialized;
  Serializer serializer(buffer.get(), encoding);
  if (!(serializer >> deserialized)) {
    ACE_ERROR((LM_ERROR, "ERROR: Deserialization failed\n"));
    return 1;
  }

  verify_seq("float_seq", FIELD(deserialized.float_seq), seq_len);
  verify_seq("no_init_float_seq", FIELD(deserialized.no_init_float_seq), seq_len);
  verify_seq("bounded_float_seq", FIELD(deserialized.bounded_float_seq), bounded_seq_len);
  verify_seq("bounded_no_init_float_seq", FIELD(deserialized.bounded_no_init_float_seq), bounded_seq_len);
  verify_seq("anon_float_seq", FIELD(deserialized.anon_float_seq), seq_len);
  verify_seq("anon_no_init_float_seq", FIELD(deserialized.anon_no_init_float_seq), seq_len);
  verify_seq("anon_bounded_float_seq", FIELD(deserialized.anon_bounded_float_seq), bounded_seq_len);
  verify_seq("anon_bounded_no_init_float_seq", FIELD(deserialized.anon_bounded_no_init_float_seq), bounded_seq_len);

#if CPP11_MAPPING
  // Make sure we can copy to the correct counterparts
  copy_vector(deserialized.float_seq());
  copy_no_init_vector(deserialized.no_init_float_seq());
  copy_vector(deserialized.bounded_float_seq());
  copy_no_init_vector(deserialized.bounded_no_init_float_seq());
  copy_vector(deserialized.anon_float_seq());
  copy_no_init_vector(deserialized.anon_no_init_float_seq());
  copy_vector(deserialized.anon_bounded_float_seq());
  copy_no_init_vector(deserialized.anon_bounded_no_init_float_seq());
#endif

  return success ? 0 : 1;
}
