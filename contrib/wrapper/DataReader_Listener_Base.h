// -*- C++ -*-

//=============================================================================
/**
 *  @file    DataReader_Listener_Base.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_DATAREADER_LISTENER_BASE_H_
#define DDS_WRAPPER_DATAREADER_LISTENER_BASE_H_

#include "wrapper_subscription.h"
#include "DDSWrapper_export.h"

/**
 * @class DataReader_Listener_Base
 *
 * @brief Implementation of the DataReaderListener interface that mainly
 *        provides empty callback method implementations so that they
 *        don't have to be redefined for each listener
 */
class DDSWrapper_Export DataReader_Listener_Base
  : public DDS::DataReaderListener
{
public:
  DataReader_Listener_Base ();

  virtual ~DataReader_Listener_Base (void);

  /// this callback is called when data for a topic arrives outside of the
  /// bounds specified by a DEADLINE policy.
  virtual void on_requested_deadline_missed (
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status);

  /// this callback is called when a data reader QoS policy value is
  /// incompatible with what is offered.
  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status);

  /// this callback is called when the liveliness of one or more DataWriter
  /// that were writing instances read through the DataReader has changed.
  virtual void on_liveliness_changed (
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status);

  /// this callback is called when the DataReader has found a DataWriter
  /// that matches the topic and has compatible QoS.
  virtual void on_subscription_matched (
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus & status);

  /// this callback is called when a (received) sample has been rejected.
  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  /// this callback is called when new information is available.
  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  /// this callback is called when a sample has been lost (never received).
  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);
};

#if defined (__ACE_INLINE__)
#include "DataReader_Listener_Base.inl"
#endif

#endif /* DDS_WRAPPER_DATAREADER_LISTENER_BASE_H_  */
