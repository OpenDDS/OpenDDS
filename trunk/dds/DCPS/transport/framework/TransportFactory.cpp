/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportDebug.h"
#include "TransportFactory.h"
#include "TransportConfiguration.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/Service_Participant.h"

#include "tao/TAO_Singleton.h"

#include "ace/OS_NS_strings.h"

#if !defined (__ACE_INLINE__)
#include "TransportFactory.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::TransportFactory*
OpenDDS::DCPS::TransportFactory::instance()
{
  // Hide the template instantiation to prevent multiple instances
  // from being created.

  return
    TAO_Singleton<TransportFactory, TAO_SYNCH_MUTEX>::instance();
}

/// This will create a new TransportImpl object (some concrete subclass of
/// TransportImpl actually), and "assign it" the provided impl_id as
/// its unique "instance" identifier.  This identifier can be passed into
/// the TransportFactory's obtain() method to obtain a reference (counted)
/// to the TransportImpl object.  This method will fail with a
/// Transport::Duplicate exception if the impl_id is already known to the
/// TransportFactory as being assigned to a previously created instance.
///
/// The type_id value is used to find the already registered TransportImplFactory
/// object (actually a concrete subclass) or create a new TransportImplFactory
/// object and bind to the impl_type_map_.
///
/// If the TransportImplFactory object (the one registered with the type_id)
/// fails to create the (concrete) TransportImpl object, then a
/// Transport::UnableToCreate exception will be raised.
///
/// Any other problems will cause a Transport::MiscProblem exception to
/// be raised.  This would include a failure to create/activate the
/// TransportReactorTask object (if necessary).
///
OpenDDS::DCPS::TransportImpl_rch
OpenDDS::DCPS::TransportFactory::create_transport_impl_i(TransportIdType impl_id, FactoryIdType type_id)
{
  DBG_ENTRY_LVL("TransportFactory","create_transport_impl_i",6);

  TransportImplFactory_rch impl_factory = this->get_or_create_factory(type_id);

  GuardType guard(this->lock_);

  int created_reactor = 0;
  TransportImpl_rch impl;
  TransportReactorTask_rch reactor_task;

  // We have two ways to go here.  If the impl_factory indicates that
  // it requires a reactor task to be running in order to create the
  // TransportImpl object, then we have one path of logic to follow.
  // Otherwise, we have an easier logic path (when no reactor task is
  // required by the factory).
  if (impl_factory->requires_reactor() == 1) {
    // Create a new reactor task for each transport.
    // We need create (and activate) the reactor task.
    // In the future, we may let the DDS user choose to
    // which reactor to use.
    TransportReactorTask* tp;
    ACE_NEW_THROW_EX(tp,
                     TransportReactorTask(),
                     CORBA::NO_MEMORY());
    reactor_task = tp;

    created_reactor = 1;

    // Attempt to activate it.
    if (reactor_task->open(0) != 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Failed to open TransportReactorTask object for ")
                 ACE_TEXT("factory (id=%s).\n"), type_id.c_str()));
      // We failed to activate the reactor task.
      throw Transport::MiscProblem();
    }

    // Now we can call the create_impl() on the TransportImplFactory
    // object and supply it with a reference to the reactor task.
    impl = impl_factory->create_impl(reactor_task.in());

  } else {
    // No need to deal with the reactor here.
    impl = impl_factory->create_impl();
  }

  if (impl.is_nil()) {
    if (created_reactor == 1) {
      reactor_task->stop();
    }

    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Failed to create TransportImpl object for ")
               ACE_TEXT("factory (id=%s).\n"), type_id.c_str()));
    throw Transport::UnableToCreate();
  }

  int result = OpenDDS::DCPS::bind(impl_map_, impl_id, impl);
  impl->set_transport_id(impl_id);
  impl->set_factory_id(type_id);
  if (TheServiceParticipant->monitor_) {
    TheServiceParticipant->monitor_->report();
  }
  impl->report();

  if (result == 0) {
    // Success!
    // The map hold the TransportImpl reference.
    return impl;

  } else if (result == 1) {
    // The TransportImpl object with the transport_id is already created.
    throw Transport::Duplicate();
  }

  // Only error cases get here.

  if (created_reactor == 1) {
    reactor_task->stop();
    reactor_task = 0;
  }

  if (result == 1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: transport (%u) has already been created in the ")
               ACE_TEXT("TransportFactory.\n"), impl_id));
    throw Transport::Duplicate();

  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Failed to bind transport (%u) to impl_map_.\n"),
               impl_id));
    throw Transport::MiscProblem();
  }
}

