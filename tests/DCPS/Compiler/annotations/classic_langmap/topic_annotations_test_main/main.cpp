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
      ACE_TEXT("For DDSTraits<%C>::key_count(), expected %d keys but ")
      ACE_TEXT("got %d!\n"),
      type_name, expected, count
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

template <typename Type>
bool assert_key_only_size(const Type& data, size_t expected) {
  typedef OpenDDS::DCPS::KeyOnly<const Type> KeyOnlyType;
  KeyOnlyType key_only_data(data);

  size_t padding;
  size_t size = find_size<KeyOnlyType>(key_only_data, padding);
  if (size != expected) {
    const char* type_name = get_type_name<Type>();
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("For gen_find_size(OpenDDS::DCPS::KeyOnly<%C>), expected %d but ")
      ACE_TEXT("got %d!\n"),
      type_name, expected, size
      ));
    return true;
  }
  return false;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  bool failed = false;

  // Check Key Counts
  failed |= assert_key_count<TopicAnnotationsTest::UnkeyedStruct>(0);
  failed |= assert_key_count<TopicAnnotationsTest::SimpleKeyStruct>(1);
  failed |= assert_key_count<TopicAnnotationsTest::NestedKeyStruct>(2);
  failed |= assert_key_count<TopicAnnotationsTest::LongArrayStruct>(2);
  failed |= assert_key_count<TopicAnnotationsTest::SimpleKeyArray>(2);
  failed |= assert_key_count<TopicAnnotationsTest::UnkeyedUnion>(0);
  failed |= assert_key_count<TopicAnnotationsTest::KeyedUnion>(1);
  failed |= assert_key_count<TopicAnnotationsTest::KeyedUnionStruct>(2);

  // Check KeyOnly for Unions
  {
    typedef TopicAnnotationsTest::UnkeyedUnion Data;
    Data data;
    failed |= assert_key_only_size(data, 0);
  }

  {
    typedef TopicAnnotationsTest::KeyedUnion Data;
    Data data;
    failed |= assert_key_only_size(data, 4);
  }

  {
    typedef TopicAnnotationsTest::KeyedUnionStruct Data;
    Data data;
    failed |= assert_key_only_size(data, 8);
  }

  return failed;
}
