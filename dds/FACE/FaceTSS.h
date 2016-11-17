#ifndef OPENDDS_FACE_TSS_H
#define OPENDDS_FACE_TSS_H

#include "FACE/TS.hpp"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/SafetyProfileStreams.h"
#include "ace/Singleton.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace FaceTSS {

class Entities {
  friend class ACE_Singleton<Entities, ACE_Thread_Mutex>;

  Entities();
  ~Entities();

public:
  struct DDSAdapter : public OpenDDS::DCPS::PoolAllocationBase {
    DDSAdapter ()
      : status_valid(FACE::INVALID)
    {}

    FACE::VALIDITY_TYPE status_valid;
  };
  struct FaceSender : public DDSAdapter {
    FaceSender () {}
    DDS::DataWriter_var dw;
  };

  struct FaceReceiver : public DDSAdapter {
    FaceReceiver ()
      : last_msg_tid(0),
        sum_recvd_msgs_latency(0),
        total_msgs_recvd(0)
    {}

    virtual ~FaceReceiver() {}
    virtual FACE::RETURN_CODE_TYPE messages_waiting(FACE::WAITING_RANGE_TYPE& /*num_waiting*/)
    {
      return FACE::NOT_AVAILABLE;
    };
    DDS::DataReader_var dr;
    FACE::TS::MessageHeader last_msg_header;
    FACE::TRANSACTION_ID_TYPE last_msg_tid;
    FACE::SYSTEM_TIME_TYPE sum_recvd_msgs_latency;
    FACE::LongLong total_msgs_recvd;
  };

  template<typename Msg>
  class DDSTypedAdapter : public FaceReceiver {
  public:
    DDSTypedAdapter(FaceReceiver& rcvr);
    ~DDSTypedAdapter();
    virtual FACE::RETURN_CODE_TYPE messages_waiting(FACE::WAITING_RANGE_TYPE& num_waiting);
    typedef typename DCPS::DDSTraits<Msg>::DataReaderType DataReader;
  };

  typedef OPENDDS_MAP(FACE::CONNECTION_ID_TYPE, FaceSender) ConnIdToSenderMap;
  typedef OPENDDS_MAP(FACE::CONNECTION_ID_TYPE, FaceReceiver*) ConnIdToReceiverMap;

  OpenDDS_FACE_Export static Entities* instance();
  ConnIdToSenderMap senders_;
  ConnIdToReceiverMap receivers_;

  struct ConnectionInfo {
    OPENDDS_STRING connection_name;
    FACE::TRANSPORT_CONNECTION_STATUS_TYPE connection_status;
    FACE::MESSAGE_TYPE_GUID platform_view_guid;
  };
  OPENDDS_MAP(FACE::CONNECTION_ID_TYPE, ConnectionInfo ) connections_;
};

OpenDDS_FACE_Export DDS::Duration_t convertTimeout(FACE::TIMEOUT_TYPE timeout);
OpenDDS_FACE_Export FACE::SYSTEM_TIME_TYPE convertDuration(const DDS::Duration_t& duration);
OpenDDS_FACE_Export FACE::SYSTEM_TIME_TYPE convertTime(const DDS::Time_t& timestamp);
OpenDDS_FACE_Export void populate_header_received(const FACE::CONNECTION_ID_TYPE& connection_id,
                                                  const DDS::DomainParticipant_var part,
                                                  const DDS::SampleInfo& sinfo,
                                                  FACE::RETURN_CODE_TYPE& return_code);

OpenDDS_FACE_Export FACE::MESSAGE_INSTANCE_GUID create_message_instance_guid(const OpenDDS::DCPS::RepoId& pub,
                                                                             const CORBA::LongLong& seq);

OpenDDS_FACE_Export FACE::RETURN_CODE_TYPE update_status(FACE::CONNECTION_ID_TYPE connection_id,
    DDS::ReturnCode_t retcode);

