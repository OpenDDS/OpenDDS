/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ace/OS_main.h"

#include "dds/DCPS/Definitions.h"

#include "../common/TestSupport.h"
#include "KeyTestTypeSupportImpl.h"

#include <iostream>

using namespace OpenDDS::DCPS;


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

    // Use Key-Only marshaling (no keys means it should be empty)
    ACE_Message_Block* mb = new ACE_Message_Block(4096);
    ::OpenDDS::DCPS::Serializer out_serializer (mb);
    out_serializer << KeyOnly<const Messenger1::Message>(message);
    size_t key_length = mb->length();
    TEST_CHECK(key_length == 0);
    ::OpenDDS::DCPS::Serializer in_serializer (mb);
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
      ACE_Message_Block* mb = new ACE_Message_Block(4096);
      ::OpenDDS::DCPS::Serializer out_serializer (mb);
      out_serializer << message;
      full_length = mb->length();
      ::OpenDDS::DCPS::Serializer in_serializer (mb);
      Messenger2::Message dm_message;
      in_serializer >> dm_message;
      TEST_CHECK(strcmp(message.from, dm_message.from) == 0);
      TEST_CHECK(strcmp(message.subject, dm_message.subject) == 0);
      TEST_CHECK(message.subject_id == dm_message.subject_id);
      TEST_CHECK(strcmp(message.text, dm_message.text) == 0);
      TEST_CHECK(message.count == dm_message.count);
    }

    {
      // Use Key-Only marshaling (only the key fields should be equal)
      ACE_Message_Block* mb = new ACE_Message_Block(4096);
      ::OpenDDS::DCPS::Serializer out_serializer (mb);
      const Messenger2::Message& mess_const_ref = message;
      out_serializer << KeyOnly<const Messenger2::Message>(mess_const_ref);
      key_length = mb->length();
      ::OpenDDS::DCPS::Serializer in_serializer (mb);
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
    message.float_field = 123.45;
    message.double_field = 123.456;
    message.long_double_field = 123.4567;
    message.boolean_field = true;
    message.octet_field = 12;
    message.enum_field = Messenger4::SECOND;
    message.string_field = "cromulent";
    const CORBA::WChar wstring_value[4] = { 'X', 'Y', 'Z', 0 };
    message.wstring_field = wstring_value;

    // Use Key-Only marshaling (only the key fields should be equal)
    ACE_Message_Block* mb = new ACE_Message_Block(4096);
    ::OpenDDS::DCPS::Serializer out_serializer (mb);
    out_serializer << KeyOnly<const Messenger4::Message>(message);
    ::OpenDDS::DCPS::Serializer in_serializer (mb);
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
    dm_message.long_double_field = 0;
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
    message.mess.float_field = 123.45;
    message.mess.double_field = 123.456;
    message.mess.long_double_field = 123.4567;
    message.mess.boolean_field = true;
    message.mess.octet_field = 12;
    message.mess.enum_field = Messenger4::SECOND;
    message.mess.string_field = "cromulent";
    const CORBA::WChar wstring_value[4] = { 'X', 'Y', 'Z', 0 };
    message.mess.wstring_field = wstring_value;

    // Use Key-Only marshaling (only the key fields should be equal)
    ACE_Message_Block* mb = new ACE_Message_Block(4096);
    ::OpenDDS::DCPS::Serializer out_serializer (mb);
    out_serializer << KeyOnly<const Messenger4::NestedMessage>(message);
    ::OpenDDS::DCPS::Serializer in_serializer (mb);
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
    dm_message.mess.long_double_field = 0;
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

    // Use Key-Only marshaling (only the key fields should be equal)
    ACE_Message_Block* mb = new ACE_Message_Block(4096);
    ::OpenDDS::DCPS::Serializer out_serializer (mb);
    out_serializer << KeyOnly<const Messenger7::Message>(message);
    ::OpenDDS::DCPS::Serializer in_serializer (mb);
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

    // Use Key-Only marshaling (only the key fields should be equal)
    ACE_Message_Block* mb = new ACE_Message_Block(4096);
    ::OpenDDS::DCPS::Serializer out_serializer (mb);
    out_serializer << KeyOnly<const Messenger9::Message>(message);
    ::OpenDDS::DCPS::Serializer in_serializer (mb);
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

  return 0;
}
