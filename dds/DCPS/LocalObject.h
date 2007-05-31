// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_LOCALOBJECT_H
#define TAO_DDS_DCPS_LOCALOBJECT_H

#include "tao/LocalObject.h"

namespace TAO
{
  namespace DCPS
  {
    // support "TAO::DCPS" style _ptr and _var
    typedef CORBA::LocalObject_ptr LocalObject_ptr;
    typedef CORBA::LocalObject_var LocalObject_var;


    /// TAO::DCPS::LocalObject resolves ambigously-inherited members like
    /// _narrow and _ptr_type.  It is used from client code like so:
    /// class MyReaderListener
    ///   : public TAO::DCPS::LocalObject<TAO::DCPS::DataReaderListener> {...};
    template <class Stub>
    class LocalObject
      : public virtual Stub
      , public virtual TAO_Local_RefCounted_Object
    {
    public:
      typedef typename Stub::_ptr_type _ptr_type;
      static _ptr_type _narrow (::CORBA::Object_ptr obj)
      { 
        return Stub::_narrow(obj);
      }
    };

  };
};

#endif /* TAO_DDS_DCPS_LOCALOBJECT_H */
