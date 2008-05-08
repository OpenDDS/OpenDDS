// -*- C++ -*-
//
// $Id$

// TMB - I had to add the following line
#include "Service_Participant.h"
#include "ace/OS_NS_string.h" // for strcmp() in partition operator==()

namespace OpenDDS
{
  namespace DCPS
  {
    ACE_INLINE 
    ACE_Time_Value time_to_time_value (const ::DDS::Time_t& t)
    {
      ACE_Time_Value tv (t.sec, t.nanosec / 1000);
      return tv;
    }

    ACE_INLINE
    ::DDS::Time_t time_value_to_time (const ACE_Time_Value& tv)
    {
      ::DDS::Time_t t;
      t.sec = static_cast<CORBA::Long> (tv.sec ());
      t.nanosec = tv.usec () * 1000;
      return t;
    }
    
    ACE_INLINE 
    ACE_Time_Value duration_to_time_value (const ::DDS::Duration_t& t)
    {
      ACE_Time_Value tv (t.sec, t.nanosec / 1000);
      return tv;
    }

    ACE_INLINE
    ::DDS::Duration_t time_value_to_duration (const ACE_Time_Value& tv)
    {
      ::DDS::Duration_t t;
      t.sec = static_cast<CORBA::Long> (tv.sec ());
      t.nanosec = tv.usec () * 1000;
      return t;
    }

    ACE_INLINE
    CORBA::Long
    get_instance_sample_list_depth (
      ::DDS::HistoryQosPolicyKind history_kind,
      long                        history_depth,
      long                        max_samples_per_instance)
    {
      CORBA::Long depth = 0;

      if (history_kind == ::DDS::KEEP_ALL_HISTORY_QOS)
      {
        // The spec says history_depth is "has no effect"
        // when history_kind = KEEP_ALL so use
        // max_samples_per_instance.
        depth = max_samples_per_instance;
      }
      else // history.kind == ::DDS::KEEP_LAST_HISTORY_QOS
      {
        depth = history_depth;
      }

      if (depth == ::DDS::LENGTH_UNLIMITED)
      {
        // ::DDS::LENGTH_UNLIMITED is negative so make it a positive
        // value that is for all intents and purposes unlimited and we
        // can use it for comparisons.  Use 2147483647L because that
        // is the greatest value a signed CORBA::Long (a signed 32 bit
        // integer) can have.
        // WARNING: The client risks running out of memory in this case.
        depth = 0x7fffffff; // ACE_Numeric_Limits<CORBA::Long>::max ();
      }

      return depth;
    }

    ACE_INLINE
    bool
    Qos_Helper::consistent (const ::DDS::DomainParticipantQos& qos)
    {
      ACE_UNUSED_ARG(qos);
      return true;
    }


    ACE_INLINE
    bool 
    Qos_Helper::consistent (const ::DDS::TopicQos            & qos) 
    {      
      if (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
          qos.history.depth > qos.resource_limits.max_samples_per_instance)
        return false;
      if (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
          qos.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED &&
          qos.resource_limits.max_samples_per_instance > qos.resource_limits.max_samples)
        return false;

      return true;
    }


    ACE_INLINE
    bool 
    Qos_Helper::consistent (const ::DDS::DataWriterQos       & qos) 
    {
      if (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
          qos.history.depth > qos.resource_limits.max_samples_per_instance)
        return false;
      if (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
          qos.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED &&
          qos.resource_limits.max_samples_per_instance > qos.resource_limits.max_samples)
        return false;

      return true;
    }


    ACE_INLINE
    bool 
    Qos_Helper::consistent (const ::DDS::PublisherQos        & qos) 
    {
      ACE_UNUSED_ARG(qos);
      return true;
    }


    ACE_INLINE
    bool 
    Qos_Helper::consistent (const ::DDS::DataReaderQos       & qos) 
    {
      //TBD: These should be check when the DEADLINE and TIME_BASED_FILTER
      //     qos are supported.

      //if (qos.deadline.period < qos.time_based_filter.minimum_separation) 
      //{
      //  return false;
      //}

      if (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
          qos.history.depth > qos.resource_limits.max_samples_per_instance)
        return false;
      if (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
          qos.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED &&
          qos.resource_limits.max_samples_per_instance > qos.resource_limits.max_samples)
        return false;

      return true;
    }


