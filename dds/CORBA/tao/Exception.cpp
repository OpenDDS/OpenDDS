#include "tao/Exception.h"
#include "tao/SystemException.h"
#include "tao/Environment.h"
#include "tao/ORB_Constants.h"
#include "tao/CORBA_String.h"

#include "ace/Malloc.h"
#include "ace/SString.h"
#include "ace/OS_NS_string.h"

#if !defined (ACE_LACKS_IOSTREAM_TOTALLY)
// Needed for ostream& operator<< (ostream &os, const CORBA::Exception &e)
// FUZZ: disable check_for_streams_include
#include "ace/streams.h"
#endif /* (ACE_LACKS_IOSTREAM_TOTALLY) */

#if !defined (__ACE_INLINE__)
# include "tao/Exception.inl"
#endif /* __ACE_INLINE__ */

#include "ace/OS_NS_stdio.h"


// ****************************************************************

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

CORBA::Exception::Exception (const char * repository_id,
                             const char * local_name)
  : id_ (CORBA::string_dup (repository_id)),
    name_ (CORBA::string_dup (local_name))
{
  ACE_ASSERT (this->id_.in () != 0 && this->name_.in () != 0);
}

CORBA::Exception::Exception (const CORBA::Exception &src)
  : id_ (CORBA::string_dup (src.id_)),
    name_ (CORBA::string_dup (src.name_))
{
  ACE_ASSERT (this->id_.in () != 0 && this->name_.in () != 0);
}

// NOTE: It's this code, not anything defined in a subclass, which is
// responsible for releasing any storage owned by the exception.  It
// can do this because it's got the local name and the id.

CORBA::Exception::Exception (void)
  : id_ (),
    name_ ()
{
}

CORBA::Exception::~Exception (void)
{
}

CORBA::Exception &
CORBA::Exception::operator= (const CORBA::Exception &src)
{
  this->id_ = CORBA::string_dup (src.id_);
  ACE_ASSERT (this->id_.in () != 0);

  this->name_ = CORBA::string_dup (src.name_);
  ACE_ASSERT (this->name_.in () != 0);

  return *this;
}

const char *
CORBA::Exception::_rep_id (void) const
{
  return this->id_.in ();
}

const char *
CORBA::Exception::_name (void) const
{
  return this->name_.in ();
}

void
CORBA::Exception::_tao_print_exception (const char *user_provided_info,
                                        FILE *) const
{
  ACE_ERROR ((LM_ERROR,
              ACE_TEXT ("(%P|%t) EXCEPTION, %C\n")
              ACE_TEXT ("%C\n"),
              user_provided_info,
              this->_info ().c_str ()));
}

#if defined (ACE_USES_WCHAR)
void
CORBA::Exception::_tao_print_exception (const ACE_WCHAR_T *info,
                                        FILE *f) const
{
  // Even though this call causes additional type conversions, this is
  // better for the maintenance.  Plus, this will occur only on
  // exception anyway.
  this->_tao_print_exception (ACE_TEXT_ALWAYS_CHAR (info), f);
}
#endif  // ACE_USES_WCHAR

void
CORBA::Exception::_tao_any_destructor (void *x)
{
  CORBA::Exception *tmp = static_cast<CORBA::Exception *> (x);
  delete tmp;
}

#if !defined (ACE_LACKS_IOSTREAM_TOTALLY)

namespace CORBA
{
  ACE_OSTREAM_TYPE& operator<< (ACE_OSTREAM_TYPE &os,
                                const CORBA::Exception &e)
  {
    os << e._name () << " (" << e._rep_id () << ')';

    return os;
  }

  ACE_OSTREAM_TYPE& operator<< (ACE_OSTREAM_TYPE &os,
                                const CORBA::Exception *e)
  {
    os << e->_name () << " (" << e->_rep_id () << ')';

    return os;
  }
}

#endif /* (ACE_LACKS_IOSTREAM_TOTALLY) */

TAO_END_VERSIONED_NAMESPACE_DECL
