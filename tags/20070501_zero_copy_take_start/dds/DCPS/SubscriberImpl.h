// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_SUBSCRIBER_H
#define TAO_DDS_DCPS_SUBSCRIBER_H

#include "dds/DdsDcpsSubscriptionS.h"
#include "dds/DdsDcpsDataReaderRemoteC.h"
#include "dds/DdsDcpsInfoC.h"
#include "EntityImpl.h"
#include "Definitions.h"
#include "dds/DCPS/transport/framework/TransportInterface.h"
#include "ace/Synch.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <map>
#include <set>
#include <list>
#include <vector>

namespace TAO
{
  namespace DCPS
  {
    // Forward declarations
    class DomainParticipantImpl;
    class DataReaderImpl ;

    // Keep track of all the DataReaders attached to this
    // Subscriber
    struct  SubscriberDataReaderInfo
    {
      DataReaderRemote_ptr        remote_reader_ ;
      DataReaderImpl*             local_reader_;
      RepoId                      topic_id_ ;
      RepoId                      subscription_id_ ;
    } ;

    typedef std::multimap<ACE_CString,
                          SubscriberDataReaderInfo*> DataReaderMap ;

    // Keep track of DataReaders with data
    // std::set for now, want to encapsulate
    // this so we can switch between a set or
    // list depending on Presentation Qos.
    typedef std::set<DataReaderImpl *> DataReaderSet ;

    //Class SubscriberImpl
    class TAO_DdsDcps_Export SubscriberImpl
      : public virtual POA_DDS::Subscriber,
        public virtual EntityImpl,
        public virtual TransportInterface
    {
    public:

      //Constructor
      SubscriberImpl (const ::DDS::SubscriberQos & qos,
                      ::DDS::SubscriberListener_ptr a_listener,
                      DomainParticipantImpl*       participant,
                      ::DDS::DomainParticipant_ptr participant_objref);

      //Destructor
      virtual ~SubscriberImpl (void);

      virtual ::DDS::DataReader_ptr create_datareader (
        ::DDS::TopicDescription_ptr a_topic_desc,
        const ::DDS::DataReaderQos & qos,
        ::DDS::DataReaderListener_ptr a_listener
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t delete_datareader (
        ::DDS::DataReader_ptr a_datareader
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t delete_contained_entities (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::DataReader_ptr lookup_datareader (
        const char * topic_name
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t get_datareaders (
        ::DDS::DataReaderSeq_out readers,
        ::DDS::SampleStateMask sample_states,
        ::DDS::ViewStateMask view_states,
        ::DDS::InstanceStateMask instance_states
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual void notify_datareaders (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t set_qos (
        const ::DDS::SubscriberQos & qos
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual void get_qos (
        ::DDS::SubscriberQos & qos
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t set_listener (
        ::DDS::SubscriberListener_ptr a_listener,
        ::DDS::StatusKindMask mask
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::SubscriberListener_ptr get_listener (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t begin_access (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t end_access (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::DomainParticipant_ptr get_participant (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t set_default_datareader_qos (
        const ::DDS::DataReaderQos & qos
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual void get_default_datareader_qos (
        ::DDS::DataReaderQos & qos
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t copy_from_topic_qos (
        ::DDS::DataReaderQos & a_datareader_qos,
        const ::DDS::TopicQos & a_topic_qos
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::ReturnCode_t enable (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    virtual ::DDS::StatusKindMask get_status_changes (
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    /** This method is not defined in the IDL and is defined for
    *  internal use.
    *  Check if there is any datareader associated with it.
    */
    int is_clean () const;

    void add_associations (
                const WriterAssociationSeq& writers,
                DataReaderImpl* reader,
                const ::DDS::DataReaderQos reader_qos) ;

    void remove_associations(
        const WriterIdSeq& writers,
        const RepoId&      reader
      ) ;

    /*
     * Cache the subscriber's object reference.
     */
     void set_object_reference (const ::DDS::Subscriber_ptr& sub) ;

    // called by DataReaderImpl::data_received
    void data_received(DataReaderImpl *reader);

    void reader_enabled(DataReaderRemote_ptr reader,
                        const char *topic_name,
                        RepoId topic_id
      )
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ));

    //void cleanup();

    ::POA_DDS::SubscriberListener* listener_for (::DDS::StatusKind kind);

    private:

      ::DDS::SubscriberQos          qos_;
      ::DDS::DataReaderQos          default_datareader_qos_;

      DDS::StatusKindMask           listener_mask_;
      ::DDS::SubscriberListener_var  listener_;
      ::POA_DDS::SubscriberListener* fast_listener_;

      DataReaderMap                 datareader_map_ ;
      DataReaderSet                 datareader_set_ ;

      DomainParticipantImpl*        participant_;
      ::DDS::DomainParticipant_var  participant_objref_;
      ::DDS::Subscriber_var         subscriber_objref_;

      DCPSInfo_var                  repository_;

      /// this lock protects the data structures in this class.
      /// It also projects the TransportInterface (it must be held when
      /// calling any TransportInterface method).
      ACE_Recursive_Thread_Mutex    si_lock_;
    };

  } // namespace DCPS
} // namespace TAO

#endif /* TAO_DDS_DCPS_SUBSCRIBER_H  */
