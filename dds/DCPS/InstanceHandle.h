/*
 * $Id$
 */

#ifndef DCPS_INSTANCEHANDLE_H
#define DCPS_INSTANCEHANDLE_H

#include "ace/Atomic_Op_T.h"
#include "ace/Thread_Mutex.h"

#include "tao/Basic_Types.h"

#include "dds/DdsDcpsInfrastructureC.h"

#include "dcps_export.h"

namespace OpenDDS
{
namespace DCPS
{
const unsigned HANDLE_KINDSZ(8);

const DDS::InstanceHandle_t HANDLE_UNKNOWN(0);

class OpenDDS_Dcps_Export InstanceHandleHelper
{
public:
  explicit InstanceHandleHelper(DDS::InstanceHandle_t handle);

  ~InstanceHandleHelper();

  bool is_topic();

  bool is_subscriber();

  bool is_publisher();

  bool is_datareader();

  bool is_datawriter();

  template <typename T_ptr>
  bool matches(T_ptr t)
  {
    return t->get_instance_handle() == handle_;
  }

private:
  DDS::InstanceHandle_t handle_;
};

class OpenDDS_Dcps_Export InstanceHandleGenerator
{
public:
  explicit InstanceHandleGenerator(CORBA::Octet kind, long begin = 0);

  ~InstanceHandleGenerator();

  CORBA::Octet kind();

  DDS::InstanceHandle_t next();

private:
  CORBA::Octet kind_;
  ACE_Atomic_Op<ACE_Thread_Mutex, long> sequence_;
};

} // namespace
} // namespace

#endif /* DCPS_INSTANCEHANDLE_H */
