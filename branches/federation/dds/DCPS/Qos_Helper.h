// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_QOS_HELPER_H
#define TAO_DDS_DCPS_QOS_HELPER_H

#include "dds/DdsDcpsC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#if defined (ACE_MAJOR_VERSION)                                 \
  && (ACE_MAJOR_VERSION > 5                                     \
      || (ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION > 5)      \
      || (ACE_MAJOR_VERSION == 5                                \
          && ACE_MINOR_VERSION == 5                             \
          && ACE_BETA_VERSION > 3))
# include "ace/Truncate.h"
#endif  /* ACE >= 5.5.4 */

// Define a macro since we should NOT declare methods as extern and
// then define them as inline
#if defined(__ACE_INLINE__)
# define EXTERN_OR_INLINE_HELPER_METHOD ACE_INLINE
#else
# define EXTERN_OR_INLINE_HELPER_METHOD extern
#endif

namespace OpenDDS
{
  namespace DCPS
  {
    class Service_Participant;

    EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export 
    ACE_Time_Value time_to_time_value (const ::DDS::Time_t& t);

    EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
    ::DDS::Time_t time_value_to_time (const ACE_Time_Value& tv);

    EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export 
    ACE_Time_Value duration_to_time_value (const ::DDS::Duration_t& t);

    EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
    ::DDS::Duration_t time_value_to_duration (const ACE_Time_Value& tv);

    EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
    CORBA::Long get_instance_sample_list_depth (
      ::DDS::HistoryQosPolicyKind history,
      long                        history_depth,
      long                        max_samples_per_instance);

    /**
    * This class implements methods that verify whether a qos is
    * valid, consistent and changeable.
    *
    * valid - the values are in acceptable ranges without respect 
    *         to any other values.
    *
    * consistent - the values are consistent with each other.
    *         The spec sometimes calls this "compatible" but I
    *         this compatible should be reserved for matching
    *         QoS of subscriptions and publications.
    *         The spec is confusing in its inconsistency of the
    *         use of "compatible" and "consistent".
    *
    * The qos supported in current implementation:
    *         Liveliness  :   kind = AUTOMATIC
    *         Reliability :   kind = RELIABLE | BEST_EFFORT 
    *                         max_blocking_time
    *         History     :   kind = KEEP_ALL | KEEP_LAST
    *                         depth > 1
    *         RESOURCE_LIMITS : max_samples_per_instance
    *         
    * Other than these supported qos, any qos that is different from the 
    * initial value is invalid.
    * 
    * Note: 
    *    Since in the first implmenation of DSS in TAO a limited number
    *    of QoS values are allowed to be modified, the consistency 
    *    test on QoS settings have not been implemented to check future  
    *    "valid" QoS values.
    *
    * Note: 
    *    None of the supported QoS in the first implemenation are 
    *    changeable. The changed value will be checked per QoS table
    *    in ::DDS spec.
    */
    class OpenDDS_Dcps_Export Qos_Helper
    {
    public:

      static bool consistent (const ::DDS::DomainParticipantQos& qos);

      static bool consistent (const ::DDS::TopicQos            & qos);

      static bool consistent (const ::DDS::DataWriterQos       & qos);

      static bool consistent (const ::DDS::PublisherQos        & qos);

      static bool consistent (const ::DDS::DataReaderQos       & qos);

      static bool consistent (const ::DDS::SubscriberQos       & qos);

      static bool valid (const ::DDS::UserDataQosPolicy& qos);

      static bool valid (const ::DDS::TopicDataQosPolicy & qos);

      static bool valid (const ::DDS::GroupDataQosPolicy& qos);

      static bool valid (const ::DDS::TransportPriorityQosPolicy& qos);

      static bool valid (const ::DDS::LifespanQosPolicy& qos);

      static bool valid (const ::DDS::DurabilityQosPolicy& qos);

      static bool valid (const ::DDS::PresentationQosPolicy& qos);

      static bool valid (const ::DDS::DeadlineQosPolicy& qos);

      static bool valid (const ::DDS::LatencyBudgetQosPolicy& qos);

      static bool valid (const ::DDS::OwnershipQosPolicy& qos);

      static bool valid (const ::DDS::OwnershipStrengthQosPolicy& qos);

      static bool valid (const ::DDS::LivelinessQosPolicy& qos);

      static bool valid (const ::DDS::TimeBasedFilterQosPolicy& qos);

      static bool valid (const ::DDS::PartitionQosPolicy& qos);

      static bool valid (const ::DDS::ReliabilityQosPolicy& qos);

      static bool valid (const ::DDS::DestinationOrderQosPolicy& qos);

      static bool valid (const ::DDS::HistoryQosPolicy& qos);

      static bool valid (const ::DDS::ResourceLimitsQosPolicy& qos);    

      static bool valid (const ::DDS::EntityFactoryQosPolicy& qos);

