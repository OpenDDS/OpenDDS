#include "tao/String_Alloc.h"

#include "ace/OS_NS_string.h"
#include "ace/OS_NS_wchar.h"
#include "ace/OS_Memory.h"
// FUZZ: disable check_for_streams_include
#include "ace/streams.h"
#include "tao/Allocation_Macros.h"

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

char *
CORBA::string_dup (const char *str)
{
  if (!str)
    {
      errno = EINVAL;
      return 0;
    }

  size_t const len = ACE_OS::strlen (str);

  // This allocates an extra byte for the '\0';
  char * copy = CORBA::string_alloc (static_cast<CORBA::ULong> (len));

  if (copy != 0)
    {
      // The memcpy() assumes that the destination is a valid buffer.
      ACE_OS::memcpy (copy, str, len + 1);
    }

  return copy;
}

char *
CORBA::string_alloc (CORBA::ULong len)
{
  // Allocate 1 + strlen to accomodate the null terminating character.
  char *s = 0;
  ACE_ALLOCATOR_NEW (s,
                     char[size_t (len + 1)],
                     0);

  s[0]= '\0';

  return s;
}

void
CORBA::string_free (char *str)
{
  ACE_Allocator::instance ()->free (str);
}

// ****************************************************************

CORBA::WChar*
CORBA::wstring_dup (const WChar *const str)
{
  if (!str)
    {
      errno = EINVAL;
      return 0;
    }

  CORBA::WChar* retval =
    CORBA::wstring_alloc (static_cast <CORBA::ULong> (ACE_OS::strlen (str)));

  // The wscpy() below assumes that the destination is a valid buffer.
  if (retval == 0)
    {
      return 0;
    }

  return ACE_OS::wscpy (retval, str);
}

CORBA::WChar*
CORBA::wstring_alloc (CORBA::ULong len)
{
  CORBA::WChar *s = 0;
  ACE_ALLOCATOR_NEW (s,
                     CORBA::WChar [(size_t) (len + 1)],
                     0);

  return s;
}

void
CORBA::wstring_free (CORBA::WChar *const str)
{
  ACE_Allocator::instance ()->free(str);
}

TAO_END_VERSIONED_NAMESPACE_DECL
