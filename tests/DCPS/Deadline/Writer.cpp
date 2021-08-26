// -*- C++ -*-
//

#include "Writer.h"

#include "Domain.h"

#include <tests/Utils/ExceptionStreams.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Qos_Helper.h>

#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>

#include <stdexcept>

Writer::Writer(const DDS::DataWriter_var& dw)
  : mdw_()
  , key_mutex_()
  , key_n_(0)
{
  if (!dw) {
    throw std::runtime_error("Writer::Writer DataWriter is null.\n");
  }
  mdw_ = Messenger::MessageDataWriter::_narrow(dw.in());
  if (!mdw_) {
    throw std::runtime_error("Writer::Writer DataWriter could not be narrowed.\n");
  }
}

bool Writer::start()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Writer::start: activate 2 threads\n")));
  if (activate(THR_NEW_LWP | THR_JOINABLE, 2) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Writer::start activate failed.\n")));
    return false;
  }
  return true;
}

void Writer::end()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Writer::end\n")));
  wait();
}

int Writer::svc()
{
  const int key = get_key();
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Writer::svc key%d begins.\n"), key));
  try {
    Messenger::Message msg;
    msg.subject_id = key;
    DDS::InstanceHandle_t instance = mdw_->register_instance(msg);
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Writer::svc key%d instance%d\n"), key, instance));
    msg.from = "Deadline";
    msg.subject = "Deadline Test";
    msg.text = "To test deadline miss.";

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Writer::svc key%d sleep %d seconds\n"), key, Domain::w_sleep.value().sec()));
    ACE_OS::sleep(Domain::w_sleep.value());
    for (msg.count = 1; msg.count <= Domain::n_msg; ++msg.count) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) write msg%d instance%d\n"), msg.count, instance));
      const DDS::ReturnCode_t r = mdw_->write(msg, instance);
      if (r != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: write msg%d instance%d returned %d\n"), msg.count, instance, r));
        return 1;
      }
      // Sleep for half a second between writes to allow some deadline periods to expire.
      // Missed deadline should not occur since the time between writes < the offered deadline period.
      ACE_OS::sleep(Domain::w_interval.value());
    }
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception(ACE_TEXT("(%P|%t) ERROR: Writer::svc:"));
    return 1;
  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Writer::svc ...\n")));
    return 1;
  }
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Writer::svc key%d finished.\n"), key));
  return 0;
}

int Writer::get_key()
{
  Lock lock(key_mutex_);
  if (!lock.locked()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Writer::get_key: failed to lock\n")));
    return 0;
  }
  return ++key_n_ == 1 ? Domain::s_key1 : Domain::s_key2;
}
