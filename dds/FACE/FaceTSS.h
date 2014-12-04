#ifndef OPENDDS_FACE_TSS_H
#define OPENDDS_FACE_TSS_H

#include "face/tss.hpp"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dds/DCPS/WaitSet.h"

#include "ace/Singleton.h"

#include <map>

namespace OpenDDS {
namespace FaceTSS {

class Entities {
  friend class ACE_Singleton<Entities, ACE_Thread_Mutex>;
  Entities();
  ~Entities();
public:
  OpenDDS_FACE_Export static Entities* instance();
  std::map<int, DDS::DataWriter_var> writers_;
  std::map<int, DDS::DataReader_var> readers_;
};

OpenDDS_FACE_Export DDS::Duration_t convertTimeout(FACE::TIMEOUT_TYPE timeout);

template <typename Msg>
void receive_message(FACE::CONNECTION_ID_TYPE connection_id,
                     FACE::TIMEOUT_TYPE timeout,
                     FACE::TRANSACTION_ID_TYPE& transaction_id,
                     Msg& message,
                     FACE::MESSAGE_SIZE_TYPE message_size,
                     FACE::RETURN_CODE_TYPE& return_code) {
  std::map<int, DDS::DataReader_var>& readers = Entities::instance()->readers_;
  if (!readers.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }

  typedef typename DCPS::DDSTraits<Msg>::DataReader DataReader;
  const typename DataReader::_var_type typedReader =
    DataReader::_narrow(readers[connection_id]);
  if (!typedReader) {
    return_code = FACE::INVALID_PARAM;
    return;
  }

  const DDS::ReadCondition_var rc =
    typedReader->create_readcondition(DDS::NOT_READ_SAMPLE_STATE,
                                      DDS::ANY_VIEW_STATE,
                                      DDS::ANY_INSTANCE_STATE);
  const DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(rc);

  DDS::ConditionSeq active;
  const DDS::Duration_t ddsTimeout = convertTimeout(timeout);
  DDS::ReturnCode_t ret = ws->wait(active, ddsTimeout);
  ws->detach_condition(rc);

  if (ret == DDS::RETCODE_TIMEOUT) {
    return_code = FACE::TIMED_OUT;
    return;
  }

  typename DCPS::DDSTraits<Msg>::Sequence seq;
  DDS::SampleInfoSeq sinfo;
  ret = typedReader->take_w_condition(seq, sinfo, 1 /*max*/, rc);

  if (ret == DDS::RETCODE_OK && sinfo[0].valid_data) {
    message = seq[0];
    return_code = FACE::NO_ERROR;
    return;
  }
  return_code = FACE::NOT_AVAILABLE;
}

template <typename Msg>
void send_message(FACE::CONNECTION_ID_TYPE connection_id,
                  const Msg& message,
                  FACE::MESSAGE_SIZE_TYPE message_size,
                  FACE::TIMEOUT_TYPE timeout,
                  FACE::TRANSACTION_ID_TYPE& transaction_id,
                  FACE::RETURN_CODE_TYPE& return_code) {
  std::map<int, DDS::DataWriter_var>& writers = Entities::instance()->writers_;
  if (!writers.count(connection_id)) {
    return_code = FACE::INVALID_PARAM;
    return;
  }

  typedef typename DCPS::DDSTraits<Msg>::DataWriter DataWriter;
  const typename DataWriter::_var_type typedWriter =
    DataWriter::_narrow(writers[connection_id]);
  if (!typedWriter) {
    return_code = FACE::INVALID_PARAM;
    return;
  }

  const DDS::ReturnCode_t ret = typedWriter->write(message, DDS::HANDLE_NIL);
  return_code =
    (ret == DDS::RETCODE_OK) ? FACE::NO_ERROR : FACE::CONNECTION_CLOSED;
  //TODO: convert errors
}

template <typename Callback>
void register_read_callback(FACE::CONNECTION_ID_TYPE connection_id,
                            const FACE::WAITSET_TYPE waitset,
                            Callback callback,
                            FACE::MESSAGE_SIZE_TYPE max_message_size,
                            FACE::RETURN_CODE_TYPE& return_code) {
  //TODO: implement
}

} }

#endif
