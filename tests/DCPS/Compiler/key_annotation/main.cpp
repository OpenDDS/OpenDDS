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
      ACE_TEXT("For DDSTraits<%C>::key_count(), expected %u keys but ")
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

template <typename Type>
bool assert_key_only_size(const Type& data, size_t expected)
{
  typedef OpenDDS::DCPS::KeyOnly<const Type> KeyOnlyType;
  KeyOnlyType key_only_data(data);

  size_t padding;
  size_t size = find_size<KeyOnlyType>(key_only_data, padding);
  if (size != expected) {
    const char* type_name = get_type_name<Type>();
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ")
      ACE_TEXT("For gen_find_size(OpenDDS::DCPS::KeyOnly<%C>), expected %u ")
      ACE_TEXT("but got %u!\n"),
      type_name, expected, size));
    return true;
  }
  return false;
}

int ACE_TMAIN(int, ACE_TCHAR**)
{
  bool failed = false;

  // Check Key Counts
  failed |= assert_key_count<UnkeyedStruct>(0);
  failed |= assert_key_count<SimpleKeyStruct>(1);
  failed |= assert_key_count<NestedKeyStruct>(2);
  failed |= assert_key_count<TypedefStructKeyStruct>(2);
  failed |= assert_key_count<LongArrayStruct>(2);
  failed |= assert_key_count<SimpleKeyArray>(2);
  failed |= assert_key_count<UnkeyedUnion>(0);
  failed |= assert_key_count<KeyedUnion>(1);
  failed |= assert_key_count<KeyedUnionStruct>(2);
  failed |= assert_key_count<MultidimensionalArrayStruct>(2 * 3 + 2 * 3 * 4);
  failed |= assert_key_count<ImpliedKeys::StructA>(5);
  failed |= assert_key_count<ImpliedKeys::StructB>(6);
  failed |= assert_key_count<ImpliedKeys::StructC>(2);

  // Check KeyOnly for Unions
  failed |= assert_key_only_size(UnkeyedUnion(), 0);
  failed |= assert_key_only_size(KeyedUnion(), 4);
  failed |= assert_key_only_size(KeyedUnionStruct(), 8);

  return failed;
}