template <typename Msg>
Entities::DDSTypedAdapter<Msg>::DDSTypedAdapter(FaceReceiver& rcvr)
  : FaceReceiver()
{
  dr = rcvr.dr;
  last_msg_header = rcvr.last_msg_header;
  last_msg_tid = rcvr.last_msg_tid;
  sum_recvd_msgs_latency = rcvr.sum_recvd_msgs_latency;
  total_msgs_recvd = rcvr.total_msgs_recvd;
}

template <typename Msg>
Entities::DDSTypedAdapter<Msg>::~DDSTypedAdapter()
{
}

template <typename Msg>
FACE::RETURN_CODE_TYPE Entities::DDSTypedAdapter<Msg>::messages_waiting(FACE::WAITING_RANGE_TYPE& num_waiting)
{
  const typename DataReader::_var_type typedReader =
    DataReader::_narrow(dr);
  if (!typedReader) {
    return FACE::INVALID_PARAM;
  }
  const DDS::ReadCondition_var rc =
      typedReader->create_readcondition(DDS::ANY_SAMPLE_STATE,
                                DDS::ANY_VIEW_STATE,
                                DDS::ALIVE_INSTANCE_STATE);

  DDS::ReturnCode_t ret;
  typename DCPS::DDSTraits<Msg>::MessageSequenceType seq;
  DDS::SampleInfoSeq sinfo;
  FACE::WAITING_RANGE_TYPE valid_waiting = 0;
  ret = typedReader->read_w_condition(seq, sinfo, DDS::LENGTH_UNLIMITED, rc);
  if (ret == DDS::RETCODE_OK) {
    for (CORBA::ULong i = 0; i < seq.length(); ++i) {
      if (sinfo[i].valid_data) {
        ++valid_waiting;
      }
    }
    num_waiting = valid_waiting;
    return FACE::RC_NO_ERROR;
  } else if (ret == DDS::RETCODE_NO_DATA) {
    num_waiting = 0;
    return FACE::RC_NO_ERROR;
  }
  return FACE::NOT_AVAILABLE;
}

