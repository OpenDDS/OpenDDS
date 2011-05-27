#include "DcpsInfo_pch.h"
#include /**/ "DCPS_Utils.h"



int operator== (::DDS::Duration_t op1,
                ::DDS::Duration_t op2)
{
  return ( (op1.sec == op2.sec) && (op1.nanosec == op2.nanosec));
}


void increment_incompatibility_count (OpenDDS::DCPS::IncompatibleQosStatus* status,
                                      ::DDS::QosPolicyId_t incompatible_policy)
{
  status->total_count++;
  status->count_since_last_send++;
  status->last_policy_id = incompatible_policy;
  CORBA::ULong size = status->policies.length();
  CORBA::ULong count = 0;
  bool updated = false;
  while (!updated && count < size)
    {
      if (status->policies[count].policy_id == incompatible_policy)
        {
          status->policies[count].count++;
          updated = true;
        }
      ++count;
    };

  if (!updated)
    {
      ::DDS::QosPolicyCount policy;
      policy.policy_id = incompatible_policy;
      policy.count = 1;
      status->policies.length(count + 1);
      status->policies[count] = policy;
    }
}


// This is extremely simple now but will get very complex when more
// QOSes are supported.
int compatibleQOS (DCPS_IR_Publication * publication,
                   DCPS_IR_Subscription * subscription)
{
  const ::DDS::DataWriterQos * writerQos = publication->get_datawriter_qos();
  const ::DDS::PublisherQos * pubQos = publication->get_publisher_qos ();

  const ::DDS::DataReaderQos * readerQos = subscription->get_datareader_qos ();
  const ::DDS::SubscriberQos * subQos = subscription->get_subscriber_qos ();


  int compatible = 1;

  ACE_UNUSED_ARG( pubQos) ;
  ACE_UNUSED_ARG( subQos) ;
  if ( publication->get_transport_id() != subscription->get_transport_id())
    {  

      // Transports are not compatible.
      compatible = 0;
       increment_incompatibility_count(publication->get_incompatibleQosStatus(),
         ::DDS::TRANSPORTTYPE_QOS_POLICY_ID);
       increment_incompatibility_count(subscription->get_incompatibleQosStatus(),
         ::DDS::TRANSPORTTYPE_QOS_POLICY_ID);
    }

  // Check the RELIABILITY_QOS_POLICY_ID
  if ( writerQos->reliability.kind < readerQos->reliability.kind )
    {
      compatible = 0;

      increment_incompatibility_count(publication->get_incompatibleQosStatus(),
                                      ::DDS::RELIABILITY_QOS_POLICY_ID);
      increment_incompatibility_count(subscription->get_incompatibleQosStatus(),
                                      ::DDS::RELIABILITY_QOS_POLICY_ID);
    }

  return compatible;
}