int
OpenDDS::DCPS::TransportFactory::load_transport_configuration(ACE_Configuration_Heap& cf)
{
  int status = 0;
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key trans_sect;

  ACE_TString sect_name;

  for (int index = 0;
       (status = cf.enumerate_sections(root, index, sect_name)) == 0;
       ++index) {
    if (ACE_OS::strncasecmp(sect_name.c_str(),
                            TRANSPORT_SECTION_NAME_PREFIX,
                            TRANSPORT_SECTION_NAME_PREFIX_LEN) == 0) { // found a [transport_impl_<id>] section
      TransportIdType transport_id
      = static_cast <TransportIdType>(ACE_OS::atoi(sect_name.substr(TRANSPORT_SECTION_NAME_PREFIX_LEN).c_str()));
      ACE_Configuration_Section_Key sect;

      if (cf.open_section(root, sect_name.c_str(), 0, sect) != 0)
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportFactory::load_transport_configuration: ")
                          ACE_TEXT("failed to open section %s\n"),
                          sect_name.c_str()),
                         -1);

      else {
        ACE_TString transport_type;
        // Get the factory_id for the transport.
        GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("transport_type"), transport_type)

        if (transport_type == ACE_TEXT("")) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportFactory::load_transport_configuration: ")
                            ACE_TEXT("missing transport_type in \"%s\" section.\n"),
                            sect_name.c_str()),
                           -1);

        } else {
          // Create a TransportConfiguration object and load the transport configuration in
          // ACE_Configuration_Heap to the TransportConfiguration object.
          TransportConfiguration_rch config
          = this->create_configuration(transport_id, transport_type);

          if (!config.is_nil() && config->load(transport_id, cf) == -1)
            return -1;
        }
      }
    }
  }

  return 0;
}

OpenDDS::DCPS::TransportImpl_rch
OpenDDS::DCPS::TransportFactory::create_transport_impl(TransportIdType transport_id,
                                                       bool auto_configure)
{
  TransportConfiguration_rch config = this->get_configuration(transport_id);

  return create_transport_impl(transport_id, config->transport_type_, auto_configure);
}

OpenDDS::DCPS::TransportImpl_rch
OpenDDS::DCPS::TransportFactory::create_transport_impl(TransportIdType transport_id,
                                                       ACE_TString transport_type,
                                                       bool auto_configure)
{
  if (transport_type.length() == 0) {
    VDBG_LVL((LM_ERROR,
              ACE_TEXT("(%P|%t) TransportFactory::create_transport_impl ")
              ACE_TEXT("transport_type is null. \n")), 0);
    throw CORBA::BAD_PARAM();
  }

  TransportImpl_rch trans_impl
  = this->create_transport_impl_i(transport_id, transport_type);

  if (auto_configure) {
    TransportConfiguration_rch config
    = this->get_or_create_configuration(transport_id, transport_type);

    if (transport_type != config->transport_type_) {
      VDBG_LVL((LM_ERROR,
                ACE_TEXT("(%P|%t) TransportFactory::create_transport_impl ")
                ACE_TEXT("transport_type conflict - provided %s configured %s\n")
                , transport_type.c_str(),
                config->transport_type_.c_str()), 0);
      throw Transport::ConfigurationConflict();
    }

    trans_impl->configure(config.in());
  }

  return trans_impl;
}

OpenDDS::DCPS::TransportConfiguration_rch
OpenDDS::DCPS::TransportFactory::get_configuration(TransportIdType transport_id)
{
  TransportConfiguration_rch config;
  int result = 0;
  {
    GuardType guard(this->lock_);
    result = OpenDDS::DCPS::find(configuration_map_, transport_id, config);
  }

  if (result != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TransportFactory::get_configuration ")
               ACE_TEXT("transport (id=%u) is not configured. \n"),
               transport_id));
    throw Transport::NotConfigured();
  }

  return config;
}

