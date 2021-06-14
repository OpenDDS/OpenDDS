// -*- C++ -*-
//

#include "DataReaderListenerImpl.h"

#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/ExceptionStreams.h"

#include <dds/DCPS/Service_Participant.h>

#include <ace/streams.h>

using namespace Messenger;
using namespace std;

DataReaderListenerImpl::DataReaderListenerImpl(DistributedConditionSet_rch dcs,
                                               const OPENDDS_STRING& actor)
  : dcs_(dcs)
  , actor_(actor)
  , expected_read_count_(0)
  , read_count_(0)
{}

void DataReaderListenerImpl::on_data_available (DDS::DataReader_ptr reader)
{
  ::Messenger::MessageDataReader_var message_dr = ::Messenger::MessageDataReader::_narrow(reader);

  Messenger::Message message;
  DDS::SampleInfo si;
  DDS::ReturnCode_t status = message_dr->take_next_sample(message, si);

  if (status == DDS::RETCODE_OK) {
    cout << "Subscriber = " << actor_ << endl;
    cout << "SampleInfo.sample_rank = " << si.sample_rank << endl;
    cout << "SampleInfo.instance_state = " << OpenDDS::DCPS::InstanceState::instance_state_string(si.instance_state) << endl;

    if (si.valid_data == 1) {
      ++read_count_;
      cout << "Message: subject    = " << message.subject.in() << endl
           << "         subject_id = " << message.subject_id   << endl
           << "         from       = " << message.from.in()    << endl
           << "         count      = " << message.count        << endl
           << "         text       = " << message.text.in()    << endl;
      if (read_count_ == expected_read_count_) {
        dcs_->post(actor_, condition_);
      }
    } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
      cout << "instance is disposed" << endl;
    } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
      cout << "instance is unregistered" << endl;
    } else {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) DataReaderListenerImpl::on_data_available: received unknown instance state %d\n",
                 si.instance_state));
    }
  } else if (status == DDS::RETCODE_NO_DATA) {
    cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << endl;
  } else {
    cerr << "ERROR: read Message: Error: " <<  status << endl;
  }
}