    ACE_INLINE
    bool 
    Qos_Helper::consistent (const ::DDS::SubscriberQos       & qos) 
    {
      ACE_UNUSED_ARG(qos);
      return true;
    }

    // Note: Since in the first implmenation of DSS in TAO 
    //       a limited number of QoS values are allowed to be
    //       modified, the validity tests are simplified to mostly
    //       being a check that they are the defaults defined in
    //       the specification.
    //       SO INVALID ALSO INCLUDES UNSUPPORTED QoS.
    // TBD - when QoS become support the valid checks should check
    //       the ranges of the values of the QoS.

    // The spec does not have specification about the content of  
    // UserDataQosPolicy,TopicDataQosPolicy and GroupDataQosPolicy
    // so they are valid with any value.
    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::UserDataQosPolicy& qos) 
    {
      ACE_UNUSED_ARG (qos);
      return true;
    }
     
    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::TopicDataQosPolicy & qos) 
    {
      ACE_UNUSED_ARG (qos);
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::GroupDataQosPolicy& qos) 
    {
      ACE_UNUSED_ARG (qos);
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::TransportPriorityQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_TransportPriorityQosPolicy())
      {
        return true;
      }
      return false;
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::LifespanQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_LifespanQosPolicy())
      {
        return true;
      }
      return false;
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DurabilityQosPolicy& qos) 
    {
      return
        qos.kind == ::DDS::VOLATILE_DURABILITY_QOS
        || qos.kind == ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS
        /*|| qos.kind == ::DDS::TRANSIENT_DURABILITY_QOS
          || qos.kind == ::DDS::PERSISTENT_DURABILITY_QOS*/;
    }

        
    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::PresentationQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_PresentationQosPolicy())
      {
        return true;
      }
      return false;
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DeadlineQosPolicy& qos) 
    {
      ::DDS::Duration_t const DDS_DURATION_INFINITY =
        {
          ::DDS::DURATION_INFINITY_SEC,
          ::DDS::DURATION_INFINITY_NSEC
        };

      // Only accept infinite or positive finite deadline periods.
      //
      // Note that it doesn't make much sense for users to set
      // deadline periods less than 10 milliseconds since the
      // underlying timer resolution is generally no better than
      // that.
      return
        qos.period == DDS_DURATION_INFINITY
        || qos.period.sec > 0
        || (qos.period.sec >= 0 && qos.period.nanosec > 0);
    }

      
    ACE_INLINE 
    bool Qos_Helper::valid (const ::DDS::LatencyBudgetQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_LatencyBudgetQosPolicy())
      {
        return true;
      }
      return false;
    }


    ACE_INLINE       
    bool Qos_Helper::valid (const ::DDS::OwnershipQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_OwnershipQosPolicy())
      {
        return true;
      }
      return false;
    }


    ACE_INLINE      
    bool Qos_Helper::valid (const ::DDS::OwnershipStrengthQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_OwnershipStrengthQosPolicy())
      {
        return true;
      }
      return false;
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::LivelinessQosPolicy& qos) 
    {
      // DeMorgan's Law : !(inf || valid ) == !inf && !valid
      if (!(qos.lease_duration.sec    ==  ::DDS::DURATION_INFINITY_SEC &&
           qos.lease_duration.nanosec == ::DDS::DURATION_INFINITY_NSEC) &&
          !(qos.lease_duration.sec    > 0) )
         return false; // bad lease_duration value

      //Only valid for qos.kind == ::DDS::AUTOMATIC_LIVELINESS_QOS for first 
      //implementation.
      if (qos.kind == ::DDS::AUTOMATIC_LIVELINESS_QOS)
      {
        return true;
      }
      return false;
    }


    ACE_INLINE       
    bool Qos_Helper::valid (const ::DDS::TimeBasedFilterQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_TimeBasedFilterQosPolicy())
      {
        return true;
      }
      return false;
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::PartitionQosPolicy& qos) 
    {
      ACE_UNUSED_ARG (qos);
      return true;
    }  
      
    ACE_INLINE 
    bool Qos_Helper::valid (const ::DDS::ReliabilityQosPolicy& qos) 
    {
      if (qos.kind == ::DDS::BEST_EFFORT_RELIABILITY_QOS
          || (qos.kind == ::DDS::RELIABLE_RELIABILITY_QOS))
        {
          return true;
        }
      return false;
    }  
      
    ACE_INLINE 
    bool Qos_Helper::valid (const ::DDS::DestinationOrderQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_DestinationOrderQosPolicy())
      {
        return true;
      }
      return false;
    }  
          

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::HistoryQosPolicy& qos) 
    {
      if (qos.depth != ::DDS::LENGTH_UNLIMITED &&
          qos.depth < 1)
        return false; // invalid depth

      if (qos.kind == ::DDS::KEEP_LAST_HISTORY_QOS ||
          qos.kind == ::DDS::KEEP_ALL_HISTORY_QOS)
        {
          return true;
        }
      else 
        {
          return false;
        }
    }  
         
    ACE_INLINE    
    bool Qos_Helper::valid (const ::DDS::ResourceLimitsQosPolicy& qos) 
    {
      ACE_UNUSED_ARG(qos);

      if (qos.max_samples != ::DDS::LENGTH_UNLIMITED &&
          qos.max_samples <= 0)
        return false;
      if (qos.max_instances != ::DDS::LENGTH_UNLIMITED &&
          qos.max_instances <= 0)
        return false;
      if (qos.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
          qos.max_samples_per_instance <= 0)
        return false;

      return true;
    }  
        
      
    ACE_INLINE 
    bool Qos_Helper::valid (const ::DDS::EntityFactoryQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_EntityFactoryQosPolicy())  {
        return true;
      }
      return false;
    }  

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::WriterDataLifecycleQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_WriterDataLifecycleQosPolicy())
      {
        return true;
      }
      return false;
    }  
       

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::ReaderDataLifecycleQosPolicy& qos) 
    {
      if (qos == TheServiceParticipant->initial_ReaderDataLifecycleQosPolicy())
      {
        return true;
      }
      return false;
    }  


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DomainParticipantQos& qos) 
    {
      if (valid(qos.user_data) && valid(qos.entity_factory))
      {
        return true;
      }
      return false;
    }  


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::TopicQos& qos) 
    {
      if (valid(qos.topic_data) 
        && valid(qos.durability)
        && valid(qos.deadline)
        && valid(qos.latency_budget)
        && valid(qos.liveliness)
        && valid(qos.destination_order)
        && valid(qos.history)
        && valid(qos.resource_limits)
        && valid(qos.transport_priority)
        && valid(qos.lifespan)
        && valid(qos.ownership))
      {
        return true;
      }
      return false;
    }  


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DataWriterQos& qos) 
    {
      if (valid(qos.durability)
        && valid(qos.deadline)
        && valid(qos.latency_budget)
        && valid(qos.liveliness)
        && valid(qos.destination_order)
        && valid(qos.history)
        && valid(qos.resource_limits)
        && valid(qos.transport_priority)
        && valid(qos.lifespan)
        && valid(qos.user_data)
        && valid(qos.ownership_strength)
        && valid(qos.writer_data_lifecycle))
      {
        return true;
      }
      return false;
    }      

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::PublisherQos& qos) 
    {
      if (valid(qos.presentation)
        && valid(qos.partition)
        && valid(qos.group_data)
        && valid(qos.entity_factory))
      {
        return true;
      }
      return false;
    }          
        

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DataReaderQos& qos) 
    {
      if (valid(qos.durability)
        && valid(qos.deadline)
        && valid(qos.latency_budget)
        && valid(qos.liveliness)
        && valid(qos.reliability)
        && valid(qos.destination_order)
        && valid(qos.history)
        && valid(qos.resource_limits)
        && valid(qos.user_data)
        && valid(qos.time_based_filter)
        && valid(qos.reader_data_lifecycle))
      {
        return true;
      }
      return false;
    }      

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::SubscriberQos& qos) 
    {
      if (valid(qos.presentation)
        && valid(qos.partition)
        && valid(qos.group_data)
        && valid(qos.entity_factory))
      {
        return true;
      }
      return false;
    }          

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::UserDataQosPolicy& qos1, 
				                         const ::DDS::UserDataQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }
     
    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::TopicDataQosPolicy & qos1,
                                 const ::DDS::TopicDataQosPolicy & qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::GroupDataQosPolicy& qos1,
				                         const ::DDS::GroupDataQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::TransportPriorityQosPolicy& qos1,
                                 const ::DDS::TransportPriorityQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::LifespanQosPolicy& qos1,
                                 const ::DDS::LifespanQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::DurabilityQosPolicy& qos1,
                                 const ::DDS::DurabilityQosPolicy& qos2) 
    {
      if (qos1 == qos2)
         return true; 
      else
         return false;
    }

        
    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::PresentationQosPolicy& qos1,
                                 const ::DDS::PresentationQosPolicy& qos2) 
    {
      if (qos1 == qos2)
         return true; 
      else
         return false;
    }

