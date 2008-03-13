// -*- C++ -*-
//
// $Id$

// TMB - I had to add the following line
#include "Service_Participant.h"

#if defined (ACE_MAJOR_VERSION)                                 \
  && (ACE_MAJOR_VERSION > 5                                     \
      || (ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION > 5)      \
      || (ACE_MAJOR_VERSION == 5                                \
          && ACE_MINOR_VERSION == 5                             \
          && ACE_BETA_VERSION > 3))
# include "ace/Truncate.h"
#endif  /* ACE >= 5.5.4 */

namespace OpenDDS
{
  namespace DCPS
  {
    // Convenience function to avoid introducing preprocessor
    // conditionals in more than one place.
    ACE_INLINE CORBA::Long
    time_t_to_long (time_t t)
    {
#if defined (ACE_MAJOR_VERSION)
# if (ACE_MAJOR_VERSION > 5                                     \
      || (ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION > 5)      \
      || (ACE_MAJOR_VERSION == 5                                \
          && ACE_MINOR_VERSION == 5                             \
          && ACE_BETA_VERSION > 6))
      // ACE_Utils::truncate_cast<> is only supported in ACE 5.5.7 or
      // better.
      return ACE_Utils::truncate_cast<CORBA::Long> (t);
# elif (ACE_MAJOR_VERSION == 5     \
        && ACE_MINOR_VERSION == 5  \
        && ACE_BETA_VERSION > 3)
      // ACE_Utils::Truncate<> is only supported between ACE 5.5.4 and
      // 5.5.6.
      return ACE_Utils::Truncate<CORBA::Long> (t);
# else
      return static_cast<CORBA::Long> (t);
# endif
#else
      return static_cast<CORBA::Long> (t);
#endif
    }

    ACE_INLINE
    ACE_Time_Value time_to_time_value (const ::DDS::Time_t& t)
    {
      ACE_Time_Value tv (t.sec, t.nanosec * 1000);
      return tv;
    }

    ACE_INLINE
    ::DDS::Time_t time_value_to_time (const ACE_Time_Value& tv)
    {
      ::DDS::Time_t t;
      t.sec = OpenDDS::DCPS::time_t_to_long (tv.sec ());
      t.nanosec = tv.usec () * 1000;
      return t;
    }

    ACE_INLINE
    ACE_Time_Value duration_to_time_value (const ::DDS::Duration_t& t)
    {
      ACE_Time_Value tv (t.sec, t.nanosec * 1000);
      return tv;
    }

    ACE_INLINE
    ::DDS::Duration_t time_value_to_duration (const ACE_Time_Value& tv)
    {
      ::DDS::Duration_t t;

      t.sec = OpenDDS::DCPS::time_t_to_long (tv.sec ());

      t.nanosec = tv.usec () * 1000;
      return t;
    }

    ACE_INLINE
    bool
    Qos_Helper::consistent (const ::DDS::DomainParticipantQos& /* qos */)
    {
      return true;
    }


    ACE_INLINE
    bool
    Qos_Helper::consistent (const ::DDS::TopicQos            & qos)
    {
      bool const inconsistent =
	((qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED
	  && qos.history.depth > qos.resource_limits.max_samples_per_instance)
	 || (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED
	     && qos.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED
	     && qos.resource_limits.max_samples_per_instance > qos.resource_limits.max_samples));

      return !inconsistent;
    }


    ACE_INLINE
    bool
    Qos_Helper::consistent (const ::DDS::DataWriterQos       & qos)
    {
      bool const inconsistent =
	((qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
	  qos.history.depth > qos.resource_limits.max_samples_per_instance)
	 || (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED
	     && qos.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED
	     && qos.resource_limits.max_samples_per_instance > qos.resource_limits.max_samples));

      return !inconsistent;
    }


    ACE_INLINE
    bool
    Qos_Helper::consistent (const ::DDS::PublisherQos        & /* qos */)
    {
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

      bool const inconsistent =
	((qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED &&
	  qos.history.depth > qos.resource_limits.max_samples_per_instance)
	 || (qos.resource_limits.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED
	     && qos.resource_limits.max_samples != ::DDS::LENGTH_UNLIMITED
	     && qos.resource_limits.max_samples_per_instance > qos.resource_limits.max_samples));

      return !inconsistent;
    }


    ACE_INLINE
    bool
    Qos_Helper::consistent (const ::DDS::SubscriberQos       & /* qos */)
    {
      return true;
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::UserDataQosPolicy& qos)
    {
      return
	(qos.value == TheServiceParticipant->initial_UserDataQosPolicy().value);
    }


    // Note: Since in the first implmenation of DSS in TAO
    //       a limited number of QoS values are allowed to be
    //       modified, the validity tests are simplified to mostly
    //       being a check that they are the defaults defined in
    //       the specification.
    //       SO INVALID ALSO INCLUDES UNSUPPORTED QoS.
    // TBD - when QoS become support the valid checks should check
    //       the ranges of the values of the QoS.
    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::TopicDataQosPolicy & qos)
    {
      return (qos == TheServiceParticipant->initial_TopicDataQosPolicy());
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::GroupDataQosPolicy& qos)
    {
      return (qos == TheServiceParticipant->initial_GroupDataQosPolicy());
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::TransportPriorityQosPolicy& qos)
    {
      return
	(qos == TheServiceParticipant->initial_TransportPriorityQosPolicy());
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::LifespanQosPolicy& qos)
    {
      return (qos == TheServiceParticipant->initial_LifespanQosPolicy());
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DurabilityQosPolicy& qos)
    {
      return
	(qos.kind == ::DDS::VOLATILE_DURABILITY_QOS //TheServiceParticipant->initial_DurabilityQosPolicy()
	 || qos.kind == ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::PresentationQosPolicy& qos)
    {
      return (qos == TheServiceParticipant->initial_PresentationQosPolicy());
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DeadlineQosPolicy& qos)
    {
      return (qos == TheServiceParticipant->initial_DeadlineQosPolicy());
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::LatencyBudgetQosPolicy& qos)
    {
      return (qos == TheServiceParticipant->initial_LatencyBudgetQosPolicy());
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::OwnershipQosPolicy& qos)
    {
      return (qos == TheServiceParticipant->initial_OwnershipQosPolicy());
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::OwnershipStrengthQosPolicy& qos)
    {
      return
	(qos == TheServiceParticipant->initial_OwnershipStrengthQosPolicy());
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
      return (qos.kind == ::DDS::AUTOMATIC_LIVELINESS_QOS);
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::TimeBasedFilterQosPolicy& qos)
    {
      return
	(qos == TheServiceParticipant->initial_TimeBasedFilterQosPolicy());
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::PartitionQosPolicy& /* qos */)
    {
      return true;
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::ReliabilityQosPolicy& qos)
    {
      return (qos.kind == ::DDS::BEST_EFFORT_RELIABILITY_QOS
	      || (qos.kind == ::DDS::RELIABLE_RELIABILITY_QOS));
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DestinationOrderQosPolicy& qos)
    {
      return
	(qos == TheServiceParticipant->initial_DestinationOrderQosPolicy());
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::HistoryQosPolicy& qos)
    {
      if (qos.depth != ::DDS::LENGTH_UNLIMITED &&
          qos.depth < 1)
        return false; // invalid depth

      return (qos.kind == ::DDS::KEEP_LAST_HISTORY_QOS
	      || qos.kind == ::DDS::KEEP_ALL_HISTORY_QOS);
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::ResourceLimitsQosPolicy& qos)
    {
      bool const invalid =
	((qos.max_samples != ::DDS::LENGTH_UNLIMITED
	   && qos.max_samples <= 0)
	  || (qos.max_instances != ::DDS::LENGTH_UNLIMITED
	      && qos.max_instances <= 0)
	  || (qos.max_samples_per_instance != ::DDS::LENGTH_UNLIMITED
	      && qos.max_samples_per_instance <= 0));

      return !invalid;
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::EntityFactoryQosPolicy& qos)
    {
      return (qos == TheServiceParticipant->initial_EntityFactoryQosPolicy());
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::WriterDataLifecycleQosPolicy& qos)
    {
      return
	(qos == TheServiceParticipant->initial_WriterDataLifecycleQosPolicy());
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::ReaderDataLifecycleQosPolicy& qos)
    {
      return
	(qos == TheServiceParticipant->initial_ReaderDataLifecycleQosPolicy());
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DomainParticipantQos& qos)
    {
      return (valid(qos.user_data) && valid(qos.entity_factory));
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::TopicQos& qos)
    {
      return (valid(qos.topic_data)
	      && valid(qos.durability)
	      && valid(qos.deadline)
	      && valid(qos.latency_budget)
	      && valid(qos.liveliness)
	      && valid(qos.destination_order)
	      && valid(qos.history)
	      && valid(qos.resource_limits)
	      && valid(qos.transport_priority)
	      && valid(qos.lifespan)
	      && valid(qos.ownership));
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DataWriterQos& qos)
    {
      return  (valid(qos.durability)
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
	       && valid(qos.writer_data_lifecycle));
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::PublisherQos& qos)
    {
      return (valid(qos.presentation)
	      && valid(qos.partition)
	      && valid(qos.group_data)
	      && valid(qos.entity_factory));
    }


    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::DataReaderQos& qos)
    {
      return (valid(qos.durability)
	      && valid(qos.deadline)
	      && valid(qos.latency_budget)
	      && valid(qos.liveliness)
	      && valid(qos.reliability)
	      && valid(qos.destination_order)
	      && valid(qos.history)
	      && valid(qos.resource_limits)
	      && valid(qos.user_data)
	      && valid(qos.time_based_filter)
	      && valid(qos.reader_data_lifecycle));
    }

    ACE_INLINE
    bool Qos_Helper::valid (const ::DDS::SubscriberQos& qos)
    {
      return (valid(qos.presentation)
	      && valid(qos.partition)
	      && valid(qos.group_data)
	      && valid(qos.entity_factory));
    }

    ACE_INLINE
    bool
    Qos_Helper::changeable (const ::DDS::DomainParticipantQos& /* qos1 */,
                            const ::DDS::DomainParticipantQos& /* qos2 */)
    {
      // TBD - check changed values are changeable
      //       per QoS table in ::DDS spec.
      return false;
    }


    ACE_INLINE
    bool
    Qos_Helper::changeable (const ::DDS::TopicQos            & qos1,
                            const ::DDS::TopicQos            & qos2)
    {
      // TBD - check changed values are changeable
      //       per QoS table in ::DDS spec.
      // The qos currently supported
      // (RELIABILITY/HISTORY/RESOURCE_LIMITS) are not changeable.
      return (qos1 == qos2);
    }


    ACE_INLINE
    bool
    Qos_Helper::changeable (const ::DDS::DataWriterQos       & qos1,
                            const ::DDS::DataWriterQos       & qos2)
    {
      // TBD - check changed values are changeable
      //       per QoS table in ::DDS spec.
      // The qos currently supported
      // (RELIABILITY/HISTORY/RESOURCE_LIMITS) are not changeable.
      return (qos1 == qos2);
    }


    ACE_INLINE
    bool
    Qos_Helper::changeable (const ::DDS::PublisherQos        & qos1,
                            const ::DDS::PublisherQos        & qos2)
    {
      // TBD - check changed values are changeable
      //       per QoS table in ::DDS spec.
      return (qos1 == qos2);
    }


    ACE_INLINE
    bool
    Qos_Helper::changeable (const ::DDS::DataReaderQos       & qos1,
                            const ::DDS::DataReaderQos       & qos2)
    {
      // TBD - check changed values are changeable
      //       per QoS table in ::DDS spec.
      // The qos currently supported
      // (RELIABILITY/HISTORY/RESOURCE_LIMITS) are not changeable.
      return (qos1 == qos2);
    }


    ACE_INLINE
    bool
    Qos_Helper::changeable (const ::DDS::SubscriberQos       & qos1,
                            const ::DDS::SubscriberQos       & qos2)
    {
      // TBD - check changed values are changeable
      //       per QoS table in ::DDS spec.
      return (qos1 == qos2);
    }

  } // namespace ::DDS
} // namespace OpenDDS


