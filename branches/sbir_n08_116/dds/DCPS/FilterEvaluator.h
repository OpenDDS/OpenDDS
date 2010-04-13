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

class MetaStruct;

template<typename T>
const MetaStruct& getMetaStruct();

class OpenDDS_Dcps_Export FilterEvaluator {
public:
  FilterEvaluator(const char* filter, bool allowOrderBy);

  ~FilterEvaluator();

  std::vector<std::string> getOrderBys() const;

  bool hasFilter() const;

  template<typename T>
  bool eval(const T& sample, const DDS::StringSeq& params) const
  {
    return eval_i(static_cast<const void*>(&sample),
                  getMetaStruct<T>(), params);
  }

  class EvalNode;

private:
  FilterEvaluator(const FilterEvaluator&);
  FilterEvaluator& operator=(const FilterEvaluator&);

  struct AstNodeWrapper;
  EvalNode* walkAst(const AstNodeWrapper& node, EvalNode* prev);

  bool eval_i(const void* sample, const MetaStruct& meta,
              const DDS::StringSeq& params) const;

  std::string filter_;
  EvalNode* filter_root_;
  std::vector<std::string> order_bys_;
};

struct OpenDDS_Dcps_Export Value {
  Value(bool b, bool conversion_preferred = false);
  Value(int i, bool conversion_preferred = false);
  Value(unsigned int u, bool conversion_preferred = false);
  Value(char c, bool conversion_preferred = false);
  Value(double f, bool conversion_preferred = false);
  Value(const char* s, bool conversion_preferred = false);

  ~Value();
  Value(const Value& v);
  Value& operator=(const Value& v);
  void swap(Value& other);

  bool operator==(const Value& v) const;
  bool operator<(const Value& v) const;
  bool like(const Value& v) const;

  enum Type {VAL_BOOL, VAL_INT, VAL_UINT, VAL_FLOAT, VAL_CHAR, VAL_STRING};
  enum {VAL_LARGEST_NUMERIC = VAL_FLOAT};
  bool convert(Type t);
  static void conversion(Value& lhs, Value& rhs);
  template<typename T> T& get();
  template<typename T> const T& get() const;

  Type type_;
  union {
    bool b_;
    int i_;
    unsigned int u_;
    char c_;
    double f_;
    const char* s_;
  };
  bool conversion_preferred_;
};

class MetaStruct {
public:
  virtual Value getValue(const void* stru, const char* fieldSpec) const = 0;
};

/// Each user-defined struct type will have an instantiation of this template
/// generated (eventually) by an IDL processing code generation tool.
template<typename T>
struct MetaStructImpl;

/// Until code generation is ready, this will be the fall-back implementation
/// so that all OpenDDS test code will continue to build.
template<typename T>
struct NoOpMetaStructImpl : MetaStruct {
  Value getValue(const void*, const char*) const
  {
    return 0;
  }
};

template<typename T>
inline const MetaStruct& getMetaStruct()
{
  static NoOpMetaStructImpl<T> nomsi;
  return nomsi;
}


// Our .cpp file will contain the implementation for the topic built-in topic
// struct, so declare it here.  This is for testing.

template<>
OpenDDS_Dcps_Export
const MetaStruct& getMetaStruct<DDS::TopicBuiltinTopicData>();

}
}

#endif
#endif
