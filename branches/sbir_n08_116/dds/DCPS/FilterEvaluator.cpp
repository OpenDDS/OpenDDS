/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "FilterEvaluator.h"
#include "FilterExpressionGrammar.h"

#include "yard/yard_parser.hpp"

#include <stdexcept>
#include <cstring>

namespace {
  std::string toString(yard::TreeBuildingParser<char>::Node* iter)
  {
    return std::string(iter->GetFirstToken(), iter->GetLastToken());
  }
}

namespace OpenDDS {
namespace DCPS {

FilterEvaluator::FilterEvaluator(const char* filter, bool allowOrderBy)
{
  using namespace FilterExpressionGrammar;

  const char* out = filter + std::strlen(filter);
  yard::SimpleTextParser parser(filter, out);
  if (allowOrderBy) {
    if (!parser.Parse<QueryCompleteInput>()) {
      throw std::runtime_error("Invalid expression");
    }
  } else {
    if (!parser.Parse<FilterCompleteInput>()) {
      throw std::runtime_error("Invalid expression");
    }
  }

  typedef yard::TreeBuildingParser<char>::Node Node;
  Node* root = parser.GetAstRoot();
  //TODO: walk the tree, populate data structures including order_bys_
  bool found_order_by(false);
  for (Node* iter = root->GetFirstChild(); iter; iter = iter->GetSibling()) {
    if (iter->TypeMatches<ORDERBY>()) {
      found_order_by = true;
    } else if (found_order_by && iter->TypeMatches<FieldName>()) {
      order_bys_.push_back(toString(iter));
    }
  }
}

std::vector<std::string>
FilterEvaluator::getOrderBys() const
{
  return order_bys_;
}

bool
FilterEvaluator::hasFilter() const
{
  return true; //TODO: impl
}

}
}

#endif
