/*
 * $Id$
 */

#ifndef DCPS_GUIDBUILDER_H
#define DCPS_GUIDBUILDER_H

#include "dds/DdsDcpsGuidC.h"

#include "GuidUtils.h"

#include "dcps_export.h"

namespace OpenDDS
{
namespace DCPS
{
class GuidBuilder
{
public:
  explicit GuidBuilder(GUID_t& guid);

  ~GuidBuilder();
  
  void guidPrefix0(long p0);
  void guidPrefix1(long p1);
  void guidPrefix2(long p2);
  
  void entityId(EntityId_t entityId);
  void entityId(long entityId);
 
  void entityKey(long entityKey);
 
  void entityKind(CORBA::Octet entityKind);
  void entityKind(EntityKind kind); 

private:
  GUID_t& guid_;
};

} // namespace
} // namespace

#endif /* DCPS_REPOIDBUILDER_H */
