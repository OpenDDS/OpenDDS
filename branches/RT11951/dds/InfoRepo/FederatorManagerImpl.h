// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORMANAGERIMPL_H
#define FEDERATORMANAGERIMPL_H

#include "federator_export.h"
#include "FederatorS.h"
#include "LinkStateManager.h"
#include "FederatorRemoteLink.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/PublisherImpl.h"
#include "ace/Condition_T.h"

#include <set>

namespace OpenDDS { namespace Federator {

class Config;

class OpenDDS_Federator_Export ManagerImpl 
  : public virtual POA_OpenDDS::Federator::Manager {
  public:
    /// Default constructor.
    ManagerImpl( Config& config);

    /// Virtual destructor.
    virtual ~ManagerImpl();

    // IDL methods.

    virtual ::OpenDDS::Federator::Status join_federation (
      const char * endpoint
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException,
      ::OpenDDS::Federator::Unavailable
    ));

    virtual ::OpenDDS::Federator::Status remove_connection (
      ::OpenDDS::Federator::RepoKey remoteId
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException,
      ::OpenDDS::Federator::ConnectionBusy
    ));

    virtual ::OpenDDS::Federator::RepoKey federationId()
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

    // Servant methods

    /// Late initialization.
    // N.B. - We need to defer intialization until after the service is
    //        initialized, which will occur long after we are constructed.
    void initialize();

    /// Accessors for the federation Id value.
    RepoKey& id();
    RepoKey  id() const;

    /// Callback with new routing information.
    void updateLinkState(
      ::OpenDDS::Federator::LinkState sample,
      ::DDS::SampleInfo info
    );

    /// Callback with new repository information.
    template< class SampleType>
    void update( SampleType& sample, ::DDS::SampleInfo& info);

  private:
    /**
     * Type mapping remote repository federation Id values to
     * information about those repositories.  Store a pointer to avoid
     * unnecessary copies in and out of the container.  Ownership is held
     * by the container.
     */
    typedef std::map< RepoKey, RemoteLink*> RemoteLinkMap;

    /// Critical section MUTEX.
    ACE_SYNCH_MUTEX lock_;

    /// Simple recursion avoidance during the join operations.
    RepoKey joining_;

    /// The repositories federation Id value within any federation.
    RepoKey federationId_;

    /// The configuration information for this manager.
    Config& config_;

    /// LinkState manager for distributing update data.
    LinkStateManager linkStateManager_;

    /// Remote repositories that are part of the Minimum Spanning Tree.
    std::set< RepoKey> mstNodes_;

    /// Retained information about remote repositories.
    RemoteLinkMap remoteLink_;

    /// Internal transport for <self> domain
    OpenDDS::DCPS::TransportImpl_rch transport_;

    /// local DomainParticipant
    ::DDS::DomainParticipant_var participant_;

    /// local Publisher
    ::DDS::Publisher_var publisher_;

    /// local LinkState listener.
    ::DDS::DataReader_var linkReader_;

};

inline
RepoKey&
ManagerImpl::id()
{
  return this->federationId_;
}

inline
RepoKey
ManagerImpl::id() const
{
  return this->federationId_;
}

}} // End namespace OpenDDS::Federator

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "FederatorManager_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma message ("FederatorManager_T.cpp template inst")
#pragma implementation ("FederatorManager_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif // FEDERATORMANAGERIMPL_H

