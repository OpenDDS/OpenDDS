#include "FilterStructDynamicTypeSupport.h"

#include <dds/DCPS/XTypes/DynamicSample.h>
#include <dds/DCPS/XTypes/DynamicTypeImpl.h>
#include <dds/DCPS/XTypes/DynamicTypeMemberImpl.h>

using namespace DDS;
using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

// Implement required parts of TypeSupportImpl

const MetaStruct& DummyTypeSupport::getMetaStructForType() const
{
  return getMetaStruct<DynamicSample>();
}

SerializedSizeBound DummyTypeSupport::serialized_size_bound(const Encoding&) const
{
  return SerializedSizeBound();
}

SerializedSizeBound DummyTypeSupport::key_only_serialized_size_bound(const Encoding&) const
{
  return SerializedSizeBound();
}

Extensibility DummyTypeSupport::base_extensibility() const
{
  return OpenDDS::DCPS::FINAL;
}

Extensibility DummyTypeSupport::max_extensibility() const
{
  return OpenDDS::DCPS::FINAL;
}

TypeIdentifier& DummyTypeSupport::getMinimalTypeIdentifier() const
{
  static TypeIdentifier ti;
  return ti;
}

const TypeMap& DummyTypeSupport::getMinimalTypeMap() const
{
  static TypeMap tm;
  return tm;
}

const TypeIdentifier& DummyTypeSupport::getCompleteTypeIdentifier() const
{
  static TypeIdentifier ti;
  return ti;
}

const TypeMap& DummyTypeSupport::getCompleteTypeMap() const
{
  static TypeMap tm;
  return tm;
}


// Construct the DynamicType for the TBTD struct in FilterStruct.idl
// Doesn't need to be spec-compliant as long as it's enough to support
// constructing DynamicData and using it with the FilterEvaluator

DynamicTypeMember_var memberForName()
{
  TypeDescriptor_var tdString = new TypeDescriptorImpl;
  tdString->kind(TK_STRING8);
  tdString->name("String8");

  DynamicTypeImpl* dtString = new DynamicTypeImpl;
  DynamicType_var dtvar = dtString;
  dtString->set_descriptor(tdString);

  MemberDescriptor_var memberNameDesc = new MemberDescriptorImpl;
  memberNameDesc->name("name");
  memberNameDesc->id(0);
  memberNameDesc->index(0);
  memberNameDesc->type(dtString);

  DynamicTypeMemberImpl* memberNameImpl = new DynamicTypeMemberImpl;
  memberNameImpl->set_descriptor(memberNameDesc);
  return memberNameImpl;
}

template <typename E>
void addEnumerator(DynamicTypeImpl* dt, E kind, const char* name)
{
  MemberDescriptorImpl* md = new MemberDescriptorImpl;
  MemberDescriptor_var md_var = md;
  md->name(name);
  md->id(static_cast<DDS::MemberId>(kind));
  md->type(dt);
  md->index(static_cast<unsigned>(kind));

  DynamicTypeMemberImpl* dtm = new DynamicTypeMemberImpl;
  DynamicTypeMember_var dtm_var = dtm;
  dtm->set_descriptor(md);
  dt->insert_dynamic_member(dtm);
}

DynamicTypeMember_var memberForKind()
{
  DynamicTypeImpl* dtKind = new DynamicTypeImpl;
  DynamicType_var dtvar = dtKind;
  addEnumerator(dtKind, VOLATILE_DURABILITY_QOS, "VOLATILE_DURABILITY_QOS");
  addEnumerator(dtKind, TRANSIENT_LOCAL_DURABILITY_QOS, "TRANSIENT_LOCAL_DURABILITY_QOS");
  addEnumerator(dtKind, TRANSIENT_DURABILITY_QOS, "TRANSIENT_DURABILITY_QOS");
  addEnumerator(dtKind, PERSISTENT_DURABILITY_QOS, "PERSISTENT_DURABILITY_QOS");

  TypeDescriptor_var tdKind = new TypeDescriptorImpl;
  tdKind->kind(TK_ENUM);
  tdKind->name("DDS::DurabilityQosPolicyKind");
  tdKind->bound().length(1);
  tdKind->bound()[0] = 32;
  dtKind->set_descriptor(tdKind);

  MemberDescriptor_var memberKindDesc = new MemberDescriptorImpl;
  memberKindDesc->name("kind");
  memberKindDesc->id(0);
  memberKindDesc->index(0);
  memberKindDesc->type(dtKind);

  DynamicTypeMemberImpl* memberKindImpl = new DynamicTypeMemberImpl;
  memberKindImpl->set_descriptor(memberKindDesc);
  return memberKindImpl;
}

DynamicTypeMember_var memberForDurability()
{
  TypeDescriptor_var tdDurabilityQosPolicy = new TypeDescriptorImpl;
  tdDurabilityQosPolicy->kind(TK_STRUCTURE);
  tdDurabilityQosPolicy->name("DDS::DurabilityQosPolicy");

  DynamicTypeImpl* dtDurabilityQosPolicy = new DynamicTypeImpl;
  DynamicType_var dtvar = dtDurabilityQosPolicy;
  dtDurabilityQosPolicy->set_descriptor(tdDurabilityQosPolicy);
  dtDurabilityQosPolicy->insert_dynamic_member(memberForKind());

  MemberDescriptor_var memberDurabilityDesc = new MemberDescriptorImpl;
  memberDurabilityDesc->name("durability");
  memberDurabilityDesc->id(1);
  memberDurabilityDesc->index(1);
  memberDurabilityDesc->type(dtDurabilityQosPolicy);

  DynamicTypeMemberImpl* memberDurabilityImpl = new DynamicTypeMemberImpl;
  memberDurabilityImpl->set_descriptor(memberDurabilityDesc);
  return memberDurabilityImpl;
}

