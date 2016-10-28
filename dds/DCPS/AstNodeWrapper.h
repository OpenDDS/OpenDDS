/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef OPENDDS_DCPS_ASTNODEWRAPPER_H
#define OPENDDS_DCPS_ASTNODEWRAPPER_H

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/PoolAllocator.h"

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "yard/yard_parser.hpp"
#include <sstream>
#include <stdexcept>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

typedef yard::TreeBuildingParser<char>::Node AstNode;

/// called after parsing has failed, throws std::exception with details
inline void reportErrors(yard::SimpleTextParser& parser, const char* input)
{
  AstNode* prev = 0;
  for (AstNode* iter = parser.GetAstRoot()->GetFirstChild(); iter;
      iter = iter->GetSibling()) {
    prev = iter;
  }
  ptrdiff_t pos = prev ? prev->GetLastToken() - parser.Begin() : 0;
  std::ostringstream oss;
  oss << pos;
  throw std::runtime_error("Invalid expression [" + OPENDDS_STRING(input)
    + "] at character " + oss.str());
}


inline OPENDDS_STRING toString(yard::TreeBuildingParser<char>::Node* iter)
{
  return iter ? OPENDDS_STRING(iter->GetFirstToken(), iter->GetLastToken()) : "";
}


/// keeps the details of yard out of the FilterEvaluator header file
struct FilterEvaluator::AstNodeWrapper {
  AstNodeWrapper(AstNode* ptr)
    : ptr_(ptr) {}
  operator AstNode*() const { return ptr_; }
  AstNode* operator->() const { return ptr_; }
  AstNode* ptr_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
#endif