ACE_INLINE
bool operator== (const ::DDS::Duration_t& t1,
                 const ::DDS::Duration_t& t2)
{
  return (t1.sec == t2.sec && t1.nanosec == t2.nanosec);
}


ACE_INLINE
bool operator < (const ::DDS::Duration_t& t1,
                  const ::DDS::Duration_t& t2)
{
  return (t1.sec < t2.sec
	  || (t1.sec == t2.sec && t1.nanosec < t2.nanosec));
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
  return (qos1.value == qos2.value);
}


ACE_INLINE
bool operator == (const ::DDS::TopicDataQosPolicy & qos1,
                  const ::DDS::TopicDataQosPolicy & qos2)
{
  return (qos1.value == qos2.value);
}


ACE_INLINE
bool operator == (const ::DDS::GroupDataQosPolicy& qos1,
                  const ::DDS::GroupDataQosPolicy& qos2)
{
  return (qos1.value == qos2.value);
}


ACE_INLINE
bool operator == (const ::DDS::TransportPriorityQosPolicy& qos1,
                  const ::DDS::TransportPriorityQosPolicy& qos2)
{
  return (qos1.value == qos2.value);
}


ACE_INLINE
bool operator == (const ::DDS::LifespanQosPolicy& qos1,
                  const ::DDS::LifespanQosPolicy& qos2)
{
  return (qos1.duration == qos2.duration);
}


