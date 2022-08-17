/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */


#ifndef OPENDDS_DCPS_RECORDER_H
#define OPENDDS_DCPS_RECORDER_H

#include "LocalObject.h"
#include "PoolAllocator.h"
#include "RawDataSample.h"
#include "RcHandle_T.h"

#include <dds/DdsDcpsInfrastructureC.h>

#include "XTypes/DynamicDataImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class Recorder;

/**
 * @class RecorderListener
 *
 * @brief Listener for handling callbacks from the Recorder
 *
 * This class is for handling callbacks from the Recorder object.
 */
class OpenDDS_Dcps_Export RecorderListener : public virtual RcObject {

public:
  virtual ~RecorderListener();
  /**
   *  Callback for when the Recorder receives a data sample.
   *  @param recorder Recorder that received the sample
   *  @param sample the received SAMPLE_DATA type sample
   *
   */
  virtual void on_sample_data_received(Recorder*            recorder,
                                       const RawDataSample& sample) = 0;

  /**
   *  Callback for when the Recorder is associated with a DataWriter.
   *  @param recorder Recorder that received the association
   */
  virtual void on_recorder_matched(Recorder*                              recorder,
                                   const DDS::SubscriptionMatchedStatus & status) = 0;
};

typedef RcHandle<RecorderListener> RecorderListener_rch;

typedef Recorder* Recorder_ptr;
typedef TAO_Objref_Var_T<Recorder> Recorder_var;

class OpenDDS_Dcps_Export Recorder
  : public virtual LocalObjectBase {
public:
  typedef Recorder_ptr _ptr_type;
  typedef Recorder_var _var_type;

  virtual ~Recorder();

  static Recorder_ptr _duplicate(Recorder_ptr obj);

#if !defined (DDS_HAS_MINIMUM_BIT)
  /**
   *  Find the bit key for a given repo id.
   */
  virtual DDS::ReturnCode_t repoid_to_bit_key(const DCPS::RepoId&     id,
                                              DDS::BuiltinTopicKey_t& key) = 0;
#endif

  /**
   * Set the Quality of Service settings for the Recorder.
   *
   */
  virtual DDS::ReturnCode_t set_qos (const DDS::SubscriberQos & subscriber_qos,
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
                                          DDS::StatusMask              mask = DEFAULT_STATUS_MASK )=0;

  /**
   * Get the listener for this Recorder.
   *
   */
  virtual RecorderListener_rch get_listener() = 0;

#ifndef OPENDDS_SAFETY_PROFILE
  virtual DDS::DynamicData_ptr get_dynamic_data(const RawDataSample& sample) = 0;
#endif

  virtual void check_encap(bool b) = 0;
  virtual bool check_encap() const = 0;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

namespace TAO {

template<>
struct OpenDDS_Dcps_Export Objref_Traits< ::OpenDDS::DCPS::Recorder> {
  static ::OpenDDS::DCPS::Recorder_ptr duplicate( ::OpenDDS::DCPS::Recorder_ptr p);
  static void release(::OpenDDS::DCPS::Recorder_ptr p);
  static ::OpenDDS::DCPS::Recorder_ptr nil();
  static bool marshal(const ::OpenDDS::DCPS::Recorder_ptr p, TAO_OutputCDR& cdr);
};

} // namespace TAO

TAO_END_VERSIONED_NAMESPACE_DECL


#endif /* end of include guard: OPENDDS_DCPS_RECORDER_H */