OpenDDS::DCPS::TransportConfiguration_rch
OpenDDS::DCPS::TransportFactory::create_configuration(TransportIdType transport_id,
                                                      ACE_TString transport_type)
{
  int result = 0;
  TransportGenerator_rch generator;
  {
    GuardType guard(this->lock_);
    result = find(generator_map_, transport_type.c_str(), generator);
  }

  if (result != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportFactory::create_configuration: ")
               ACE_TEXT("transport_type=%s is not registered.\n"),
               transport_type.c_str()));
    return TransportConfiguration_rch();
  }

  TransportConfiguration_rch config = generator->new_configuration(transport_id);
  this->register_configuration(transport_id, config);
  return config;
}

OpenDDS::DCPS::TransportConfiguration_rch
OpenDDS::DCPS::TransportFactory::get_or_create_configuration(TransportIdType transport_id,
                                                             ACE_TString transport_type)
{
  if (transport_type == ACE_TEXT("")) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TransportFactory::get_or_create_configuration transport_type")
               ACE_TEXT("is null. \n")));
    throw CORBA::BAD_PARAM();
  }

  TransportConfiguration_rch config;
  int result = 0;
  {
    GuardType guard(this->lock_);
    result = find(configuration_map_, transport_id, config);
  }

  if (result == 0) {
    if (transport_type != config->transport_type_) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TransportFactory::get_or_create_configuration transport_type ")
                 ACE_TEXT("conflict - provided %s configured %s\n"), transport_type.c_str(),
                 config->transport_type_.c_str()));
      throw Transport::ConfigurationConflict();
    }

    return config;
  }

  return this->create_configuration(transport_id, transport_type);
}

OpenDDS::DCPS::TransportImplFactory_rch
OpenDDS::DCPS::TransportFactory::get_or_create_factory(FactoryIdType factory_id)
{
  DBG_ENTRY_LVL("TransportFactory","get_or_create_factory",6);

  if (factory_id == ACE_TEXT("")) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) TransportFactory::get_or_create_factory factory_id is null. \n")));
    throw CORBA::BAD_PARAM();
  }

  TransportImplFactory_rch factory;
  int result = 0;
  {
    GuardType guard(this->lock_);
    result = find(impl_type_map_, factory_id, factory);
  }

  if (result == 0)
    return factory;

  TransportGenerator_rch generator;
  {
    GuardType guard(this->lock_);
    // The factory_id uses the transport type name, we use the factory_id
    // to find the generator for the transport type.
    result = find(generator_map_, factory_id.c_str(), generator);
  }

  if (result == 0) {
    factory = generator->new_factory();
    this->register_factory(factory_id, factory);

  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TransportFactory::get_or_create_factory: transport (type=%s) is not registered.\n "),
               factory_id.c_str()));
    throw Transport::NotFound();
  }

  return factory;
}

void
OpenDDS::DCPS::TransportFactory::register_generator(const ACE_TCHAR* type,
                                                    TransportGenerator* generator)
{
  DBG_ENTRY_LVL("TransportFactory","register_generator",6);
  // We take ownership (reasons explained above) of the impl_factory
  // immediately using a (local variable) smart pointer.
  TransportGenerator_rch generator_rch = generator;

  int result;

  // Attempt to insert the impl_factory into the impl_type_map_ while
  // we hold the lock.
  {
    GuardType guard(this->lock_);
    result = OpenDDS::DCPS::bind(generator_map_, type, generator_rch);
  }

  // Check to see if it worked.
  //
  // 0 means it worked, 1 means it is a duplicate (and didn't work), and
  // -1 means something bad happened.
  if (result == 1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: transport type=%s already registered ")
               ACE_TEXT("with TransportFactory.\n"), type));
    throw Transport::Duplicate();

  } else if (result == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Failed to bind transport type=%s to generator_map_.\n"),
               type));
    throw Transport::MiscProblem();
  }

  // Get the list of default transport. Some transports(e.g. SimpleMcast) have multiple default
  // transport ids.
  TransportIdList default_ids;
  generator_rch->default_transport_ids(default_ids);

  // Create default transport configuration associated with the default id.
  TransportIdList::iterator itEnd = default_ids.end();

  for (TransportIdList::iterator it = default_ids.begin(); it != itEnd; ++it) {
    TransportConfiguration_rch config = create_configuration(*it, type);
  }
}