ACE_INLINE
bool operator == (const ::DDS::DurabilityQosPolicy& qos1,
                  const ::DDS::DurabilityQosPolicy& qos2)
{
  return (qos1.kind == qos2.kind
	  && qos1.service_cleanup_delay == qos2.service_cleanup_delay);
}


ACE_INLINE
bool operator == (const ::DDS::PresentationQosPolicy& qos1,
                  const ::DDS::PresentationQosPolicy& qos2)
{
  return (qos1.access_scope == qos2.access_scope
	  && qos1.coherent_access == qos2.coherent_access
	  && qos1.ordered_access == qos2.ordered_access);
}


ACE_INLINE
bool operator == (const ::DDS::DeadlineQosPolicy& qos1,
                  const ::DDS::DeadlineQosPolicy& qos2)
{
  return (qos1.period == qos2.period);
}


ACE_INLINE
bool operator == (const ::DDS::LatencyBudgetQosPolicy& qos1,
                  const ::DDS::LatencyBudgetQosPolicy& qos2)
{
  return (qos1.duration == qos2.duration);
}


ACE_INLINE
bool operator == (const ::DDS::OwnershipQosPolicy& qos1,
                  const ::DDS::OwnershipQosPolicy& qos2)
{
  return (qos1.kind == qos2.kind);
}