template <typename Msg>
void receive_message(/*in*/    FACE::CONNECTION_ID_TYPE connection_id,
                     /*in*/    FACE::TIMEOUT_TYPE timeout,
                     /*inout*/ FACE::TRANSACTION_ID_TYPE& transaction_id,
                     /*inout*/ Msg& message,
                     /*in*/    FACE::MESSAGE_SIZE_TYPE message_size,
                     /*out*/   FACE::RETURN_CODE_TYPE& return_code)
{
  Entities::ConnIdToReceiverMap& readers = Entities::instance()->receivers_;
  if (!readers.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  if(!Entities::instance()->connections_.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  FACE::TRANSPORT_CONNECTION_STATUS_TYPE status =
    Entities::instance()->connections_[connection_id].connection_status;
  if (message_size < status.MAX_MESSAGE_SIZE) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  typedef typename DCPS::DDSTraits<Msg>::DataReaderType DataReader;
  const typename DataReader::_var_type typedReader =
    DataReader::_narrow(readers[connection_id]->dr);
  if (!typedReader) {
    return_code = update_status(connection_id, DDS::RETCODE_BAD_PARAMETER);
    return;
  }
  if (readers[connection_id]->status_valid != FACE::VALID) {
    Entities::FaceReceiver* tmp = readers[connection_id];
    readers[connection_id] = new Entities::DDSTypedAdapter<Msg>(*readers[connection_id]);
    delete tmp;
  }
  readers[connection_id]->status_valid = FACE::VALID;

  const DDS::ReadCondition_var rc =
    typedReader->create_readcondition(DDS::ANY_SAMPLE_STATE,
                                      DDS::ANY_VIEW_STATE,
                                      DDS::ALIVE_INSTANCE_STATE);
  const DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(rc);

  DDS::ConditionSeq active;
  const DDS::Duration_t ddsTimeout = convertTimeout(timeout);
  DDS::ReturnCode_t ret = ws->wait(active, ddsTimeout);
  ws->detach_condition(rc);

  if (ret == DDS::RETCODE_TIMEOUT) {
    return_code = update_status(connection_id, ret);
    return;
  }

  typename DCPS::DDSTraits<Msg>::MessageSequenceType seq;
  DDS::SampleInfoSeq sinfo;
  ret = typedReader->take_w_condition(seq, sinfo, 1 /*max*/, rc);
  if (ret == DDS::RETCODE_OK && sinfo[0].valid_data) {
    DDS::DomainParticipant_var participant = typedReader->get_subscriber()->get_participant();
    FACE::RETURN_CODE_TYPE ret_code;
    populate_header_received(connection_id, participant, sinfo[0], ret_code);
    if (ret_code != FACE::RC_NO_ERROR) {
      return_code = update_status(connection_id, ret_code);
      return;
    }

    transaction_id = ++readers[connection_id]->last_msg_tid;

    message = seq[0];
    return_code = update_status(connection_id, ret);
    return;
  }
  return_code = update_status(connection_id, DDS::RETCODE_NO_DATA);
}

template <typename Msg>
void send_message(FACE::CONNECTION_ID_TYPE connection_id,
                  FACE::TIMEOUT_TYPE timeout,
                  FACE::TRANSACTION_ID_TYPE& /*transaction_id*/,
                  const Msg& message,
                  FACE::MESSAGE_SIZE_TYPE message_size,
                  FACE::RETURN_CODE_TYPE& return_code)
{
  if(!Entities::instance()->connections_.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  FACE::TRANSPORT_CONNECTION_STATUS_TYPE status =
    Entities::instance()->connections_[connection_id].connection_status;
  if (message_size < status.MAX_MESSAGE_SIZE) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  Entities::ConnIdToSenderMap& writers = Entities::instance()->senders_;
  if (!writers.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }

  typedef typename DCPS::DDSTraits<Msg>::DataWriterType DataWriter;
  const typename DataWriter::_var_type typedWriter =
    DataWriter::_narrow(writers[connection_id].dw);
  if (!typedWriter) {
    return_code = update_status(connection_id, DDS::RETCODE_BAD_PARAMETER);
    return;
  }
  writers[connection_id].status_valid = FACE::VALID;

  DDS::DataWriterQos dw_qos;
  typedWriter->get_qos(dw_qos);
  FACE::SYSTEM_TIME_TYPE max_blocking_time = convertDuration(dw_qos.reliability.max_blocking_time);
  if (dw_qos.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS &&
      timeout != FACE::INF_TIME_VALUE &&
      ((max_blocking_time == FACE::INF_TIME_VALUE) || (timeout < max_blocking_time))) {
    return_code = update_status(connection_id, DDS::RETCODE_BAD_PARAMETER);
    return;
  }

  return_code = update_status(connection_id, typedWriter->write(message, DDS::HANDLE_NIL));
}

template <typename Msg>
class Listener : public DCPS::LocalObject<DDS::DataReaderListener> {
public:
  typedef void (*Callback)(FACE::TRANSACTION_ID_TYPE, Msg&,
                           FACE::MESSAGE_TYPE_GUID,
                           FACE::MESSAGE_SIZE_TYPE,
                           const FACE::WAITSET_TYPE,
                           FACE::RETURN_CODE_TYPE&);

  Listener(Callback callback, FACE::CONNECTION_ID_TYPE connection_id)
    : connection_id_(connection_id)
  {
    callbacks_.push_back(callback);
  }

  void add_callback(Callback callback) {
    GuardType guard(callbacks_lock_);
    callbacks_.push_back(callback);
  }

private:
  void on_requested_deadline_missed(DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus&) {}

  void on_requested_incompatible_qos(DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus&) {}

  void on_sample_rejected(DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&) {}

  void on_liveliness_changed(DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus&) {}

  void on_subscription_matched(DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus&) {}

  void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&) {}

  void on_data_available(DDS::DataReader_ptr reader)
  {
    typedef typename DCPS::DDSTraits<Msg>::DataReaderType DataReader;
    const typename DataReader::_var_type typedReader =
      DataReader::_narrow(reader);
    if (!typedReader) {
      update_status(connection_id_, DDS::RETCODE_BAD_PARAMETER);
      return;
    }

    FACE::MESSAGE_TYPE_GUID& msg_id = Entities::instance()->connections_[connection_id_].platform_view_guid;
    Msg sample;
    DDS::SampleInfo sinfo;
    while (typedReader->take_next_sample(sample, sinfo) == DDS::RETCODE_OK) {
      if (sinfo.valid_data) {
        DDS::DomainParticipant_var participant = typedReader->get_subscriber()->get_participant();
        FACE::RETURN_CODE_TYPE ret_code;
        populate_header_received(connection_id_, participant, sinfo, ret_code);
        if (ret_code != FACE::RC_NO_ERROR) {
          update_status(connection_id_, ret_code);
          return;
        }

        FACE::TRANSACTION_ID_TYPE transaction_id = ++Entities::instance()->receivers_[connection_id_]->last_msg_tid;
        update_status(connection_id_, DDS::RETCODE_OK);
        FACE::RETURN_CODE_TYPE retcode;
        GuardType guard(callbacks_lock_);
        if (OpenDDS::DCPS::DCPS_debug_level > 3) {
          ACE_DEBUG((LM_DEBUG, "Listener::on_data_available - invoking %d callbacks\n", callbacks_.size()));
        }
        for (size_t i = 0; i < callbacks_.size(); ++i) {
          retcode = FACE::RC_NO_ERROR;
          callbacks_.at(i)(transaction_id /*Transaction_ID*/, sample, msg_id, sizeof(Msg), 0 /*WAITSET_TYPE*/, retcode);
          if (retcode != FACE::RC_NO_ERROR) {
            ACE_ERROR((LM_ERROR, "ERROR: Listener::on_data_available - callback %d returned retcode: %d\n", i, retcode));
          }
        }
      }
    }
  }

  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;
  LockType callbacks_lock_;
  OPENDDS_VECTOR(Callback) callbacks_;
  const FACE::CONNECTION_ID_TYPE connection_id_;
};

template <typename Msg>
void register_callback(FACE::CONNECTION_ID_TYPE connection_id,
                       const FACE::WAITSET_TYPE /*waitset*/,
                       void (*callback)(FACE::TRANSACTION_ID_TYPE, Msg&,
                                        FACE::MESSAGE_TYPE_GUID,
                                        FACE::MESSAGE_SIZE_TYPE,
                                        const FACE::WAITSET_TYPE,
                                        FACE::RETURN_CODE_TYPE&),
                       FACE::MESSAGE_SIZE_TYPE max_message_size,
                       FACE::RETURN_CODE_TYPE& return_code)
{
  Entities::ConnIdToReceiverMap& readers = Entities::instance()->receivers_;
  if (!readers.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  if(!Entities::instance()->connections_.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  FACE::TRANSPORT_CONNECTION_STATUS_TYPE status =
    Entities::instance()->connections_[connection_id].connection_status;
  if (max_message_size < status.MAX_MESSAGE_SIZE) {
    return_code = FACE::INVALID_PARAM;
    return;
  }
  DDS::DataReaderListener_ptr existing_listener = readers[connection_id]->dr->get_listener();
  if (existing_listener) {
    Listener<Msg>* typedListener = dynamic_cast<Listener<Msg>*>(existing_listener);
    typedListener->add_callback(callback);
  } else {
    DDS::DataReaderListener_var listener = new Listener<Msg>(callback, connection_id);
    readers[connection_id]->dr->set_listener(listener, DDS::DATA_AVAILABLE_STATUS);
  }
  if (readers[connection_id]->status_valid != FACE::VALID) {
    Entities::FaceReceiver* tmp = readers[connection_id];
    readers[connection_id] = new Entities::DDSTypedAdapter<Msg>(*readers[connection_id]);
    delete tmp;
  }
  readers[connection_id]->status_valid = FACE::VALID;

  return_code = FACE::RC_NO_ERROR;
}

} }

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
