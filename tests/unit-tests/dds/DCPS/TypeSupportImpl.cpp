/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SAFETY_PROFILE

#  include <XTypesUtilsTypeSupportImpl.h>

#  include <dds/DCPS/TypeSupportImpl.h>
#  include <dds/DCPS/XTypes/TypeLookupService.h>

#  include <ace/Log_Msg.h>
#  include <ace/Log_Msg_Callback.h>
#  include <ace/Log_Record.h>

#  include <gtest/gtest.h>

using namespace OpenDDS;

namespace {

// Counts LM_ERROR messages logged on this thread while in scope.
class ErrorLogCounter : public ACE_Log_Msg_Callback {
public:
  ErrorLogCounter()
    : count_(0)
    , prev_callback_(ACE_LOG_MSG->msg_callback(this))
    , prev_flags_(ACE_LOG_MSG->flags())
  {
    ACE_LOG_MSG->set_flags(ACE_Log_Msg::MSG_CALLBACK);
  }

  ~ErrorLogCounter()
  {
    ACE_LOG_MSG->clr_flags(ACE_Log_Msg::MSG_CALLBACK);
    ACE_LOG_MSG->set_flags(prev_flags_);
    ACE_LOG_MSG->msg_callback(prev_callback_);
  }

  void log(ACE_Log_Record& record)
  {
    if (record.type() == LM_ERROR) {
      ++count_;
    }
  }

  unsigned count() const
  {
    return count_;
  }

private:
  unsigned count_;
  ACE_Log_Msg_Callback* const prev_callback_;
  const unsigned long prev_flags_;
};

template <typename TypeSupportImplType, typename XTag>
void expect_complete_to_minimal_map()
{
  const XTypes::TypeLookupService_rch tls = DCPS::make_rch<XTypes::TypeLookupService>();
  TypeSupportImplType ts;
  {
    // Converting a type before its dependencies logs an error
    ErrorLogCounter errors;
    ts.add_types(tls);
    EXPECT_EQ(0u, errors.count());
  }

  XTypes::TypeIdentifier minimal_ti;
  EXPECT_TRUE(tls->get_minimal_type_identifier(DCPS::getCompleteTypeIdentifier<XTag>(),
                                               minimal_ti));
  EXPECT_TRUE(DCPS::getMinimalTypeIdentifier<XTag>() == minimal_ti);
}

} // namespace

TEST(dds_DCPS_TypeSupportImpl, AddTypesNestedStructs)
{
  expect_complete_to_minimal_map<XTypesUtils::FinalMaxMutableStructTypeSupportImpl,
                                 DCPS::XTypesUtils_FinalMaxMutableStruct_xtag>();
}

TEST(dds_DCPS_TypeSupportImpl, AddTypesUnionWithAliasMember)
{
  expect_complete_to_minimal_map<XTypesUtils::AppendableMaxMutableUnionTypeSupportImpl,
                                 DCPS::XTypesUtils_AppendableMaxMutableUnion_xtag>();
}

TEST(dds_DCPS_TypeSupportImpl, AddTypesManyMemberKinds)
{
  expect_complete_to_minimal_map<XTypesUtils::LessThanStructTypeSupportImpl,
                                 DCPS::XTypesUtils_LessThanStruct_xtag>();
}

#endif