ACE_INLINE
bool operator == (const ::DDS::OwnershipStrengthQosPolicy& qos1,
                  const ::DDS::OwnershipStrengthQosPolicy& qos2)
{
  return (qos1.value == qos2.value);
}


ACE_INLINE
bool operator == (const ::DDS::LivelinessQosPolicy& qos1,
                  const ::DDS::LivelinessQosPolicy& qos2)
{
  return (qos1.kind == qos2.kind
	  && qos1.lease_duration == qos2.lease_duration);
}


ACE_INLINE
bool operator == (const ::DDS::TimeBasedFilterQosPolicy& qos1,
                  const ::DDS::TimeBasedFilterQosPolicy& qos2)
{
  return (qos1.minimum_separation == qos2.minimum_separation);
}


ACE_INLINE
bool operator == (const ::DDS::PartitionQosPolicy& qos1,
                  const ::DDS::PartitionQosPolicy& qos2)
{
  CORBA::ULong const len = qos1.name.length();
  if (len == qos2.name.length())
  {
    for(CORBA::ULong i = 0; i < len; ++i)
    {
      if (qos1.name[i] != qos2.name[i])
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
  return (qos1.kind == qos2.kind
	  && qos1.max_blocking_time == qos2.max_blocking_time);
}


ACE_INLINE
bool operator == (const ::DDS::DestinationOrderQosPolicy& qos1,
                  const ::DDS::DestinationOrderQosPolicy& qos2)
{
  return (qos1.kind == qos2.kind);
}


ACE_INLINE
bool operator == (const ::DDS::HistoryQosPolicy& qos1,
                  const ::DDS::HistoryQosPolicy& qos2)
{
  return (qos1.kind == qos2.kind
	  && qos1.depth == qos2.depth);
}


ACE_INLINE
bool operator == (const ::DDS::ResourceLimitsQosPolicy& qos1,
                  const ::DDS::ResourceLimitsQosPolicy& qos2)
{
  return (qos1.max_samples == qos2.max_samples
	  && qos1.max_instances == qos2.max_instances
	  && qos1.max_samples_per_instance == qos2.max_samples_per_instance);
}


ACE_INLINE
bool operator == (const ::DDS::EntityFactoryQosPolicy& qos1,
                  const ::DDS::EntityFactoryQosPolicy& qos2)
{
  return
    (qos1.autoenable_created_entities == qos2.autoenable_created_entities);
}


ACE_INLINE
bool operator == (const ::DDS::WriterDataLifecycleQosPolicy& qos1,
                  const ::DDS::WriterDataLifecycleQosPolicy& qos2)
{
  return
    (qos1.autodispose_unregistered_instances == qos2.autodispose_unregistered_instances);
}


ACE_INLINE
bool operator == (const ::DDS::ReaderDataLifecycleQosPolicy& qos1,
                  const ::DDS::ReaderDataLifecycleQosPolicy& qos2)
{
  return
    (qos1.autopurge_nowriter_samples_delay == qos2.autopurge_nowriter_samples_delay);
}


ACE_INLINE
bool operator ==  (const ::DDS::DomainParticipantQos& qos1,
		   const ::DDS::DomainParticipantQos& qos2)
{
  return (qos1.user_data == qos2.user_data
	  && qos1.entity_factory == qos2.entity_factory);
}


ACE_INLINE
bool operator == (const ::DDS::TopicQos& qos1,
                  const ::DDS::TopicQos& qos2)
{
  return (qos1.topic_data == qos2.topic_data
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
	  && qos1.ownership == qos2.ownership);
}


ACE_INLINE
bool operator == (const ::DDS::DataWriterQos& qos1,
                  const ::DDS::DataWriterQos& qos2)
{
  return (qos1.durability == qos2.durability
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
	  && qos1.writer_data_lifecycle == qos2.writer_data_lifecycle);
}


ACE_INLINE
bool operator == (const ::DDS::PublisherQos& qos1,
                  const ::DDS::PublisherQos& qos2)
{
  return (qos1.presentation == qos2.presentation
	  && qos1.partition == qos2.partition
	  && qos1.group_data == qos2.group_data
	  && qos1.entity_factory == qos2.entity_factory);
}


ACE_INLINE
bool operator == (const ::DDS::DataReaderQos& qos1,
                  const ::DDS::DataReaderQos& qos2)
{
  return (qos1.durability == qos2.durability
	  && qos1.deadline == qos2.deadline
	  && qos1.latency_budget == qos2.latency_budget
	  && qos1.liveliness == qos2.liveliness
	  && qos1.reliability == qos2.reliability
	  && qos1.destination_order == qos2.destination_order
	  && qos1.history == qos2.history
	  && qos1.resource_limits == qos2.resource_limits
	  && qos1.user_data == qos2.user_data
	  && qos1.time_based_filter == qos2.time_based_filter
	  && qos1.reader_data_lifecycle == qos2.reader_data_lifecycle);
}


ACE_INLINE
bool operator == (const ::DDS::SubscriberQos& qos1,
                  const ::DDS::SubscriberQos& qos2)
{
  return (qos1.presentation == qos2.presentation
	  && qos1.partition == qos2.partition
	  && qos1.group_data == qos2.group_data
	  && qos1.entity_factory == qos2.entity_factory);
}
