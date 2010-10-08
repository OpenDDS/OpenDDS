/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_LOCALOBJECT_H
#define OPENDDS_DCPS_LOCALOBJECT_H

#include "tao/LocalObject.h"
#include "tao/Version.h"

namespace OpenDDS {
namespace DCPS {

// support TAO-style _ptr and _var
typedef CORBA::LocalObject_ptr LocalObject_ptr;
typedef CORBA::LocalObject_var LocalObject_var;

/// OpenDDS::DCPS::LocalObject resolves ambigously-inherited members like
/// _narrow and _ptr_type.  It is used from client code like so:
/// class MyReaderListener
///   : public OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener> {...};
template <class Stub>
class LocalObject
  : public virtual Stub
#if TAO_MAJOR_VERSION > 1 || TAO_MINOR_VERSION > 6
  , public virtual CORBA::LocalObject
#else
  , public virtual TAO_Local_RefCounted_Object
#endif
{
public:
  typedef typename Stub::_ptr_type _ptr_type;
  typedef typename Stub::_var_type _var_type;
  static _ptr_type _narrow(CORBA::Object_ptr obj) {
    return Stub::_narrow(obj);
  }
};

/// OpenDDS::DCPS::LocalObject_NoRefCount is the same as LocalObject, but needs to
/// be inherited from if the user wishes to allocate a "Local Object" on the stack.
/// class MyReaderListener
///   : public OpenDDS::DCPS::LocalObject_NoRefCount<OpenDDS::DCPS::DataReaderListener> {...};
template <class Stub>
class LocalObject_NoRefCount
  : public virtual Stub {
public:
  typedef typename Stub::_ptr_type _ptr_type;
  static _ptr_type _narrow(CORBA::Object_ptr obj) {
    return Stub::_narrow(obj);
  }

  // we want to keep refcounting from happening
  virtual void _add_ref() {}
  virtual void _remove_ref() {}
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_LOCALOBJECT_H */