      static bool valid (const ::DDS::WriterDataLifecycleQosPolicy& qos);   

      static bool valid (const ::DDS::ReaderDataLifecycleQosPolicy& qos);

      static bool valid (const ::DDS::DomainParticipantQos& qos);

      static bool valid (const ::DDS::TopicQos& qos);

      static bool valid (const ::DDS::DataWriterQos& qos);

      static bool valid (const ::DDS::PublisherQos& qos);    

      static bool valid (const ::DDS::DataReaderQos& qos);

      static bool valid (const ::DDS::SubscriberQos& qos);

      static bool changeable (const ::DDS::UserDataQosPolicy& qos1, 
                              const ::DDS::UserDataQosPolicy& qos2);
     
      static bool changeable (const ::DDS::TopicDataQosPolicy & qos1,
                              const ::DDS::TopicDataQosPolicy & qos2);

      static bool changeable (const ::DDS::GroupDataQosPolicy& qos1,
				                      const ::DDS::GroupDataQosPolicy& qos2);

      static bool changeable (const ::DDS::TransportPriorityQosPolicy& qos1,
                              const ::DDS::TransportPriorityQosPolicy& qos2);

      static bool changeable (const ::DDS::LifespanQosPolicy& qos1,
                              const ::DDS::LifespanQosPolicy& qos2);

      static bool changeable (const ::DDS::DurabilityQosPolicy& qos1,
                              const ::DDS::DurabilityQosPolicy& qos2);
        
      static bool changeable (const ::DDS::PresentationQosPolicy& qos1,
                              const ::DDS::PresentationQosPolicy& qos2);

      static bool changeable (const ::DDS::DeadlineQosPolicy& qos1,
                              const ::DDS::DeadlineQosPolicy& qos2);
      
      static bool changeable (const ::DDS::LatencyBudgetQosPolicy& qos1,
                              const ::DDS::LatencyBudgetQosPolicy& qos2);

      static bool changeable (const ::DDS::OwnershipQosPolicy& qos1,
                              const ::DDS::OwnershipQosPolicy& qos2);

      static bool changeable (const ::DDS::OwnershipStrengthQosPolicy& qos1,
                              const ::DDS::OwnershipStrengthQosPolicy& qos2);

      static bool changeable (const ::DDS::LivelinessQosPolicy& qos1,
                              const ::DDS::LivelinessQosPolicy& qos2);

      static bool changeable (const ::DDS::TimeBasedFilterQosPolicy& qos1,
                              const ::DDS::TimeBasedFilterQosPolicy& qos2);

      static bool changeable (const ::DDS::PartitionQosPolicy& qos1,
                              const ::DDS::PartitionQosPolicy& qos2);
      
      static bool changeable (const ::DDS::ReliabilityQosPolicy& qos1,
                                 const ::DDS::ReliabilityQosPolicy& qos2);
      
      static bool changeable (const ::DDS::DestinationOrderQosPolicy& qos1,
                              const ::DDS::DestinationOrderQosPolicy& qos2);
          
      static bool changeable (const ::DDS::HistoryQosPolicy& qos1,
                              const ::DDS::HistoryQosPolicy& qos2);

      static bool changeable (const ::DDS::ResourceLimitsQosPolicy& qos1,
                              const ::DDS::ResourceLimitsQosPolicy& qos2);
            
      static bool changeable (const ::DDS::EntityFactoryQosPolicy& qos1,
                              const ::DDS::EntityFactoryQosPolicy& qos2) ;

      static bool changeable (const ::DDS::WriterDataLifecycleQosPolicy& qos1,
                              const ::DDS::WriterDataLifecycleQosPolicy& qos2);

      static bool changeable (const ::DDS::ReaderDataLifecycleQosPolicy& qos1,
                              const ::DDS::ReaderDataLifecycleQosPolicy& qos2);

      static bool changeable (const ::DDS::DomainParticipantQos& qos1,
                              const ::DDS::DomainParticipantQos& qos2);
      
      static bool changeable (const ::DDS::TopicQos            & qos1,
                              const ::DDS::TopicQos            & qos2);
      
      static bool changeable (const ::DDS::DataWriterQos       & qos1,
                              const ::DDS::DataWriterQos       & qos2);
      
      static bool changeable (const ::DDS::PublisherQos        & qos1,
                              const ::DDS::PublisherQos        & qos2);
      
      static bool changeable (const ::DDS::DataReaderQos       & qos1,
                              const ::DDS::DataReaderQos       & qos2);
      
      static bool changeable (const ::DDS::SubscriberQos       & qos1,
                              const ::DDS::SubscriberQos       & qos2);

      // this only uses the duration portion of liveliness, not kind, to determine
      // if qos1 is greater than qos2
      static bool lease_greater_than (const ::DDS::LivelinessQosPolicy& qos1,
                                      const ::DDS::LivelinessQosPolicy& qos2);

    };

