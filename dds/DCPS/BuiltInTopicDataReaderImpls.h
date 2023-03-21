#ifndef OPENDDS_DCPS_BUILTINTOPICDATAREADERIMPLS_H
#define OPENDDS_DCPS_BUILTINTOPICDATAREADERIMPLS_H

#include "DataReaderImpl_T.h"

#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/OpenddsDcpsExtTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

typedef DataReaderImpl_T<DDS::ParticipantBuiltinTopicData> ParticipantBuiltinTopicDataDataReaderImpl;
typedef DataReaderImpl_T<DDS::PublicationBuiltinTopicData> PublicationBuiltinTopicDataDataReaderImpl;
typedef DataReaderImpl_T<DDS::SubscriptionBuiltinTopicData> SubscriptionBuiltinTopicDataDataReaderImpl;
typedef DataReaderImpl_T<DDS::TopicBuiltinTopicData> TopicBuiltinTopicDataDataReaderImpl;
typedef DataReaderImpl_T<ParticipantLocationBuiltinTopicData> ParticipantLocationBuiltinTopicDataDataReaderImpl;
typedef DataReaderImpl_T<InternalThreadBuiltinTopicData> InternalThreadBuiltinTopicDataDataReaderImpl;
typedef DataReaderImpl_T<ConnectionRecord> ConnectionRecordDataReaderImpl;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
#endif
