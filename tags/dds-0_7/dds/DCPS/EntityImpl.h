// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_ENTITY_IMPL_H
#define TAO_DDS_DCPS_ENTITY_IMPL_H

#include  "dds/DdsDcpsInfrastructureS.h"
#include  "ace/Atomic_Op_T.h"

#include  <map>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace TAO 
{
  namespace DCPS 
  {
    /**
    * @class EntityImpl
    *
    * @brief Implements the ::TAO::DCPS::Entity 
    *        interfaces.
    *
    * This class is the base class of other servant classes. 
    * e.g. DomainParticipantImpl, PublisherImpl ...
    */
    class TAO_DdsDcps_Export EntityImpl 
    : public virtual POA_DDS::Entity,
      public virtual PortableServer::RefCountServantBase
    {
    public:
      ///Constructor 
      EntityImpl ();
      
      ///Destructor 
      virtual ~EntityImpl (void);
      


    virtual ::DDS::ReturnCode_t set_enabled (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::StatusKindMask get_status_changes (
        ACE_ENV_SINGLE_ARG_DECL
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));


    virtual void set_deleted (bool state);

    virtual bool get_deleted ();

    protected:

      void set_status_changed_flag (::DDS::StatusKind status, 
                                    bool status_changed_flag);

      /// The flag indicates the entity is enabled.
      ACE_Atomic_Op<TAO_SYNCH_MUTEX, bool>       enabled_;

      /// The flag indicates the entity is being deleted.
      ACE_Atomic_Op<TAO_SYNCH_MUTEX, bool>       entity_deleted_;

      typedef bool StatusChangedFlag;
      typedef std::map< ::DDS::StatusKind, StatusChangedFlag > Statuses;

      /// The map lists all status changed flag. 
      /// The StatusChangedFlag becomes TRUE whenever the plain communication 
      /// status changes and it is reset to FALSE each time the application 
      /// accesses the plain communication status via the proper 
      /// get_<plain communication status> operation on the Entity.
      Statuses status_changes_;
    };

  } // namespace DCPS
} // namespace TAO

#endif /* TAO_DDS_DCPS_ENTITY_IMPL_H */