/*******TBD: These qos are not supported currently, they are changeable, but   ****
 ******     need check compatiable between publisher and subscriber ends when  ****
 *******    changing the qos.                                                  ****/

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::DeadlineQosPolicy& qos1,
                                 const ::DDS::DeadlineQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }

      
    ACE_INLINE 
    bool Qos_Helper::changeable (const ::DDS::LatencyBudgetQosPolicy& qos1,
                                 const ::DDS::LatencyBudgetQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }


    ACE_INLINE       
    bool Qos_Helper::changeable (const ::DDS::OwnershipQosPolicy& qos1,
                                 const ::DDS::OwnershipQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }

//**********************************************************************************
         

    ACE_INLINE      
    bool Qos_Helper::changeable (const ::DDS::OwnershipStrengthQosPolicy& qos1,
                                 const ::DDS::OwnershipStrengthQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::LivelinessQosPolicy& qos1,
                                 const ::DDS::LivelinessQosPolicy& qos2) 
    {
      if (qos1 == qos2)
         return true; 
      else
         return false;
    }


    ACE_INLINE       
    bool Qos_Helper::changeable (const ::DDS::TimeBasedFilterQosPolicy& qos1,
                                 const ::DDS::TimeBasedFilterQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::PartitionQosPolicy& qos1,
                                 const ::DDS::PartitionQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }  
      
    ACE_INLINE 
    bool Qos_Helper::changeable (const ::DDS::ReliabilityQosPolicy& qos1,
                                 const ::DDS::ReliabilityQosPolicy& qos2) 
    {
      if (qos1 == qos2)
         return true; 
      else
         return false;
    }  
      
    ACE_INLINE 
    bool Qos_Helper::changeable (const ::DDS::DestinationOrderQosPolicy& qos1,
                                 const ::DDS::DestinationOrderQosPolicy& qos2) 
    {
      if (qos1 == qos2)
         return true; 
      else
         return false;
    }  
          

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::HistoryQosPolicy& qos1,
                                 const ::DDS::HistoryQosPolicy& qos2) 
    {
      if (qos1 == qos2)
         return true; 
      else
         return false;
    }  
         
    ACE_INLINE    
    bool Qos_Helper::changeable (const ::DDS::ResourceLimitsQosPolicy& qos1,
                                 const ::DDS::ResourceLimitsQosPolicy& qos2) 
    {
      if (qos1 == qos2)
         return true; 
      else
         return false;
    }  
        
      
    ACE_INLINE 
    bool Qos_Helper::changeable (const ::DDS::EntityFactoryQosPolicy& qos1,
                                 const ::DDS::EntityFactoryQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }  

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::WriterDataLifecycleQosPolicy& qos1,
                                 const ::DDS::WriterDataLifecycleQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }  
       

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::ReaderDataLifecycleQosPolicy& qos1,
                                 const ::DDS::ReaderDataLifecycleQosPolicy& qos2) 
    {
      ACE_UNUSED_ARG (qos1);
      ACE_UNUSED_ARG (qos2);
      return true;
    }  


    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::DomainParticipantQos& qos1,
                                 const ::DDS::DomainParticipantQos& qos2) 
    {
      if (changeable(qos1.user_data, qos2.user_data) 
          && changeable(qos1.entity_factory, qos2.entity_factory))
      {
        return true;
      }
      return false;
    }  


    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::TopicQos& qos1,
                                 const ::DDS::TopicQos& qos2) 
    {
      if (changeable(qos1.topic_data, qos2.topic_data) 
        && changeable(qos1.durability, qos2.durability)
        && changeable(qos1.deadline, qos2.deadline)
        && changeable(qos1.latency_budget, qos2.latency_budget)
        && changeable(qos1.liveliness, qos2.liveliness)
        && changeable(qos1.destination_order, qos2.destination_order)
        && changeable(qos1.history, qos2.history)
        && changeable(qos1.resource_limits, qos2.resource_limits)
        && changeable(qos1.transport_priority, qos2.transport_priority)
        && changeable(qos1.lifespan, qos2.lifespan)
        && changeable(qos1.ownership, qos2.ownership))
      {
        return true;
      }
      return false;
    }  


    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::DataWriterQos& qos1,
                                 const ::DDS::DataWriterQos& qos2)
    {
      return changeable(qos1.durability, qos2.durability)
        && changeable(qos1.deadline, qos2.deadline)
        && changeable(qos1.latency_budget, qos2.latency_budget)
        && changeable(qos1.liveliness, qos2.liveliness)
        && changeable(qos1.destination_order, qos2.destination_order)
        && changeable(qos1.history, qos2.history)
        && changeable(qos1.resource_limits, qos2.resource_limits)
        && changeable(qos1.transport_priority, qos2.transport_priority)
        && changeable(qos1.lifespan, qos2.lifespan)
        && changeable(qos1.user_data, qos2.user_data)
        && changeable(qos1.ownership_strength, qos2.ownership_strength)
        && changeable(qos1.writer_data_lifecycle, qos2.writer_data_lifecycle);
    }      

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::PublisherQos& qos1,
                                 const ::DDS::PublisherQos& qos2) 
    {
      return changeable(qos1.presentation, qos2.presentation)
        && changeable(qos1.partition, qos2.partition)
        && changeable(qos1.group_data, qos2.group_data)
        && changeable(qos1.entity_factory, qos2.entity_factory);
    }          
        

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::DataReaderQos& qos1,
                                 const ::DDS::DataReaderQos& qos2) 
    {
      return changeable(qos1.durability, qos2.durability)
        && changeable(qos1.deadline, qos2.deadline)
        && changeable(qos1.latency_budget, qos2.latency_budget)
        && changeable(qos1.liveliness, qos2.liveliness)
        && changeable(qos1.reliability, qos2.reliability)
        && changeable(qos1.destination_order, qos2.destination_order)
        && changeable(qos1.history, qos2.history)
        && changeable(qos1.resource_limits, qos2.resource_limits)
        && changeable(qos1.user_data, qos2.user_data)
        && changeable(qos1.time_based_filter, qos2.time_based_filter)
        && changeable(qos1.reader_data_lifecycle, qos2.reader_data_lifecycle);
    }      

    ACE_INLINE
    bool Qos_Helper::changeable (const ::DDS::SubscriberQos& qos1,
                                 const ::DDS::SubscriberQos& qos2) 
    {
      return changeable(qos1.presentation, qos2.presentation)
        && changeable(qos1.partition, qos2.partition)
        && changeable(qos1.group_data, qos2.group_data)
        && changeable(qos1.entity_factory, qos2.entity_factory);
    }          


    ACE_INLINE
    bool Qos_Helper::lease_greater_than (const ::DDS::LivelinessQosPolicy& qos1,
                                         const ::DDS::LivelinessQosPolicy& qos2) 
    {
      // kind is ignored
      if (((qos2.lease_duration.sec     != ::DDS::DURATION_INFINITY_SEC) ||
           (qos2.lease_duration.nanosec != ::DDS::DURATION_INFINITY_NSEC)  )    &&
          (((qos1.lease_duration.sec     == ::DDS::DURATION_INFINITY_SEC) &&
            (qos1.lease_duration.nanosec == ::DDS::DURATION_INFINITY_NSEC)  ) ||
           ((qos1.lease_duration.sec   >  qos2.lease_duration.sec)       ||
            ((qos1.lease_duration.sec  == qos2.lease_duration.sec) &&
             (qos1.lease_duration.nanosec > qos2.lease_duration.nanosec)) )    )  )
      {
        return true;
      }
      return false;
    }


  } // namespace ::DDS
} // namespace OpenDDS


