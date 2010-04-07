/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_FILTER_EVALUATOR_H
#define OPENDDS_DCPS_FILTER_EVALUATOR_H

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "dds/DdsDcpsInfrastructureC.h"

#include <vector>
#include <string>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export FilterEvaluator {
public:
  FilterEvaluator(const char* filter, bool allowOrderBy);

  std::vector<std::string> getOrderBys() const;

  bool hasFilter() const;

  template<typename T>
  bool eval(const T& sample, const DDS::StringSeq& params) const
  {
    return true; //TODO: impl
  }

private:
  std::vector<std::string> order_bys_;
};

}
}

#endif
#endif
