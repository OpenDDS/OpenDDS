/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
 
 
#ifndef OPENDDS_DCPS_RECORDER_H
#define OPENDDS_DCPS_RECORDER_H
 
#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/RawDataSample.h"

 
namespace OpenDDS {
namespace DCPS {
class Recorder;

class Recorder;
typedef RcHandle<Recorder> Recorder_rch;

/**
 * @class RecorderListener
 *
 * @brief Listener for handling callbacks from the Recorder
 *
 * This class is for handling callbacks from the Recorder object.
 */
class OpenDDS_Dcps_Export RecorderListener : public RcObject<ACE_SYNCH_MUTEX> {
  
public:
  virtual ~RecorderListener();
   /**
    *  Callback for when the Recorder receives a data sample.
    *  @param recorder Recorder that received the sample
    *  @param sample the received SAMPLE_DATA type sample
    *
    */
   virtual void on_sample_data_received(const Recorder_rch & writer,
                                const RawDataSample& sample)=0;

   /**
    *  Callback for when the Recorder is associated with a DataWriter.
    *  @param recorder Recorder that received the association
    */
   virtual void on_recorder_matched(const Recorder* recorder,
                            const ::DDS::SubscriptionMatchedStatus & status )=0;
};


typedef RcHandle<RecorderListener> RecorderListener_rch;

class OpenDDS_Dcps_Export Recorder : public RcObject<ACE_SYNCH_MUTEX> {
public:
  virtual ~Recorder();
  /**
   *  Find the bit key for a given repo id.
   */
  // virtual DDS::ReturnCode_t repoid_to_bit_key(const DCPS::RepoId& id,
  //                                     DDS::BuiltinTopicKey_t& key)=0;


  /**
   * Set the Quality of Service settings for the Recorder.
   *
   */
  virtual DDS::ReturnCode_t set_qos (const ::DDS::SubscriberQos & subscriber_qos,
                             const DDS::DataReaderQos & datareader_qos)=0;

  /**
   * Get the Quality of Service settings for the Recorder.
   *
   */
  virtual DDS::ReturnCode_t get_qos (DDS::SubscriberQos & subscriber_qos,
                             DDS::DataReaderQos & datareader_qos)=0;

  /**
   * Change the listener for this Recorder.
   *
   */
  virtual DDS::ReturnCode_t set_listener (const RecorderListener_rch & a_listener,
                                          DDS::StatusMask mask = DEFAULT_STATUS_MASK )=0;

  /**
   * Get the listener for this Recorder.
   *
   */
  virtual RecorderListener_rch get_listener ()=0;
};
 
} // namespace DCPS
} // namespace

#endif /* end of include guard: OPENDDS_DCPS_RECORDER_H */
 
 
 
 