ACE_INLINE
bool operator== (const ::DDS::Duration_t& t1, 
                 const ::DDS::Duration_t& t2)
{
  if (t1.sec == t2.sec && t1.nanosec == t2.nanosec)
  {
    return true;
  }
  else
  {
    return false;
  }
}


ACE_INLINE
bool operator < (const ::DDS::Duration_t& t1, 
                  const ::DDS::Duration_t& t2)
{
  if (t1.sec < t2.sec 
    || (t1.sec == t2.sec && t1.nanosec < t2.nanosec))
  {
    return true;
  }
  else
  {
    return false;
  }
}

ACE_INLINE
bool operator<= (const ::DDS::Duration_t& t1, 
                  const ::DDS::Duration_t& t2)
{
  return t1 < t2 || t1 == t2;
}


ACE_INLINE
bool operator == (const ::DDS::UserDataQosPolicy& qos1, 
                  const ::DDS::UserDataQosPolicy& qos2) 
{
  if (qos1.value == qos2.value)
  {
    return true;
  }
  return false;
}
  

ACE_INLINE
bool operator == (const ::DDS::TopicDataQosPolicy & qos1, 
                  const ::DDS::TopicDataQosPolicy & qos2) 
{
  if (qos1.value == qos2.value)
  {
    return true;
  }
  return false;
}


