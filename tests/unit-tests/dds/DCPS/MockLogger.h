#ifndef TEST_DDS_DCPS_MOCK_LOGGER_H
#define TEST_DDS_DCPS_MOCK_LOGGER_H

#include <gmock/gmock.h>

#include <ace/Log_Msg.h>
#include <ace/Log_Msg_Backend.h>
#include <ace/Log_Record.h>

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Test {

class MockLogger : public ACE_Log_Msg_Backend {
public:
  MockLogger()
  {
    // Install as backend.
    previous_ = ACE_Log_Msg::msg_backend(this);
    ACE_Log_Msg::instance()->set_flags(ACE_Log_Msg::CUSTOM);
    ACE_Log_Msg::instance()->clr_flags(ACE_Log_Msg::STDERR);
  }

  ~MockLogger()
  {
    ACE_Log_Msg::instance()->clr_flags(ACE_Log_Msg::CUSTOM);
    ACE_Log_Msg::instance()->set_flags(ACE_Log_Msg::STDERR);
    ACE_Log_Msg::msg_backend(previous_);
  }

  MOCK_METHOD1(open, int(const ACE_TCHAR*));
  MOCK_METHOD0(reset, int());
  MOCK_METHOD0(close, int());
  MOCK_METHOD1(log, ssize_t(ACE_Log_Record&));

private:
  ACE_Log_Msg_Backend* previous_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // TEST_DDS_DCPS_MOCK_LOGGER_H