/// This method is a bit unusual in regards to the way it treats the
/// impl_factory argument.  The caller gives us our own reference,
/// rather than just passing-in their "copy" (which we would normally have to
/// "duplicate" (aka, bump up the reference count) to get our own "copy").
/// Instead, we assume ownership of the impl_factory object.
///
/// This allows a caller to simply invoke this register_factory() method
/// and pass in a "new SomeTransportImplFactory()" object without having
/// to deal with holding/owning its own reference to the impl factory.
/// It's this (non-standard) way because of the expected use pattern.
void
OpenDDS::DCPS::TransportFactory::register_factory(FactoryIdType            factory_id,
                                                  TransportImplFactory_rch factory)
{
  DBG_ENTRY_LVL("TransportFactory","register_factory",6);

  int result;

  // Attempt to insert the impl_factory into the impl_type_map_ while
  // we hold the lock.
  {
    GuardType guard(this->lock_);
    result = OpenDDS::DCPS::bind(impl_type_map_, factory_id, factory);
  }

  // Check to see if it worked.
  //
  // 0 means it worked, 1 means it is a duplicate (and didn't work), and
  // -1 means something bad happened.
  if (result == 1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: factory (id=%s) already registered ")
               ACE_TEXT("in impl_type_map_.\n"), factory_id.c_str()));
    throw Transport::Duplicate();

  } else if (result == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Failed to bind factory (id=%s) to impl_type_map_.\n"),
               factory_id.c_str()));
    throw Transport::MiscProblem();
  }
}

void
OpenDDS::DCPS::TransportFactory::register_configuration(TransportIdType       transport_id,
                                                        TransportConfiguration_rch  config)
{
  DBG_ENTRY_LVL("TransportFactory","register_configuration",6);

  int result;

  // Attempt to insert the impl_factory into the impl_type_map_ while
  // we hold the lock.
  {
    GuardType guard(this->lock_);
    result = OpenDDS::DCPS::bind(configuration_map_, transport_id, config);
  }

  // Check to see if it worked.
  //
  // 0 means it worked, 1 means it is a duplicate (and didn't work), and
  // -1 means something bad happened.
  if (result == 1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: transport (id=%u) already registered ")
               ACE_TEXT("in configuration_map_.\n"), transport_id));
    throw Transport::Duplicate();

  } else if (result == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Failed to bind transport (id=%u) to configuration_map_.\n"),
               transport_id));
    throw Transport::MiscProblem();
  }
}

OpenDDS::DCPS::TransportImpl_rch
OpenDDS::DCPS::TransportFactory::obtain(TransportIdType impl_id)
{
  DBG_ENTRY_LVL("TransportFactory","obtain",6);
  GuardType guard(this->lock_);

  // This is a bit complex to avoid creating nil entries in the map.
  // The entire class would need to be cleansed of assumptions about
  // contents in order to just
  //   return this->impl_map_[ impl_id];
  ImplMap::iterator where = this->impl_map_.find(impl_id);

  if (where == this->impl_map_.end()) {
    return TransportImpl_rch();

  } else {
    return where->second;
  }

}

void
OpenDDS::DCPS::TransportFactory::release()
{
  DBG_ENTRY_LVL("TransportFactory","release",6);
  GuardType guard(this->lock_);

  // Iterate over all of the entries in the impl_map_, and
  // each TransportImpl object to shutdown().
  for (ImplMap::iterator iter = impl_map_.begin();
       iter != impl_map_.end();
       ++iter) {
    iter->second->shutdown();
  }

  // Clear the maps.
  impl_type_map_.clear();
  impl_map_.clear();
  generator_map_.clear();
  configuration_map_.clear();
}

void
OpenDDS::DCPS::TransportFactory::release(TransportIdType impl_id)
{
  DBG_ENTRY_LVL("TransportFactory","release",6);
  int result;

  TransportImpl_rch impl;

  // Use separate scope for guard
  {
    GuardType guard(this->lock_);
    result = unbind(impl_map_, impl_id, impl);

    if (TheServiceParticipant->monitor_) {
      TheServiceParticipant->monitor_->report();
    }

    // 0 means the unbind was successful, -1 means it failed.
    if (result == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Unknown impl_id (%d) in impl_map_.\n"),impl_id));
      // Just leave.  Don't raise an exception here.
      return;
    }

    result = unbind(configuration_map_, impl_id);

    if (result == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Unknown impl_id (%d) in configuration_map_.\n"),impl_id));
      // Just leave.  Don't raise an exception here.
      return;
    }
  }

  // Now we can tell the TransportImpl to shutdown (without having to
  // hold on to our lock_).
  impl->shutdown();
}