ACE_INLINE
bool operator == (const ::DDS::GroupDataQosPolicy& qos1, 
                  const ::DDS::GroupDataQosPolicy& qos2) 
{
  if (qos1.value == qos2.value)
  {
    return true;
  }
  return false;
}


ACE_INLINE
bool operator == (const ::DDS::TransportPriorityQosPolicy& qos1, 
                  const ::DDS::TransportPriorityQosPolicy& qos2) 
{
  if (qos1.value == qos2.value)
  {
    return true;
  }
  return false;
}


ACE_INLINE
bool operator == (const ::DDS::LifespanQosPolicy& qos1, 
                  const ::DDS::LifespanQosPolicy& qos2) 
{
  if (qos1.duration == qos2.duration)
  {
    return true;
  }
  return false;
}


ACE_INLINE
bool operator == (const ::DDS::DurabilityQosPolicy& qos1,
                  const ::DDS::DurabilityQosPolicy& qos2) 
{
  if (qos1.kind == qos2.kind
  && qos1.service_cleanup_delay == qos2.service_cleanup_delay)
  {
    return true;
  }
  return false;
}

    
ACE_INLINE
bool operator == (const ::DDS::PresentationQosPolicy& qos1,
                  const ::DDS::PresentationQosPolicy& qos2) 
{
  if (qos1.access_scope == qos2.access_scope
  && qos1.coherent_access == qos2.coherent_access
  && qos1.ordered_access == qos2.ordered_access)
  {
    return true;
  }
  return false;
}


