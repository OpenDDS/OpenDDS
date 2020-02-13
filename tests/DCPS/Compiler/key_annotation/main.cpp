#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/FilterEvaluator.h>

#include <ace/ACE.h>
#include <ace/Log_Msg.h>

#include "key_annotationTypeSupportImpl.h"

using namespace key_annotation;

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
bool assert_key_count(size_t expected)
{
  size_t count = get_key_count<T>();
  if (count != expected) {
    const char* type_name = get_type_name<T>();
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("For DDSTraits<%C>::key_count(), expected %u key(s) but ")
      ACE_TEXT("got %u!\n"),
      type_name, expected, count));
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
bool assert_key_only_size(const T& data, size_t expected)
{
  typedef OpenDDS::DCPS::KeyOnly<const T> KeyOnlyType;
  KeyOnlyType key_only_data(data);

  size_t padding;
  size_t size = find_size<KeyOnlyType>(key_only_data, padding);
  if (size != expected) {
    const char* type_name = get_type_name<T>();
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("For gen_find_size(OpenDDS::DCPS::KeyOnly<%C>), expected %u ")
      ACE_TEXT("but got %u!\n"),
      type_name, expected, size));
    return true;
  }
  return false;
}

template <typename T>
class KeyCheck {
public:
  KeyCheck()
  : failed_(false)
  {
  }

  void add_key(const std::string& key_name)
  {
    if (!keys_.insert(key_name).second) {
      const char* type_name = get_type_name<T>();
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
        ACE_TEXT("For %C, %C is being inserted twice.\n"),
        type_name, key_name.c_str()));
      failed_ = true;
    }
  }

  bool failed()
  {
#ifndef OPENDDS_NO_MULTI_TOPIC
    for (Keys::const_iterator i = keys_.begin(); i != keys_.end(); ++i) {
      if (!OpenDDS::DCPS::getMetaStruct<T>().isDcpsKey(i->c_str())) {
        const char* type_name = get_type_name<T>();
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
          ACE_TEXT("For getMetaStruct<%C>().isDcpsKey(), expected %C to be a key, but it wasn't"),
          type_name, i->c_str()));
        failed_ = true;
      }
    }
#endif
    failed_ |= assert_key_count<T>(keys_.size());
    return failed_;
  }

private:
  typedef std::set<std::string> Keys;
  Keys keys_;
  bool failed_;
};

int ACE_TMAIN(int, ACE_TCHAR**)
{
  bool failed = false;

  {
    KeyCheck<UnkeyedStruct> c;
    failed |= c.failed();
  }
  {
    KeyCheck<SimpleKeyStruct> c;
    c.add_key("key");
    failed |= c.failed();
  }
  {
    KeyCheck<NestedKeyStruct> c;
    c.add_key("non_nested_key");
    c.add_key("nested_key.key");
    failed |= c.failed();
  }
  {
    KeyCheck<TypedefStructKeyStruct> c;
    c.add_key("a_key_value");
    c.add_key("my_struct_typedef_key.key");
    failed |= c.failed();
  }
  {
    KeyCheck<LongArrayStruct> c;
    c.add_key("values[0]");
    c.add_key("values[1]");
    failed |= c.failed();
  }
  {
    KeyCheck<SimpleKeyArray> c;
    c.add_key("values[0].key");
    c.add_key("values[1].key");
    failed |= c.failed();
  }
  failed |= assert_key_count<UnkeyedUnion>(0);
  failed |= assert_key_count<KeyedUnion>(1);
  {
    KeyCheck<KeyedUnionStruct> c;
    c.add_key("value");
    c.add_key("another_key");
    failed |= c.failed();
  }
  {
    KeyCheck<MultidimensionalArrayStruct> c;
    c.add_key("array1[0][0]");
    c.add_key("array1[0][1]");
    c.add_key("array1[0][2]");
    c.add_key("array1[1][0]");
    c.add_key("array1[1][1]");
    c.add_key("array1[1][2]");
    c.add_key("array2[0][0][0]");
    c.add_key("array2[0][0][1]");
    c.add_key("array2[0][0][2]");
    c.add_key("array2[0][0][3]");
    c.add_key("array2[0][1][0]");
    c.add_key("array2[0][1][1]");
    c.add_key("array2[0][1][2]");
    c.add_key("array2[0][1][3]");
    c.add_key("array2[0][2][0]");
    c.add_key("array2[0][2][1]");
    c.add_key("array2[0][2][2]");
    c.add_key("array2[0][2][3]");
    c.add_key("array2[1][0][0]");
    c.add_key("array2[1][0][1]");
    c.add_key("array2[1][0][2]");
    c.add_key("array2[1][0][3]");
    c.add_key("array2[1][1][0]");
    c.add_key("array2[1][1][1]");
    c.add_key("array2[1][1][2]");
    c.add_key("array2[1][1][3]");
    c.add_key("array2[1][2][0]");
    c.add_key("array2[1][2][1]");
    c.add_key("array2[1][2][2]");
    c.add_key("array2[1][2][3]");
    failed |= c.failed();
  }
  {
    KeyCheck<ImpliedKeys::StructA> c;
    c.add_key("nested_no_keys.a");
    c.add_key("nested_no_keys.b");
    c.add_key("nested_no_keys.c");
    c.add_key("nested_one_key.a");
    c.add_key("non_nested");
    failed |= c.failed();
  }
  {
    KeyCheck<ImpliedKeys::StructB> c;
    c.add_key("as_key.nested_no_keys.a");
    c.add_key("as_key.nested_no_keys.b");
    c.add_key("as_key.nested_no_keys.c");
    c.add_key("as_key.nested_one_key.a");
    c.add_key("as_key.non_nested");
    c.add_key("yet_another_key");
    failed |= c.failed();
  }
  {
    KeyCheck<ImpliedKeys::StructC> c;
    c.add_key("as_key.a");
    c.add_key("as_key.c");
    failed |= c.failed();
  }

  // Check KeyOnly for Unions
  failed |= assert_key_only_size(UnkeyedUnion(), 0);
  failed |= assert_key_only_size(KeyedUnion(), 4);
  failed |= assert_key_only_size(KeyedUnionStruct(), 8);

  return failed;
}
