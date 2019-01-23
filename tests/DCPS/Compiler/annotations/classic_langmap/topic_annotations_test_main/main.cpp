#include "topic_annotations_testTypeSupportImpl.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dds/DCPS/FilterEvaluator.h"

template <typename T>
OpenDDS::DCPS::DDSTraits<T>& get_traits()
{
  static OpenDDS::DCPS::DDSTraits<T> traits;
  return traits;
}

template <typename T>
const char* get_type_name()
{
  return get_traits<T>().type_name();
}

template <typename T>
size_t get_key_count()
{
  return get_traits<T>().key_count();
}

template <typename T>
bool assert_key_count(size_t expected) {
  size_t count = get_key_count<T>();
  if (count != expected) {
    const char* type_name = get_type_name<T>();
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("For %C, expected %d keys but ")
      ACE_TEXT("OpenDDS::DCPS::DDSTraits<%C>::key_count() returned %d!\n"),
      type_name, expected, type_name, count
      ));
    return true;
  }
  return false;
}

template <typename T>
size_t find_size(const T& data, size_t& padding)
{
  size_t size = 0;
  padding = 0;
  OpenDDS::DCPS::gen_find_size(data, size, padding);
  return size;
}

template <typename T>
bool assert_size(const T& data, size_t expected) {
  size_t padding;
  size_t size = find_size<T>(data, padding);
  if (size != expected) {
    const char* type_name = get_type_name<T>();
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("For KeyOnly<%C> size, expected %d but ")
      ACE_TEXT("OpenDDS::DCPS::genfind_size() returned %d!\n"),
      type_name, expected, size
      ));
    return true;
  }
  return false;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  using TopicAnnotationsTest;

  bool failed = false;

  // Check Key Counts
  failed |= assert_key_count<UnkeyedStruct>(0);
  failed |= assert_key_count<SimpleKeyStruct>(1);
  failed |= assert_key_count<NestedKeyStruct>(2);
  failed |= assert_key_count<LongArrayStruct>(2);
  failed |= assert_key_count<SimpleKeyArray>(2);
  failed |= assert_key_count<UnkeyedUnion>(0);
  failed |= assert_key_count<KeyedUnion>(1);
  failed |= assert_key_count<KeyedUnionStruct>(2);

  // Check KeyOnly for Unions
  OpenDDS::DCPS::KeyOnly<UnkeyedUnion> key_only_unkeyed_union;
  failed |= assery_size(key_only_unkeyed_union, 0);

  OpenDDS::DCPS::KeyOnly<KeyedUnion> key_only_keyed_union;
  failed |= assery_size(key_only_keyed_union, 4);

  OpenDDS::DCPS::KeyOnly<KeyedUnionStruct> key_only_keyed_union_struct;
  failed |= assery_size(key_only_keyed_union_struct, 8);

  return failed;
}