ACE_INLINE
bool operator == (const ::DDS::DeadlineQosPolicy& qos1,
                  const ::DDS::DeadlineQosPolicy& qos2) 
{
  if (qos1.period == qos2.period)
  {
    return true;
  }
  return false;
}

    
ACE_INLINE
bool operator == (const ::DDS::LatencyBudgetQosPolicy& qos1,
                  const ::DDS::LatencyBudgetQosPolicy& qos2) 
{
  if (qos1.duration == qos2.duration)
  {
    return true;
  }
  return false;
}

    
ACE_INLINE
bool operator == (const ::DDS::OwnershipQosPolicy& qos1,
                  const ::DDS::OwnershipQosPolicy& qos2) 
{
  if (qos1.kind == qos2.kind)
  {
    return true;
  }
  return false;
}

  
ACE_INLINE
bool operator == (const ::DDS::OwnershipStrengthQosPolicy& qos1,
                  const ::DDS::OwnershipStrengthQosPolicy& qos2) 
{
  if (qos1.value == qos2.value)
  {
    return true;
  }
  return false;
}


ACE_INLINE
bool operator == (const ::DDS::LivelinessQosPolicy& qos1,
                  const ::DDS::LivelinessQosPolicy& qos2) 
{
  if (qos1.kind == qos2.kind
    && qos1.lease_duration == qos2.lease_duration)
  {
    return true;
  }
  return false;
}


