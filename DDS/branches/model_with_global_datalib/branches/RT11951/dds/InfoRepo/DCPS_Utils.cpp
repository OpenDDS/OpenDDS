#include "DcpsInfo_pch.h"
#include /**/ "DCPS_Utils.h"
#include "dds/DCPS/Qos_Helper.h"

#include "ace/ACE.h"  /* For ACE::wild_match() */
// #include "ace/os_include/os_fnmatch.h"

namespace
{
  bool
  matching_partitions (::DDS::PartitionQosPolicy const & pub,
                       ::DDS::PartitionQosPolicy const & sub)
  {
    ::DDS::StringSeq const & publisher_partitions  = pub.name;
    ::DDS::StringSeq const & subscriber_partitions = sub.name;

    // Both publisher and subscriber are in the same (default)
    // partition if the partition string sequence lengths are both
    // zero.
    CORBA::ULong const pub_len = publisher_partitions.length ();
    CORBA::ULong const sub_len = subscriber_partitions.length ();
    if (pub_len == 0 && sub_len == 0)
      return true;

    bool match = false;
    // We currently don't support "[]" wildcards.  See pattern_match()
    // function above for details.
    static char const wildcard[] = "*?" /* "[[*?])" */;

    // @todo Really slow (O(n^2)) search.  Optimize if partition name
    //       sequences will potentially be very long.  If they remain
    //       short, optimizing may be unnecessary or simply not worth
    //       the trouble.
    for (CORBA::ULong i = 0; i < pub_len; ++i)
      {
        char const * const pname = publisher_partitions[i];

        // The DDS specification requires pattern matching
        // capabilities corresponding to those provided by the POSIX
        // fnmatch() function.  However, some platforms, such as
        // Windows, do not support such capabilities.  To be
        // completely portable across all platforms we instead use
        // ACE::wild_match().  Currently this prevents matching of
        // patterns containing square brackets, i.e. "[]" even though
        // some platforms support fnmatch().
        bool const pub_is_wildcard = ACE::wild_match (wildcard, pname);

        for (CORBA::ULong j = 0; j < sub_len; ++j)
          {
            char const * const sname = subscriber_partitions[j];

            bool const sub_is_wildcard = ACE::wild_match (wildcard, sname);

            if (pub_is_wildcard && sub_is_wildcard)
              continue;

            // @@ Is it really necessary to place the "pattern"
            //    argument to fnmatch() in the first parameter?
            if (pub_is_wildcard)
              match = ACE::wild_match (sname, pname);
            else if (sub_is_wildcard)
              match = ACE::wild_match (pname, sname);
            else
              match = (ACE_OS::strcmp (pname, sname) == 0);
          }
      }

    return match;
  }
}

void
increment_incompatibility_count (OpenDDS::DCPS::IncompatibleQosStatus* status,
                                 ::DDS::QosPolicyId_t incompatible_policy)
{
  ++status->total_count;
  ++status->count_since_last_send;
  status->last_policy_id = incompatible_policy;
  CORBA::ULong const size = status->policies.length();
  CORBA::ULong count = 0;
  bool updated = false;
  for ( ; !updated && count < size; ++count)
  {
    if (status->policies[count].policy_id == incompatible_policy)
    {
      ++status->policies[count].count;
      updated = true;
    }
  }

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
bool
compatibleQOS (DCPS_IR_Publication * publication,
               DCPS_IR_Subscription * subscription)
{
  bool compatible = true;

  if (publication->get_transport_id() != subscription->get_transport_id())
  {  
    // Transports are not compatible.
    compatible = false;
    increment_incompatibility_count(publication->get_incompatibleQosStatus(),
                                    ::DDS::TRANSPORTTYPE_QOS_POLICY_ID);
    increment_incompatibility_count(subscription->get_incompatibleQosStatus(),
                                    ::DDS::TRANSPORTTYPE_QOS_POLICY_ID);
    }

  ::DDS::DataWriterQos const * const writerQos =
    publication->get_datawriter_qos();
  ::DDS::DataReaderQos const * const readerQos =
      subscription->get_datareader_qos ();

  // Check the RELIABILITY_QOS_POLICY_ID
  if (writerQos->reliability.kind < readerQos->reliability.kind )
  {
    compatible = false;

    increment_incompatibility_count(publication->get_incompatibleQosStatus(),
                                    ::DDS::RELIABILITY_QOS_POLICY_ID);
    increment_incompatibility_count(subscription->get_incompatibleQosStatus(),
                                    ::DDS::RELIABILITY_QOS_POLICY_ID);
  }

  ::DDS::PublisherQos const * const pubQos =
      publication->get_publisher_qos ();
  ::DDS::SubscriberQos const * const subQos =
      subscription->get_subscriber_qos ();

  // Verify publisher and subscriber are in a matching partition.
  compatible =
    compatible && matching_partitions (pubQos->partition,
                                       subQos->partition);

    // According to the DDS spec:
    //
    //   Failure to match partitions is not considered an incompatible
    //   QoS and does not trigger any listeners nor conditions.
    //
    // Don't increment the incompatibity count.

  // Check the DURABILITY_QOS_POLICY_ID
  if ( writerQos->durability.kind < readerQos->durability.kind )
    {
      compatible = 0;

      increment_incompatibility_count(publication->get_incompatibleQosStatus(),
                                      ::DDS::DURABILITY_QOS_POLICY_ID);
      increment_incompatibility_count(subscription->get_incompatibleQosStatus(),
                                      ::DDS::DURABILITY_QOS_POLICY_ID);
    }

  // Check the LIVELINESS_QOS_POLICY_ID
  // invalid if offered kind is less than requested kind OR
  //         if offered liveliness duration greater than requested liveliness duration
  if (( writerQos->liveliness.kind < readerQos->liveliness.kind ) ||
      ( ::OpenDDS::DCPS::Qos_Helper::lease_greater_than(writerQos->liveliness,readerQos->liveliness)))
    {
      compatible = 0;

      increment_incompatibility_count(publication->get_incompatibleQosStatus(),
                                      ::DDS::LIVELINESS_QOS_POLICY_ID);
      increment_incompatibility_count(subscription->get_incompatibleQosStatus(),
                                      ::DDS::LIVELINESS_QOS_POLICY_ID);
    }

  return compatible;
}