    // Convenience function to avoid introducing preprocessor
    // conditionals in more than one place.
    template<typename TO, typename FROM>
    inline TO truncate_cast (FROM val)
    {
#if defined (ACE_MAJOR_VERSION)
# if (ACE_MAJOR_VERSION > 5                                     \
      || (ACE_MAJOR_VERSION == 5 && ACE_MINOR_VERSION > 5)      \
      || (ACE_MAJOR_VERSION == 5                                \
          && ACE_MINOR_VERSION == 5                             \
          && ACE_BETA_VERSION > 6))
      // ACE_Utils::truncate_cast<> is only supported in ACE 5.5.7 or
      // better.
      return ACE_Utils::truncate_cast<TO> (val);
# elif (ACE_MAJOR_VERSION == 5     \
        && ACE_MINOR_VERSION == 5  \
        && ACE_BETA_VERSION > 3)
      // ACE_Utils::Truncate<> is only supported between ACE 5.5.4 and
      // 5.5.6.
      return ACE_Utils::Truncate<TO> (val);
# else
      // We're SoL.  Ideally, ACE_Utils::truncate_cast<> should be
      // backported to both TAO 1.4a and TAO 1.5a to avoid the need
      // for this convenience function altogether.
      return static_cast<TO> (val);
# endif
#else
      // We're SoL.  See above.
      return static_cast<TO> (val);
#endif
    }

  } // namespace DCPS
} // namespace OpenDDS


EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export 
bool operator== (const ::DDS::Duration_t& t1, 
		 const ::DDS::Duration_t& t2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator < (const ::DDS::Duration_t& t1, 
		 const ::DDS::Duration_t& t2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator <= (const ::DDS::Duration_t& t1, 
                  const ::DDS::Duration_t& t2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::UserDataQosPolicy& qos1, 
                  const ::DDS::UserDataQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::TopicDataQosPolicy & qos1, 
                  const ::DDS::TopicDataQosPolicy & qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::GroupDataQosPolicy& qos1, 
                  const ::DDS::GroupDataQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::TransportPriorityQosPolicy& qos1, 
                  const ::DDS::TransportPriorityQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::LifespanQosPolicy& qos1, 
                  const ::DDS::LifespanQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::DurabilityQosPolicy& qos1,
                  const ::DDS::DurabilityQosPolicy& qos2);    

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::PresentationQosPolicy& qos1,
                  const ::DDS::PresentationQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::DeadlineQosPolicy& qos1,
                  const ::DDS::DeadlineQosPolicy& qos2);
    
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::LatencyBudgetQosPolicy& qos1,
                  const ::DDS::LatencyBudgetQosPolicy& qos2);
    
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::OwnershipQosPolicy& qos1,
                  const ::DDS::OwnershipQosPolicy& qos2);
  
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::OwnershipStrengthQosPolicy& qos1,
                  const ::DDS::OwnershipStrengthQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::LivelinessQosPolicy& qos1,
                  const ::DDS::LivelinessQosPolicy& qos2);
    
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::TimeBasedFilterQosPolicy& qos1,
                  const ::DDS::TimeBasedFilterQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::PartitionQosPolicy& qos1,
                  const ::DDS::PartitionQosPolicy& qos2);
    
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::ReliabilityQosPolicy& qos1,
                  const ::DDS::ReliabilityQosPolicy& qos2);
    
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::DestinationOrderQosPolicy& qos1,
                  const ::DDS::DestinationOrderQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::HistoryQosPolicy& qos1,
                  const ::DDS::HistoryQosPolicy& qos2);
          
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::ResourceLimitsQosPolicy& qos1,
                  const ::DDS::ResourceLimitsQosPolicy& qos2);
    
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::EntityFactoryQosPolicy& qos1,
                  const ::DDS::EntityFactoryQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::WriterDataLifecycleQosPolicy& qos1,
                  const ::DDS::WriterDataLifecycleQosPolicy& qos2);   

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::ReaderDataLifecycleQosPolicy& qos1,
                  const ::DDS::ReaderDataLifecycleQosPolicy& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator ==  (const ::DDS::DomainParticipantQos& qos1,
                  const ::DDS::DomainParticipantQos& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::TopicQos& qos1,
                  const ::DDS::TopicQos& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::DataWriterQos& qos1,
                  const ::DDS::DataWriterQos& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::PublisherQos& qos1,
                  const ::DDS::PublisherQos& qos2);
    
EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::DataReaderQos& qos1,
                  const ::DDS::DataReaderQos& qos2);

EXTERN_OR_INLINE_HELPER_METHOD OpenDDS_Dcps_Export
bool operator == (const ::DDS::SubscriberQos& qos1,
                  const ::DDS::SubscriberQos& qos2);

#if defined(__ACE_INLINE__)
#include "Qos_Helper.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DDS_DCPS_QOS_HELPER_H */
