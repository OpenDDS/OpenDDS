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

