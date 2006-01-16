// -*- C++ -*-
//
// $Id$

#include  "DCPS/DdsDcps_pch.h"
#include  "TransportFactory.h"

#if !defined (__ACE_INLINE__)
#include "TransportFactory.inl"
#endif /* __ACE_INLINE__ */



/// This will create a new TransportImpl object (some concrete subclass of
/// TransportImpl actually), and "assign it" the provided impl_id as
/// its unique "instance" identifier.  This identifier can be passed into
/// the TransportFactory's obtain() method to obtain a reference (counted)
/// to the TransportImpl object.  This method will fail with a 
/// Transport::Duplicate exception if the impl_id is already known to the
/// TransportFactory as being assigned to a previously created instance.
///
/// The type_id value must correspond to a TransportImplFactory object
/// (actually a concrete subclass) that has already been registered with
/// this TransportFactory via its register_type() method.  If the type_id
/// is not known as a registered type, then a Transport::NotFound will
/// be raised.
///
/// If the TransportImplFactory object (the one registered with the type_id)
/// fails to create the (concrete) TransportImpl object, then a
/// Transport::UnableToCreate exception will be raised.
///
/// Any other problems will cause a Transport::MiscProblem exception to
/// be raised.  This would include a failure to create/activate the
/// TransportReactorTask object (if necessary).
///
TAO::DCPS::TransportImpl*
TAO::DCPS::TransportFactory::create(IdType impl_id, IdType type_id)
{
  DBG_ENTRY("TransportFactory","create");
  GuardType guard(this->lock_);

  TransportImplFactory_rch impl_factory;

  if (this->impl_type_map_.find(type_id,impl_factory) != 0)
    {
      // The find() didn't work.  The type_id is no good.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: type_id (%d) is not registered with "
                 "the TransportFactory.\n", type_id));
      throw Transport::NotFound();
    }

  int created_reactor = 0;
  TransportImpl_rch impl;

  // We have two ways to go here.  If the impl_factory indicates that
  // it requires a reactor task to be running in order to create the
  // TransportImpl object, then we have one path of logic to follow.
  // Otherwise, we have an easier logic path (when no reactor task is
  // required by the factory).
  if (impl_factory->requires_reactor() == 1)
    {
      // The impl_factory requires a reactor task.  Do we already have
      // one?  Or are we going to have to create one here (and activate
      // it to get it running in its own thread)?
      if (this->reactor_task_.is_nil())
        {
          // We need to create (and activate) the reactor task.
          TransportReactorTask_rch task = new TransportReactorTask();
          created_reactor = 1;

          // Attempt to activate it.
          if (task->open(0) != 0)
            {
              // We failed to activate the reactor task.
              throw Transport::MiscProblem();
            }

          // Now we can save it in our data member.
          this->reactor_task_ = task._retn();
        }

      // Now we can call the create_impl() on the TransportImplFactory
      // object and supply it with a reference to the reactor task.
      impl = impl_factory->create_impl(this->reactor_task_.in());
    }
  else
    {
      // No need to deal with the reactor here.
      impl = impl_factory->create_impl();
    }

  if (impl.is_nil())
    {
      if (created_reactor == 1)
        {
          this->reactor_task_->stop();
          this->reactor_task_ = 0;
        }

      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to create TransportImpl object for "
                 "type_id (%d).\n", type_id));
      throw Transport::UnableToCreate();
    }

  int result = this->impl_map_.bind(impl_id, impl);

  if (result == 0)
    {
      // Success!
      // Give away the TransportImpl reference to the caller.
      return impl._retn();
    }

  // Only error cases get here.

  if (created_reactor == 1)
    {
      this->reactor_task_->stop();
      this->reactor_task_ = 0;
    }

  if (result == 1)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: impl_id (%d) has already been created in the "
                 "TransportFactory.\n", impl_id));
      throw Transport::Duplicate();
    }
  else
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to bind impl_id (%d) to impl_map_.\n",
                 impl_id));
      throw Transport::MiscProblem();
    }
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
void
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


void
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
void
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


TAO::DCPS::TransportImpl*
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


