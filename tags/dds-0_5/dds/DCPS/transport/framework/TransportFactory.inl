// -*- C++ -*-
//
// $Id$

#include  "TransportImpl.h"
#include  "TransportImplFactory.h"
#include  "TransportReactorTask.h"
#include  "TransportExceptions.h"
#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::TransportFactory::TransportFactory()
{
  DBG_ENTRY("TransportFactory","TransportFactory");
}

ACE_INLINE
TAO::DCPS::TransportFactory::~TransportFactory()
{
  DBG_ENTRY("TransportFactory","~TransportFactory");
  // Since our maps hold smart pointers, we don't have to iterate over them
  // and release references.  The map destructors will cause the smart
  // pointers to destruct, and that will release the references for us.
}


/// This method is a bit unusual in regards to the way it treats the
/// impl_factory argument.  The caller gives us our own reference,
/// rather than just passing-in their "copy" (which we would normally have to
/// "duplicate" (aka, bump up the reference count) to get our own "copy").
/// Instead, we assume ownership of the impl_factory object.
///
/// This allows a caller to simply invoke this register_type() method
/// and pass in a "new SomeTransportImplFactory()" object without having
/// to deal with holding/owning its own reference to the impl factory.
/// It's this (non-standard) way because of the expected use pattern.
ACE_INLINE void
TAO::DCPS::TransportFactory::register_type(IdType                type_id,
                                           TransportImplFactory* impl_factory)
{
  DBG_ENTRY("TransportFactory","register_type");
  // We take ownership (reasons explained above) of the impl_factory
  // immediately using a (local variable) smart pointer.
  TransportImplFactory_rch factory = impl_factory;

  int result;

  // Attempt to insert the impl_factory into the impl_type_map_ while
  // we hold the lock.
  {
    GuardType guard(this->lock_);
    result = this->impl_type_map_.bind(type_id, factory);
  }

  // Check to see if it worked.
  //
  // 0 means it worked, 1 means it is a duplicate (and didn't work), and
  // -1 means something bad happened.
  if (result == 1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: type_id (%d) already registered "
                 "with TransportFactory.\n", type_id));
      throw Transport::Duplicate();
    }
  else if (result == -1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to bind type_id (%d) to impl_type_map_.\n",
                 type_id));
      throw Transport::MiscProblem();
    }
}



ACE_INLINE TAO::DCPS::TransportImpl*
TAO::DCPS::TransportFactory::obtain(IdType impl_id)
{
  DBG_ENTRY("TransportFactory","obtain");
  TransportImpl_rch impl;

  // Use separate scope for guard
//MJM: I don't understand why the guard scope is restricted here.  Is
//MJM: there a reason to release the lock before we return instead of as
//MJM: we return?
  {
    GuardType guard(this->lock_);

    // Attempt to locate the TransportImpl object
    int result = this->impl_map_.find(impl_id, impl);

    // 0 means we found it, and -1 means we didn't.
    if (result != 0)
      {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Unknown impl_id (%d).\n", impl_id));
        throw Transport::NotFound();
      }
  }

  // Give away the TransportImpl reference to the caller.
  return impl._retn();
}


ACE_INLINE void
TAO::DCPS::TransportFactory::release()
{
  DBG_SUB_ENTRY("TransportFactory","release",1);
  GuardType guard(this->lock_);

  if (!this->reactor_task_.is_nil())
    {
      this->reactor_task_->stop();
      this->reactor_task_ = 0;
    }

  // Iterate over all of the entries in the impl_map_, and
  // each TransportImpl object to shutdown().
  ImplMap::ENTRY* entry;

  for (ImplMap::ITERATOR itr(this->impl_map_);
       itr.next(entry);
       itr.advance())
    {
      entry->int_id_->shutdown();
    }

  // Clear the maps.
  this->impl_type_map_.unbind_all();
  this->impl_map_.unbind_all();
}


// Note that we don't stop or drop our reference to the reactor_task_,
// if one should exist, and the TransportImpl being release was the only
// one that required the reactor.  The other release() method above does
// deal with the reactor.
ACE_INLINE void
TAO::DCPS::TransportFactory::release(IdType impl_id)
{
  DBG_SUB_ENTRY("TransportFactory","release",2);
  int result;

  TransportImpl_rch impl;

  // Use separate scope for guard
  {
    GuardType guard(this->lock_);
    result = this->impl_map_.unbind(impl_id, impl);
  }

  // 0 means the unbind was successful, -1 means it failed.
  if (result == -1)
    {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: Unknown impl_id (%d).\n",impl_id));
        // Just leave.  Don't raise an exception here.
        return;
    }

  // Now we can tell the TransportImpl to shutdown (without having to
  // hold on to our lock_).
  impl->shutdown();
}

