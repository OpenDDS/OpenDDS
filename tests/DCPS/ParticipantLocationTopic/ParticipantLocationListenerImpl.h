/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef PARTICIPANT_LOCATION_LISTENER_IMPL
#define PARTICIPANT_LOCATION_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/GuidUtils.h>

#include <map>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class ParticipantLocationListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  //Constructor
  explicit ParticipantLocationListenerImpl(const std::string& id);

  //Destructor
  virtual ~ParticipantLocationListenerImpl();

  virtual void on_requested_deadline_missed(
                                            DDS::DataReader_ptr reader,
                                            const DDS::RequestedDeadlineMissedStatus & status);

  virtual void on_requested_incompatible_qos(
                                             DDS::DataReader_ptr reader,
                                             const DDS::RequestedIncompatibleQosStatus & status);

  virtual void on_liveliness_changed(
                                     DDS::DataReader_ptr reader,
                                     const DDS::LivelinessChangedStatus & status);

  virtual void on_subscription_matched(
                                       DDS::DataReader_ptr reader,
                                       const DDS::SubscriptionMatchedStatus & status);

  virtual void on_sample_rejected(
                                  DDS::DataReader_ptr reader,
                                  const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
                                 DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
                              DDS::DataReader_ptr reader,
                              const DDS::SampleLostStatus& status);

  bool check(bool no_ice);

  typedef std::map<OpenDDS::DCPS::RepoId, unsigned long, OpenDDS::DCPS::GUID_tKeyLessThan> LocationMapType;
  LocationMapType location_map;

private:
  const std::string id_;

};

#endif /* PARTICIPANT_LOCATION_LISTENER_IMPL  */
