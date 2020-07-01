#ifndef _TYPE_LOOKUP_GET_DEPENDENCIES_IN_IMPL_H_
#define _TYPE_LOOKUP_GET_DEPENDENCIES_IN_IMPL_H_
#include "TypeLookup_getTypeDependencies_InC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DdsDcpsC.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/TypeObject.h"


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

void serialized_size(const Encoding& encoding, size_t& size, const XTypes::TypeIdentifierSeq& seq);

bool operator<<(Serializer& strm, const XTypes::TypeIdentifierSeq& seq);

bool operator>>(Serializer& strm, XTypes::TypeIdentifierSeq& seq);

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

bool gen_skip_over(Serializer& ser, XTypes::TypeIdentifierSeq*);

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

struct XTypes_TypeIdentifierSeq_xtag {};
template<> const XTypes::TypeObject& getMinimalTypeObject<XTypes_TypeIdentifierSeq_xtag>();

template<> XTypes::TypeIdentifier getMinimalTypeIdentifier<XTypes_TypeIdentifierSeq_xtag>();

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

void serialized_size(const Encoding& encoding, size_t& size, const XTypes::continuation_point_Seq& seq);

bool operator<<(Serializer& strm, const XTypes::continuation_point_Seq& seq);

bool operator>>(Serializer& strm, XTypes::continuation_point_Seq& seq);

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

bool gen_skip_over(Serializer& ser, XTypes::continuation_point_Seq*);

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

struct XTypes_continuation_point_Seq_xtag {};
template<> const XTypes::TypeObject& getMinimalTypeObject<XTypes_continuation_point_Seq_xtag>();

template<> XTypes::TypeIdentifier getMinimalTypeIdentifier<XTypes_continuation_point_Seq_xtag>();

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

void serialized_size(const Encoding& encoding, size_t& size, const XTypes::TypeLookup_getTypeDependencies_In& stru);

bool operator<<(Serializer& strm, const XTypes::TypeLookup_getTypeDependencies_In& stru);

bool operator>>(Serializer& strm, XTypes::TypeLookup_getTypeDependencies_In& stru);

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

class MetaStruct;

template<typename T>
const MetaStruct& getMetaStruct();

template<>
const MetaStruct& getMetaStruct<XTypes::TypeLookup_getTypeDependencies_In>();
bool gen_skip_over(Serializer& ser, XTypes::TypeLookup_getTypeDependencies_In*);

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace DCPS {

struct XTypes_TypeLookup_getTypeDependencies_In_xtag {};
template<> const XTypes::TypeObject& getMinimalTypeObject<XTypes_TypeLookup_getTypeDependencies_In_xtag>();

template<> XTypes::TypeIdentifier getMinimalTypeIdentifier<XTypes_TypeLookup_getTypeDependencies_In_xtag>();

}  }
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* _TYPE_LOOKUP_GET_DEPENDENCIES_IN_IMPL_H_ */
