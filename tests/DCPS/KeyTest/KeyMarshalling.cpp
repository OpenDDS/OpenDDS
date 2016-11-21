/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "../common/TestSupport.h"
#include "KeyTestTypeSupportImpl.h"
#include "KeyTest2TypeSupportImpl.h"

#include <iostream>

using namespace OpenDDS::DCPS;

void print_hex(void* d)
{
  unsigned char* a = (unsigned char*) d;
  printf("0x");
  for (int j=0;j<16; j++){
    printf("%02x",a[j]);
  }
  printf("\n");
}


int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  {
    Messenger1::Message message;  // no key
    message.from = "from";
    message.subject = "subject";
    message.subject_id = 1234;
    message.text = "text";
    message.count = 4321;

    // Use Key-Only marshalling (no keys means it should be empty)
    KeyOnly<const Messenger1::Message> ko_message(message);
    size_t size = 0, padding = 0;
    gen_find_size(ko_message, size, padding);
    ACE_Message_Block mb(size);
    Serializer out_serializer(&mb);
    out_serializer << ko_message;
    size_t key_length = mb.length();
    TEST_CHECK(key_length == 0);
    Serializer in_serializer(&mb);
    Messenger1::Message dm_message;
    dm_message.subject_id=0;  // avoid unitialized data
    dm_message.count=0;
    in_serializer >> KeyOnly<Messenger1::Message>(dm_message);
    TEST_CHECK(strcmp(message.from, dm_message.from) != 0);
    TEST_CHECK(strcmp(message.subject, dm_message.subject) != 0);
    TEST_CHECK(message.subject_id != dm_message.subject_id);
    TEST_CHECK(strcmp(message.text, dm_message.text) != 0);
    TEST_CHECK(message.count != dm_message.count);
  }

  {
    Messenger2::Message message;
    message.from = "from";
    message.subject = "subject";
    message.subject_id = 1234;   // key
    message.text = "text";
    message.count = 4321;

    size_t full_length = 0;
    size_t key_length = 0;
    {
      // Use normal messaging (all fields should be equal).
      size_t size = 0, padding = 0;
      gen_find_size(message, size, padding);
      ACE_Message_Block mb(size);
      Serializer out_serializer(&mb);
      out_serializer << message;
      full_length = mb.length();
      Serializer in_serializer(&mb);
      Messenger2::Message dm_message;
      in_serializer >> dm_message;
      TEST_CHECK(strcmp(message.from, dm_message.from) == 0);
      TEST_CHECK(strcmp(message.subject, dm_message.subject) == 0);
      TEST_CHECK(message.subject_id == dm_message.subject_id);
      TEST_CHECK(strcmp(message.text, dm_message.text) == 0);
      TEST_CHECK(message.count == dm_message.count);
    }

    {
      // Use Key-Only marshalling (only the key fields should be equal)
      const Messenger2::Message& mess_const_ref = message;
      KeyOnly<const Messenger2::Message> ko_message(mess_const_ref);
      size_t size = 0, padding = 0;
      gen_find_size(ko_message, size, padding);
      ACE_Message_Block mb(size);
      ::OpenDDS::DCPS::Serializer out_serializer(&mb);
      out_serializer << ko_message;
      key_length = mb.length();
      ::OpenDDS::DCPS::Serializer in_serializer(&mb);
      Messenger2::Message dm_message;
      dm_message.subject_id=0;  // avoid unitialized data
      dm_message.count=0;
      in_serializer >> KeyOnly<Messenger2::Message>(dm_message);
      TEST_CHECK(strcmp(message.from, dm_message.from) != 0);
      TEST_CHECK(strcmp(message.subject, dm_message.subject) != 0);
      TEST_CHECK(message.subject_id == dm_message.subject_id);
      TEST_CHECK(strcmp(message.text, dm_message.text) != 0);
      TEST_CHECK(message.count != dm_message.count);
    }
    TEST_CHECK(key_length < full_length);
  }

  {
    Messenger4::Message message;
    message.short_field = -123;
    message.unsigned_short_field = 123;
    message.long_field = -1234;
    message.unsigned_long_field = 1234;
    message.long_long_field = -12345;
    message.unsigned_long_long_field = 12345;
    message.char_field = 'x';
    message.wchar_field = 'x';
    message.float_field = 123.45f;
    message.double_field = 123.456;
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(message.long_double_field, 123.4567L);
    message.boolean_field = true;
    message.octet_field = 12;
    message.enum_field = Messenger4::SECOND;
    message.string_field = "cromulent";
    const CORBA::WChar wstring_value[4] = { 'X', 'Y', 'Z', 0 };
    message.wstring_field = wstring_value;

    // Use Key-Only marshalling (only the key fields should be equal)
    KeyOnly<const Messenger4::Message> ko_message(message);
    size_t size = 0, padding = 0;
    gen_find_size(ko_message, size, padding);
    ACE_Message_Block mb(size);
    ::OpenDDS::DCPS::Serializer out_serializer(&mb);
    out_serializer << ko_message;
    ::OpenDDS::DCPS::Serializer in_serializer(&mb);
    Messenger4::Message dm_message;
    dm_message.short_field = 0;
    dm_message.unsigned_short_field = 0;
    dm_message.long_field = 0;
    dm_message.unsigned_long_field = 0;
    dm_message.long_long_field = 0;
    dm_message.unsigned_long_long_field = 0;
    dm_message.char_field = 0;
    dm_message.wchar_field = 0;
    dm_message.float_field = 0;
    dm_message.double_field = 0;
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(dm_message.long_double_field,0.0L);
    dm_message.boolean_field = 0;
    dm_message.octet_field = 0;
    dm_message.enum_field = Messenger4::FIRST;
    in_serializer >> KeyOnly<Messenger4::Message>(dm_message);
    TEST_CHECK(message.short_field == dm_message.short_field);
    TEST_CHECK(message.unsigned_short_field == dm_message.unsigned_short_field);
    TEST_CHECK(message.long_field == dm_message.long_field);
    TEST_CHECK(message.unsigned_long_field == dm_message.unsigned_long_field);
    TEST_CHECK(message.long_long_field == dm_message.long_long_field);
    TEST_CHECK(message.unsigned_long_long_field == dm_message.unsigned_long_long_field);
    TEST_CHECK(message.char_field == dm_message.char_field);
    TEST_CHECK(message.wchar_field == dm_message.wchar_field);
    TEST_CHECK(message.float_field == dm_message.float_field);
    TEST_CHECK(message.double_field == dm_message.double_field);
    TEST_CHECK(message.long_double_field == dm_message.long_double_field);
    TEST_CHECK(message.boolean_field == dm_message.boolean_field);
    TEST_CHECK(message.octet_field == dm_message.octet_field);
    TEST_CHECK(message.enum_field == dm_message.enum_field);
    TEST_CHECK(strcmp(message.string_field, dm_message.string_field) == 0);
    TEST_CHECK(ACE_OS::strcmp(message.wstring_field, dm_message.wstring_field) == 0);
  }

  {
    Messenger4::NestedMessage message;
    message.mess.short_field = -123;
    message.mess.unsigned_short_field = 123;
    message.mess.long_field = -1234;
    message.mess.unsigned_long_field = 1234;
    message.mess.long_long_field = -12345;
    message.mess.unsigned_long_long_field = 12345;
    message.mess.char_field = 'x';
    message.mess.wchar_field = 'x';
    message.mess.float_field = 123.45f;
    message.mess.double_field = 123.456;
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(message.mess.long_double_field,123.4567L);
    message.mess.boolean_field = true;
    message.mess.octet_field = 12;
    message.mess.enum_field = Messenger4::SECOND;
    message.mess.string_field = "cromulent";
    const CORBA::WChar wstring_value[4] = { 'X', 'Y', 'Z', 0 };
    message.mess.wstring_field = wstring_value;

    // Use Key-Only marshalling (only the key fields should be equal)
    KeyOnly<const Messenger4::NestedMessage> ko_message(message);
    size_t size = 0, padding = 0;
    gen_find_size(ko_message, size, padding);
    ACE_Message_Block mb(size);
    ::OpenDDS::DCPS::Serializer out_serializer(&mb);
    out_serializer << ko_message;
    ::OpenDDS::DCPS::Serializer in_serializer(&mb);
    Messenger4::NestedMessage dm_message;
    dm_message.mess.short_field = 0;
    dm_message.mess.unsigned_short_field = 0;
    dm_message.mess.long_field = 0;
    dm_message.mess.unsigned_long_field = 0;
    dm_message.mess.long_long_field = 0;
    dm_message.mess.unsigned_long_long_field = 0;
    dm_message.mess.char_field = 0;
    dm_message.mess.wchar_field = 0;
    dm_message.mess.float_field = 0;
    dm_message.mess.double_field = 0;
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(dm_message.mess.long_double_field,0.0L);
    dm_message.mess.boolean_field = 0;
    dm_message.mess.octet_field = 0;
    dm_message.mess.enum_field = Messenger4::FIRST;
    in_serializer >> KeyOnly<Messenger4::NestedMessage>(dm_message);
    TEST_CHECK(message.mess.short_field == dm_message.mess.short_field);
    TEST_CHECK(message.mess.unsigned_short_field == dm_message.mess.unsigned_short_field);
    TEST_CHECK(message.mess.long_field == dm_message.mess.long_field);
    TEST_CHECK(message.mess.unsigned_long_field == dm_message.mess.unsigned_long_field);
    TEST_CHECK(message.mess.long_long_field == dm_message.mess.long_long_field);
    TEST_CHECK(message.mess.unsigned_long_long_field == dm_message.mess.unsigned_long_long_field);
    TEST_CHECK(message.mess.char_field == dm_message.mess.char_field);
    TEST_CHECK(message.mess.wchar_field == dm_message.mess.wchar_field);
    TEST_CHECK(message.mess.float_field == dm_message.mess.float_field);
    TEST_CHECK(message.mess.double_field == dm_message.mess.double_field);
    TEST_CHECK(message.mess.long_double_field == dm_message.mess.long_double_field);
    TEST_CHECK(message.mess.boolean_field == dm_message.mess.boolean_field);
    TEST_CHECK(message.mess.octet_field == dm_message.mess.octet_field);
    TEST_CHECK(message.mess.enum_field == dm_message.mess.enum_field);
    TEST_CHECK(strcmp(message.mess.string_field, dm_message.mess.string_field) == 0);
    TEST_CHECK(ACE_OS::strcmp(message.mess.wstring_field, dm_message.mess.wstring_field) == 0);
  }

  {
    Messenger7::Message message;
    message.header.from = "from";
    message.header.subject = "subject";
    message.header.subject_id = 1234;
    message.text = "text";
    message.count = 4321;
    message.responses[0] = 1;   // key
    message.responses[1] = 2;
    message.responses[2] = 3;

    // Use Key-Only marshalling (only the key fields should be equal)
    KeyOnly<const Messenger7::Message> ko_message(message);
    size_t size = 0, padding = 0;
    gen_find_size(ko_message, size, padding);
    ACE_Message_Block mb(size);
    ::OpenDDS::DCPS::Serializer out_serializer(&mb);
    out_serializer << ko_message;
    ::OpenDDS::DCPS::Serializer in_serializer(&mb);
    Messenger7::Message dm_message;
    dm_message.header.subject_id = 0;
    dm_message.count = 0;
    dm_message.responses[0] = 0;
    dm_message.responses[1] = 0;
    dm_message.responses[2] = 0;
    in_serializer >> KeyOnly<Messenger7::Message>(dm_message);
    TEST_CHECK(strcmp(message.header.from, dm_message.header.from) != 0);
    TEST_CHECK(strcmp(message.header.subject, dm_message.header.subject) != 0);
    TEST_CHECK(message.header.subject_id != dm_message.header.subject_id);
    TEST_CHECK(strcmp(message.text, dm_message.text) != 0);
    TEST_CHECK(message.count != dm_message.count);
    TEST_CHECK(message.responses[0] == dm_message.responses[0]);
    TEST_CHECK(message.responses[1] != dm_message.responses[1]);
    TEST_CHECK(message.responses[2] != dm_message.responses[2]);
  }

  {
    Messenger9::Message message;
    message.headers[0].from = "from";
    message.headers[0].subject = "subject";
    message.headers[0].subject_id = 1234;
    message.headers[1].from = "from-1";
    message.headers[1].subject = "subject-1";
    message.headers[1].subject_id = 12345;    //key
    message.text = "text";
    message.count = 4321;

    // Use Key-Only marshalling (only the key fields should be equal)
    KeyOnly<const Messenger9::Message> ko_message(message);
    size_t size = 0, padding = 0;
    gen_find_size(ko_message, size, padding);
    ACE_Message_Block mb(size);
    ::OpenDDS::DCPS::Serializer out_serializer(&mb);
    out_serializer << ko_message;
    ::OpenDDS::DCPS::Serializer in_serializer(&mb);
    Messenger9::Message dm_message;
    dm_message.headers[0].subject_id = 0;
    dm_message.headers[1].subject_id = 0;
    dm_message.count = 0;
    in_serializer >> KeyOnly<Messenger9::Message>(dm_message);
    TEST_CHECK(strcmp(message.headers[0].from, dm_message.headers[0].from) != 0);
    TEST_CHECK(strcmp(message.headers[0].subject, dm_message.headers[0].subject) != 0);
    TEST_CHECK(message.headers[0].subject_id != dm_message.headers[0].subject_id);
    TEST_CHECK(strcmp(message.headers[1].from, dm_message.headers[1].from) != 0);
    TEST_CHECK(strcmp(message.headers[1].subject, dm_message.headers[1].subject) != 0);
    TEST_CHECK(message.headers[1].subject_id == dm_message.headers[1].subject_id);
    TEST_CHECK(strcmp(message.text, dm_message.text) != 0);
    TEST_CHECK(message.count != dm_message.count);
  }

  // The following tests exercise the KeyHash related functionality using
  // a variety of different key types
  {
    Messenger1::Message message;  // No Key, length = 0, bounded = true
    std::cout << "Messenger1::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger1::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger1::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 0);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger2::Message message;  // long Key, length = 4, bounded = true
    message.subject_id = 0x01020304;
    std::cout << "Messenger2::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger2::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger2::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 4);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger3::Message message;  // two long Keys, length = 8, bounded = true
    message.subject_id = 0x01020304;
    message.count      = 0x05060708;
    std::cout << "Messenger3::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger3::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger3::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 8);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger4::Message message;  // Keys of many types, length = 0, bounded = false
    std::cout << "Messenger4::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger4::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger4::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(!is_bounded);
    TEST_CHECK(length == 0);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger5::Message message;  // Wide string Key, length = 0, bounded = false
    std::cout << "Messenger5::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger5::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger5::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(!is_bounded);
    TEST_CHECK(length == 0);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger6::Message message;  // long Key, length = 4, bounded = true
    message.payload.header.subject_id = 0x01020304;

    std::cout << "Messenger6::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger6::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger6::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 4);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger7::Message message;  // Long Key, length = 4, bounded = true
    message.responses[0] = 0x01020304;
    std::cout << "Messenger7::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger7::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger7::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 4);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger8::Message message;  // Long Key, length = 4, bounded = true
    message.header.responses[0] = 0x01020304;
    std::cout << "Messenger8::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger8::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger8::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 4);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger9::Message message;  // Long Key, length = 4, bounded = true
    message.headers[1].subject_id = 0x01020304;
    std::cout << "Messenger9::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger9::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger9::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 4);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger10::Message message;  // String and long Keys, length = 0, bounded = false
    message.count = 0x01020304;
    std::cout << "Messenger10::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger10::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger10::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(!is_bounded);
    TEST_CHECK(length == 0);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger11::Message message;  // Four long Keys, length = 16, bounded = true
    message.long_1 = 0x01020304;
    message.long_2 = 0x05060708;
    message.long_3 = 0x090a0b0c;
    message.long_4 = 0x0d0e0f10;
    std::cout << "Messenger11::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger11::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger11::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 16);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  {
    Messenger12::Message message;  // Five long Keys, length = 20, bounded = true
    message.long_1 = 0x01020304;
    message.long_2 = 0x05060708;
    message.long_3 = 0x090a0b0c;
    message.long_4 = 0x0d0e0f10;
    message.long_5 = 0x11121314;
    std::cout << "Messenger12::Message" << std::endl;
    size_t length = gen_max_marshaled_size(KeyOnly<const Messenger12::Message>(message), false /*align*/);
    bool is_bounded = MarshalTraits<Messenger12::Message>::gen_is_bounded_key_size();
    std::cout << "  is bounded = " << is_bounded << std::endl;
    std::cout << "  length = " << length << std::endl;
    TEST_CHECK(is_bounded);
    TEST_CHECK(length == 20);

    OpenDDS::RTPS::KeyHash_t hash;
    OpenDDS::RTPS::marshal_key_hash(message, hash);
    print_hex(hash.value);
  }

  return 0;
}
