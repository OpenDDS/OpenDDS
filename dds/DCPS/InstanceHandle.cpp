/*
 * $Id$
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "dds/DdsDcpsGuidC.h"

#include "InstanceHandle.h"

namespace OpenDDS
{
namespace DCPS
{
InstanceHandleHelper::InstanceHandleHelper(DDS::InstanceHandle_t handle)
  : handle_(handle)
{
}

InstanceHandleHelper::~InstanceHandleHelper()
{
}

bool
InstanceHandleHelper::is_topic()
{
  return handle_ & ENTITYKIND_OPENDDS_TOPIC;
}

bool
InstanceHandleHelper::is_subscriber()
{
  return handle_ & ENTITYKIND_OPENDDS_SUBSCRIBER;
}

bool
InstanceHandleHelper::is_publisher()
{
  return handle_ & ENTITYKIND_OPENDDS_PUBLISHER;
}

bool
InstanceHandleHelper::is_datareader()
{
  return handle_ & ENTITYKIND_USER_READER_WITH_KEY
      || handle_ & ENTITYKIND_USER_READER_NO_KEY;
}

bool
InstanceHandleHelper::is_datawriter()
{
  return handle_ & ENTITYKIND_USER_WRITER_WITH_KEY
      || handle_ & ENTITYKIND_USER_WRITER_NO_KEY;
}


InstanceHandleGenerator::InstanceHandleGenerator(CORBA::Octet kind, long begin)
  : kind_(kind),
    sequence_(begin)
{
}

InstanceHandleGenerator::~InstanceHandleGenerator()
{
}

CORBA::Octet
InstanceHandleGenerator::kind()
{
  return kind_;
}

DDS::InstanceHandle_t
InstanceHandleGenerator::next()
{
  DDS::InstanceHandle_t handle(kind_);
  return handle |= (++sequence_ << HANDLE_KINDSZ);
}

} // namespace
} // namespace