ACE_INLINE
bool operator == (const ::DDS::TimeBasedFilterQosPolicy& qos1,
                  const ::DDS::TimeBasedFilterQosPolicy& qos2) 
{
  if (qos1.minimum_separation == qos2.minimum_separation)
  {
    return true;
  }
  return false;
}


ACE_INLINE
bool operator == (const ::DDS::PartitionQosPolicy& qos1,
                  const ::DDS::PartitionQosPolicy& qos2) 
{
  if (qos1.name.length() == qos2.name.length())
  {
    for(CORBA::ULong i = 0; i < qos1.name.length(); i++)
    {
      if ( 0 != ACE_OS::strcmp( qos1.name[i], qos2.name[i]) )
      {
        return false;
      }
    }
    return true;
  }
  return false;
}  
  

ACE_INLINE 
bool operator == (const ::DDS::ReliabilityQosPolicy& qos1,
                  const ::DDS::ReliabilityQosPolicy& qos2) 
{
  if (qos1.kind == qos2.kind
    && qos1.max_blocking_time == qos2.max_blocking_time)
  {
    return true;
  }
  return false;
}  
    

ACE_INLINE
bool operator == (const ::DDS::DestinationOrderQosPolicy& qos1,
                  const ::DDS::DestinationOrderQosPolicy& qos2) 
{
  if (qos1.kind == qos2.kind)
  {
    return true;
  }
  return false;
}  
      

ACE_INLINE
bool operator == (const ::DDS::HistoryQosPolicy& qos1,
                  const ::DDS::HistoryQosPolicy& qos2) 
{
  if (qos1.kind == qos2.kind
  && qos1.depth == qos2.depth)
  {
    return true;
  }
  return false;
}  
          

ACE_INLINE
bool operator == (const ::DDS::ResourceLimitsQosPolicy& qos1,
                  const ::DDS::ResourceLimitsQosPolicy& qos2) 
{
  if (qos1.max_samples == qos2.max_samples
  && qos1.max_instances == qos2.max_instances
  && qos1.max_samples_per_instance == qos2.max_samples_per_instance)
  {
    return true;
  }
  return false;
}  
    
    
ACE_INLINE
bool operator == (const ::DDS::EntityFactoryQosPolicy& qos1,
                  const ::DDS::EntityFactoryQosPolicy& qos2) 
{
  if (qos1.autoenable_created_entities == qos2.autoenable_created_entities)
  {
    return true;
  }
  return false;
}  


ACE_INLINE
bool operator == (const ::DDS::WriterDataLifecycleQosPolicy& qos1,
                  const ::DDS::WriterDataLifecycleQosPolicy& qos2) 
{
  if (qos1.autodispose_unregistered_instances == qos2.autodispose_unregistered_instances)
  {
    return true;
  }
  return false;
}  
    

