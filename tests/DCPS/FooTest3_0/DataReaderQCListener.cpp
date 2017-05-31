/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Service_Participant.h>

#include "DataReaderQCListener.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType3/FooDefTypeSupportImpl.h"

#include <iostream>

DataReaderQCListenerImpl::DataReaderQCListenerImpl()
  : DataReaderListenerImpl()
{
}

DataReaderQCListenerImpl::~DataReaderQCListenerImpl()
{
}

void DataReaderQCListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  try {
    Xyz::FooDataReader_var foo_dr =
      Xyz::FooDataReader::_narrow(reader);

    if (CORBA::is_nil(foo_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    Xyz::FooSeq foo;
    DDS::SampleInfoSeq si;

    DDS::ReturnCode_t status = foo_dr->take_w_condition(foo, si, 1, qc_.in ()) ;

    if (status == DDS::RETCODE_OK) {
      for (CORBA::ULong index = 0; index < si.length(); index++)
        {
          ++samples_read_;
          std::cout << "SampleInfo.valid_data = " << si[index].valid_data << std::endl;
          std::cout << "SampleInfo.sample_rank = " << si[index].sample_rank << std::endl;
          std::cout << "SampleInfo.instance_state = " << si[index].instance_state << std::endl;

          if (si[index].valid_data) {
            std::cout << "Foo sample processed" << std::endl;
          } else if (si[index].instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));
            ++samples_disposed_;

          } else if (si[index].instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));

          } else {
            ACE_ERROR((LM_ERROR,
                      ACE_TEXT("%N:%l: on_data_available()")
                      ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                      si[index].instance_state));
          }
        }
    } else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: unexpected status: %d\n"),
                 status));
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    ACE_OS::exit(-1);
  }
}

void DataReaderQCListenerImpl::set_qc (
  DDS::QueryCondition_ptr qc)
{
  qc_ = DDS::QueryCondition::_duplicate (qc);
}