DynamicTypeMember_var integerMember(DDS::TypeKind tk, const char* name, DDS::MemberId mid)
{
  TypeDescriptor_var tdInt = new TypeDescriptorImpl;
  tdInt->kind(tk);
  tdInt->name(tk == TK_UINT32 ? "UInt32" : "Int32"); // update if other types are used

  DynamicTypeImpl* dtInt = new DynamicTypeImpl;
  DynamicType_var dtvar = dtInt;
  dtInt->set_descriptor(tdInt);

  MemberDescriptor_var memberDesc = new MemberDescriptorImpl;
  memberDesc->name(name);
  memberDesc->id(mid);
  memberDesc->index(mid);
  memberDesc->type(dtInt);

  DynamicTypeMemberImpl* member = new DynamicTypeMemberImpl;
  member->set_descriptor(memberDesc);
  return member;
}

DynamicTypeMember_var memberForServiceCleanupDelay()
{
  TypeDescriptor_var tdDuration = new TypeDescriptorImpl;
  tdDuration->kind(TK_STRUCTURE);
  tdDuration->name("DDS::Duration_t");

  DynamicTypeImpl* dtDuration = new DynamicTypeImpl;
  DynamicType_var dtvar = dtDuration;
  dtDuration->set_descriptor(tdDuration);
  dtDuration->insert_dynamic_member(integerMember(TK_INT32, "sec", 0));
  dtDuration->insert_dynamic_member(integerMember(TK_UINT32, "nanosec", 1));

  MemberDescriptor_var memberDesc = new MemberDescriptorImpl;
  memberDesc->name("service_cleanup_delay");
  memberDesc->id(0);
  memberDesc->index(0);
  memberDesc->type(dtDuration);

  DynamicTypeMemberImpl* member = new DynamicTypeMemberImpl;
  member->set_descriptor(memberDesc);
  return member;
}

DynamicTypeMember_var memberForHistoryKind()
{
  DynamicTypeImpl* dtKind = new DynamicTypeImpl;
  DynamicType_var dtvar = dtKind;
  addEnumerator(dtKind, KEEP_LAST_HISTORY_QOS, "KEEP_LAST_HISTORY_QOS");
  addEnumerator(dtKind, KEEP_ALL_HISTORY_QOS, "KEEP_ALL_HISTORY_QOS");

  TypeDescriptor_var tdKind = new TypeDescriptorImpl;
  tdKind->kind(TK_ENUM);
  tdKind->name("DDS::HistoryQosPolicyKind");
  tdKind->bound().length(1);
  tdKind->bound()[0] = 32;
  dtKind->set_descriptor(tdKind);

  MemberDescriptor_var memberKindDesc = new MemberDescriptorImpl;
  memberKindDesc->name("history_kind");
  memberKindDesc->id(1);
  memberKindDesc->index(1);
  memberKindDesc->type(dtKind);

  DynamicTypeMemberImpl* memberKindImpl = new DynamicTypeMemberImpl;
  memberKindImpl->set_descriptor(memberKindDesc);
  return memberKindImpl;
}

DynamicTypeMember_var memberForDurabilityService()
{
  TypeDescriptor_var tdDurabilityServiceQosPolicy = new TypeDescriptorImpl;
  tdDurabilityServiceQosPolicy->kind(TK_STRUCTURE);
  tdDurabilityServiceQosPolicy->name("DDS::DurabilityServiceQosPolicy");

  DynamicTypeImpl* dtDurabilityServiceQosPolicy = new DynamicTypeImpl;
  DynamicType_var dtvar = dtDurabilityServiceQosPolicy;
  dtDurabilityServiceQosPolicy->set_descriptor(tdDurabilityServiceQosPolicy);
  dtDurabilityServiceQosPolicy->insert_dynamic_member(memberForServiceCleanupDelay());
  dtDurabilityServiceQosPolicy->insert_dynamic_member(memberForHistoryKind());
  dtDurabilityServiceQosPolicy->insert_dynamic_member(integerMember(TK_INT32, "history_depth", 2));
  dtDurabilityServiceQosPolicy->insert_dynamic_member(integerMember(TK_INT32, "max_samples", 3));
  dtDurabilityServiceQosPolicy->insert_dynamic_member(integerMember(TK_INT32, "max_instances", 4));
  dtDurabilityServiceQosPolicy->insert_dynamic_member(integerMember(TK_INT32, "max_samples_per_instance", 5));

  MemberDescriptor_var memberDurabilityServiceDesc = new MemberDescriptorImpl;
  memberDurabilityServiceDesc->name("durability_service");
  memberDurabilityServiceDesc->id(2);
  memberDurabilityServiceDesc->index(2);
  memberDurabilityServiceDesc->type(dtDurabilityServiceQosPolicy);

  DynamicTypeMemberImpl* memberDurabilityServiceImpl = new DynamicTypeMemberImpl;
  memberDurabilityServiceImpl->set_descriptor(memberDurabilityServiceDesc);
  return memberDurabilityServiceImpl;
}

DynamicType* DummyTypeSupport::get_type() const
{
  TypeDescriptor_var td = new TypeDescriptorImpl;
  td->kind(TK_STRUCTURE);
  td->name("TBTD");

  DynamicTypeImpl* dt = new DynamicTypeImpl;
  dt->set_descriptor(td);
  dt->insert_dynamic_member(memberForName());
  dt->insert_dynamic_member(memberForDurability());
  dt->insert_dynamic_member(memberForDurabilityService());
  return dt;
}
