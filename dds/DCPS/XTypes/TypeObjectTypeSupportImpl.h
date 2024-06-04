// The files generated from dds/DCPS/RTPS/TypeLookup.idl will expect
// TypeObjectTypeSupportImpl.h because it includes TypeObject.idl.  Since TypeObject.idl
// isn't actually processed, we can just make this file and include
// TypeObject.h, which implements what would be generated by TypeObject.idl
// if we had full IDL4 support.

#ifndef OPENDDS_DCPS_XTYPES_TYPEOBJECTTYPESUPPORTIMPL_H
#define OPENDDS_DCPS_XTYPES_TYPEOBJECTTYPESUPPORTIMPL_H

#include "TypeObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ValueReader;
class ValueWriter;

inline bool vread(ValueReader&, XTypes::TypeIdentifier&)
{
  return false;
}

inline bool vwrite(ValueWriter&, const XTypes::TypeIdentifier&)
{
  return false;
}

inline bool vread(ValueReader&, XTypes::TypeIdentifierTypeObjectPair&)
{
  return false;
}

inline bool vwrite(ValueWriter&, const XTypes::TypeIdentifierTypeObjectPair&)
{
  return false;
}

inline bool vread(ValueReader&, XTypes::TypeIdentifierPair&)
{
  return false;
}

inline bool vwrite(ValueWriter&, const XTypes::TypeIdentifierPair&)
{
  return false;
}

inline bool vread(ValueReader&, XTypes::TypeIdentifierWithSize&)
{
  return false;
}

inline bool vwrite(ValueWriter&, const XTypes::TypeIdentifierWithSize&)
{
  return false;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_XTYPES_TYPEOBJECTTYPESUPPORTIMPL_H */