ACE_INLINE
bool operator == (const ::DDS::ReaderDataLifecycleQosPolicy& qos1,
                  const ::DDS::ReaderDataLifecycleQosPolicy& qos2) 
{
  if (qos1.autopurge_nowriter_samples_delay == qos2.autopurge_nowriter_samples_delay)
  {
    return true;
  }
  return false;
}  


ACE_INLINE
bool operator ==  (const ::DDS::DomainParticipantQos& qos1,
                  const ::DDS::DomainParticipantQos& qos2) 
{
  if (qos1.user_data == qos2.user_data
    && qos1.entity_factory == qos2.entity_factory)
  {
    return true;
  }
  return false;
}  


ACE_INLINE
bool operator == (const ::DDS::TopicQos& qos1,
                  const ::DDS::TopicQos& qos2) 
{
  if (qos1.topic_data == qos2.topic_data
    && qos1.durability == qos2.durability
    && qos1.deadline == qos2.deadline
    && qos1.latency_budget == qos2.latency_budget
    && qos1.liveliness == qos2.liveliness
    && qos1.reliability == qos2.reliability
    && qos1.destination_order == qos2.destination_order
    && qos1.history == qos2.history
    && qos1.resource_limits == qos2.resource_limits
    && qos1.transport_priority == qos2.transport_priority
    && qos1.lifespan == qos2.lifespan
    && qos1.ownership == qos2.ownership)
  {
    return true;
  }
  return false;
}  


ACE_INLINE
bool operator == (const ::DDS::DataWriterQos& qos1,
                  const ::DDS::DataWriterQos& qos2) 
{
  if (qos1.durability == qos2.durability
    && qos1.deadline == qos2.deadline
    && qos1.latency_budget == qos2.latency_budget
    && qos1.liveliness == qos2.liveliness
    && qos1.reliability == qos2.reliability
    && qos1.destination_order == qos2.destination_order
    && qos1.history == qos2.history
    && qos1.resource_limits == qos2.resource_limits
    && qos1.transport_priority == qos2.transport_priority
    && qos1.lifespan == qos2.lifespan
    && qos1.user_data == qos2.user_data
    && qos1.ownership_strength == qos2.ownership_strength
    && qos1.writer_data_lifecycle == qos2.writer_data_lifecycle)
  {
    return true;
  }
  return false;
}      


ACE_INLINE
bool operator == (const ::DDS::PublisherQos& qos1,
                  const ::DDS::PublisherQos& qos2) 
{
  if (qos1.presentation == qos2.presentation
    && qos1.partition == qos2.partition
    && qos1.group_data == qos2.group_data
    && qos1.entity_factory == qos2.entity_factory)
  {
    return true;
  }
  return false;
}          
    

ACE_INLINE
bool operator == (const ::DDS::DataReaderQos& qos1,
                  const ::DDS::DataReaderQos& qos2) 
{
  if (qos1.durability == qos2.durability
    && qos1.deadline == qos2.deadline
    && qos1.latency_budget == qos2.latency_budget
    && qos1.liveliness == qos2.liveliness
    && qos1.reliability == qos2.reliability
    && qos1.destination_order == qos2.destination_order
    && qos1.history == qos2.history
    && qos1.resource_limits == qos2.resource_limits
    && qos1.user_data == qos2.user_data
    && qos1.time_based_filter == qos2.time_based_filter
    && qos1.reader_data_lifecycle == qos2.reader_data_lifecycle)
  {
    return true;
  }
  return false;
}      


ACE_INLINE
bool operator == (const ::DDS::SubscriberQos& qos1,
                  const ::DDS::SubscriberQos& qos2) 
{
  if (qos1.presentation == qos2.presentation
    && qos1.partition == qos2.partition
    && qos1.group_data == qos2.group_data
    && qos1.entity_factory == qos2.entity_factory)
  {
    return true;
  }
  return false;
}          

