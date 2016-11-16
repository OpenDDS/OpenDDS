// -*- C++ -*-
//

#include "DataReaderListener4.h"
#include "common.h"
#include "tests/DCPS/common/SampleInfo.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/Service_Participant.h"
#include "Foo4DefTypeSupportC.h"
#include "Foo4DefTypeSupportImpl.h"

void DataReaderListenerImpl4::read(::DDS::DataReader_ptr reader)
{
  ::T4::Foo4DataReader_var foo_dr = ::T4::Foo4DataReader::_narrow(reader);

  if (CORBA::is_nil(foo_dr)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ::T4::Foo4DataReader::_narrow failed.\n")));
    return;
  }

  DDS::TopicDescription_var td = reader->get_topicdescription();
  CORBA::String_var topic = td->get_name();

  ::T4::Foo4Seq foo(num_ops_per_thread_);
  ::DDS::SampleInfoSeq si(num_ops_per_thread_);

  DDS::ReturnCode_t const status = foo_dr->read(foo, si,
                                              num_ops_per_thread_,
                                              ::DDS::NOT_READ_SAMPLE_STATE,
                                              ::DDS::ANY_VIEW_STATE,
                                              ::DDS::ANY_INSTANCE_STATE);

  if (status == ::DDS::RETCODE_OK) {

    for (CORBA::ULong i = 0; i < si.length(); ++i) {
      if (si[i].valid_data) {
        ++num_samples_;
        const CORBA::ULong len = foo[i].values.length();
        ACE_OS::printf("%s foo4[%u]: length = %u\n", topic.in(), i, len);

        for (CORBA::ULong j = 0; j < len; ++j) {
          ACE_OS::printf("\tfoo4[%u][%u]: value = %f\n",
                         i, j, foo[i].values[j]);
        }
      }
    }

  } else if (status == ::DDS::RETCODE_NO_DATA) {
    ACE_OS::printf("read returned ::DDS::RETCODE_NO_DATA\n") ;

  } else {
    ACE_OS::printf("read - Error: %d\n", status) ;
  }
}
