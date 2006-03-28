// -*- C++ -*-
//
// $Id$

#include "DataReaderListener2.h"
#include "common.h"
#include "../common/SampleInfo.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/Service_Participant.h"
#include "tests/DCPS/MultiTopicTypes/Foo2TypeSupportC.h"
#include "tests/DCPS/MultiTopicTypes/Foo2TypeSupportImpl.h"

  void DataReaderListenerImpl2::read(::DDS::DataReader_ptr reader)
  {
    ACE_UNUSED_ARG(max_samples_per_instance);
    ACE_UNUSED_ARG(history_depth);
    ACE_UNUSED_ARG(using_udp);

    ::Mine::Foo2DataReader_var foo_dr = 
        ::Mine::Foo2DataReader::_narrow(reader
                                       ACE_ENV_ARG_PARAMETER);

    if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) ::Mine::FooDataReader::_narrow failed.\n")));
      }

    ::Mine::Foo2DataReaderImpl* dr_servant =
        ::TAO::DCPS::reference_to_servant< ::Mine::Foo2DataReaderImpl,
                                           ::Mine::Foo2DataReader_ptr>
            (foo_dr.in () ACE_ENV_SINGLE_ARG_PARAMETER);

    ::Mine::Foo2Seq foo(num_ops_per_thread) ;
    ::DDS::SampleInfoSeq si(num_ops_per_thread) ;
   
    DDS::ReturnCode_t status  ;
    status = dr_servant->read(foo, si,
                              num_ops_per_thread,
                              ::DDS::NOT_READ_SAMPLE_STATE,
                              ::DDS::ANY_VIEW_STATE,
                              ::DDS::ANY_INSTANCE_STATE);

    if (status == ::DDS::RETCODE_OK)
      {
        for (CORBA::ULong i = 0 ; i < si.length() ; i++)
        {
          num_samples_++ ;

          ACE_OS::printf ("foo2[%d]: text = %s, key = %d\n",
                          i, foo[i].text.in(), foo[i].key);
          PrintSampleInfo(si[i]) ;
        }
      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_OS::printf ("read returned ::DDS::RETCODE_NO_DATA\n") ;
      }
      else
      {
        ACE_OS::printf ("read - Error: %d\n", status) ;
      }
  }

