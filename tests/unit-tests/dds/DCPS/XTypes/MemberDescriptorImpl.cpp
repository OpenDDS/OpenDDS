#ifndef OPENDDS_SAFETY_PROFILE

#include <dds/DCPS/XTypes/MemberDescriptorImpl.h>

#include <dds/DCPS/XTypes/TypeDescriptorImpl.h>
#include <dds/DCPS/XTypes/DynamicTypeImpl.h>

#include <gtest/gtest.h>

using namespace OpenDDS::XTypes;

TEST(dds_DCPS_XTypes_MemberDescriptorImpl, MemberDescriptorImpl_ctor)
{
  MemberDescriptorImpl md;

  EXPECT_STREQ(md.name(), "");
  EXPECT_EQ(md.id(), MEMBER_ID_INVALID);
  EXPECT_EQ(md.type(), DDS::DynamicType::_nil());
  EXPECT_STREQ(md.default_value(), "");
  EXPECT_EQ(md.index(), 0U);
  EXPECT_EQ(md.label().length(), 0U);
  EXPECT_EQ(md.try_construct_kind(), DDS::DISCARD);
  EXPECT_FALSE(md.is_key());
  EXPECT_FALSE(md.is_optional());
  EXPECT_FALSE(md.is_must_understand());
  EXPECT_FALSE(md.is_shared());
  EXPECT_FALSE(md.is_default_label());
}

TEST(dds_DCPS_XTypes_MemberDescriptorImpl, MemberDescriptorImpl_equals)
{
  MemberDescriptorImpl md1;

  EXPECT_FALSE(md1.equals(0));
  EXPECT_TRUE(md1.equals(&md1));

  MemberDescriptorImpl md2;
  EXPECT_TRUE(md1.equals(&md2));
  EXPECT_TRUE(md2.equals(&md1));

  TypeDescriptorImpl* tdi = new TypeDescriptorImpl();
  tdi->kind(TK_BOOLEAN);
  tdi->name("Boolean");
  DDS::TypeDescriptor_var td = tdi;
  DynamicTypeImpl* dti = new DynamicTypeImpl();
  DDS::DynamicType_var dt = dti;
  dti->set_descriptor(td);

  {
    MemberDescriptorImpl md3;
    md3.name("name");
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.id(1);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.type(dt);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.default_value("true");
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.index(2);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    DDS::UnionCaseLabelSeq label;
    label.length(1);
    label[0] = 3;
    md3.label(label);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.try_construct_kind(DDS::USE_DEFAULT);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.is_key(true);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.is_optional(true);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.is_must_understand(true);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.is_shared(true);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
  {
    MemberDescriptorImpl md3;
    md3.is_default_label(true);
    EXPECT_TRUE(md3.equals(&md3));
    EXPECT_FALSE(md3.equals(&md1));
    EXPECT_FALSE(md1.equals(&md3));
  }
}

TEST(dds_DCPS_XTypes_MemberDescriptorImpl, MemberDescriptorImpl_copy_from)
{
  TypeDescriptorImpl* tdi = new TypeDescriptorImpl();
  tdi->kind(TK_BOOLEAN);
  tdi->name("Boolean");
  DDS::TypeDescriptor_var td = tdi;
  DynamicTypeImpl* dti = new DynamicTypeImpl();
  DDS::DynamicType_var dt = dti;
  dti->set_descriptor(td);

  MemberDescriptorImpl md1;
  EXPECT_EQ(md1.copy_from(0), DDS::RETCODE_BAD_PARAMETER);

  MemberDescriptorImpl md2;
  md2.name("name");
  md2.id(1);
  md2.type(dt);
  md2.default_value("TRUE");
  md2.index(2);
  DDS::UnionCaseLabelSeq label;
  label.length(1);
  label[0] = 3;
  md2.label(label);
  md2.try_construct_kind(DDS::DISCARD);
  md2.is_key(true);
  md2.is_optional(true);
  md2.is_must_understand(true);
  md2.is_shared(true);
  md2.is_default_label(true);

  EXPECT_EQ(md1.copy_from(&md2), DDS::RETCODE_OK);
  EXPECT_TRUE(md1.equals(&md2));
  EXPECT_TRUE(md2.equals(&md1));
}

// TODO:  Test is_consistent.

#endif // OPENDDS_SAFETY_PROFILE
