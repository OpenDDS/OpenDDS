#include "stdio.h" // yard references printf() without including this
#include "string.h" // yard references strncpy() without including this

#include "FilterStructTypeSupportImpl.h"
#include "FilterStructDynamicTypeSupport.h"

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/EncapsulationHeader.h"
#include "dds/DCPS/FilterExpressionGrammar.h"
#include "dds/DCPS/yard/yard_parser.hpp"
#include "dds/DCPS/FilterEvaluator.h"

#include "dds/DCPS/XTypes/DynamicDataFactory.h"
#include "dds/DCPS/XTypes/DynamicSample.h"

#include "ace/OS_main.h"
#include "ace/OS_NS_string.h"

#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>

DDS::DynamicData_var copy(const TBTD& sample, DDS::DynamicType* type)
{
  using namespace DDS;
  DynamicData_var dd = DynamicDataFactory::get_instance()->create_data(type);
  dd->set_string_value(dd->get_member_id_by_name("name"), sample.name);

  DynamicData_var nested;
  dd->get_complex_value(nested, dd->get_member_id_by_name("durability"));
  nested->set_int32_value(nested->get_member_id_by_name("kind"), static_cast<ACE_CDR::Long>(sample.durability.kind));

  dd->get_complex_value(nested, dd->get_member_id_by_name("durability_service"));
  DynamicData_var duration;
  nested->get_complex_value(duration, nested->get_member_id_by_name("service_cleanup_delay"));
  duration->set_int32_value(duration->get_member_id_by_name("sec"), sample.durability_service.service_cleanup_delay.sec);
  duration->set_uint32_value(duration->get_member_id_by_name("nanosec"), sample.durability_service.service_cleanup_delay.nanosec);

  nested->set_int32_value(nested->get_member_id_by_name("history_kind"), static_cast<ACE_CDR::Long>(sample.durability_service.history_kind));
  nested->set_int32_value(nested->get_member_id_by_name("history_depth"), sample.durability_service.history_depth);
  nested->set_int32_value(nested->get_member_id_by_name("max_samples"), sample.durability_service.max_samples);
  nested->set_int32_value(nested->get_member_id_by_name("max_instances"), sample.durability_service.max_instances);
  nested->set_int32_value(nested->get_member_id_by_name("max_samples_per_instance"), sample.durability_service.max_samples_per_instance);
  return dd;
}

template <typename T>
ACE_Message_Block* serialize(const OpenDDS::DCPS::Encoding& enc, const T& sample)
{
  using namespace OpenDDS::DCPS;
  const EncapsulationHeader encapsulation(enc, APPENDABLE);
  size_t sz = EncapsulationHeader::serialized_size;
  serialized_size(enc, sz, sample);
  Message_Block_Ptr mb(new ACE_Message_Block(sz));
  Serializer ser(mb.get(), enc);
  if (!(ser << encapsulation) || !(ser << sample)) {
    return 0;
  }
  return mb.release();
}

using OpenDDS::DCPS::TypeSupportImpl;
template <size_t N, typename T>
bool doEvalTest(const char* (&input)[N], bool expected, const T& sample, const DDS::StringSeq& params,
                TypeSupportImpl& tsStatic, TypeSupportImpl& tsDynamic)
{
  ACE_UNUSED_ARG(tsStatic);
  ACE_UNUSED_ARG(tsDynamic);
  using namespace OpenDDS::DCPS;
  static const Encoding enc_xcdr2(Encoding::KIND_XCDR2);
  bool pass = true;
  for (size_t i = 0; i < N; ++i) {
    try {
      FilterEvaluator fe(input[i], false);
      const bool result = fe.eval(sample, params);
      if (result != expected) pass = false;
      std::cout << input[i] << " => " << result << std::endl;
    } catch (const std::exception& e) {
      if (expected) pass = false;
      std::cout << input[i] << " => exception " << e.what() << std::endl;
    }
    try {
      Message_Block_Ptr amb(serialize(enc_xcdr2, sample));
      FilterEvaluator fe(input[i], false);
      const bool result = fe.eval(amb.get(), enc_xcdr2, tsStatic, params);
      if (result != expected) pass = false;
      std::cout << input[i] << " =xcdr=> " << result << std::endl;
    } catch (const std::exception& e) {
      if (expected) pass = false;
      std::cout << input[i] << " =xcdr=> exception " << e.what() << std::endl;
    }
    try {
      DDS::DynamicType_var dyntype = tsDynamic.get_type();
      DDS::DynamicData_var dynamic = copy(sample, dyntype);
      OpenDDS::XTypes::DynamicSample dsample(dynamic);
      FilterEvaluator fe(input[i], false);
      const bool result = fe.eval(dsample, params);
      if (result != expected) pass = false;
      std::cout << input[i] << " =dynamic=> " << result << std::endl;
    } catch (const std::exception& e) {
      if (expected) pass = false;
      std::cout << input[i] << " =dynamic=> exception " << e.what() << std::endl;
    }
    try {
      Message_Block_Ptr amb(serialize(enc_xcdr2, sample));
      FilterEvaluator fe(input[i], false);
      const bool result = fe.eval(amb.get(), enc_xcdr2, tsDynamic, params);
      if (result != expected) pass = false;
      std::cout << input[i] << " =dynamic/xcdr=> " << result << std::endl;
    } catch (const std::exception& e) {
      if (expected) pass = false;
      std::cout << input[i] << " =dynamic/xcdr=> exception " << e.what() << std::endl;
    }
  }
  return pass;
}

bool testEval() {

  try {

    TBTD sample;
    sample.name = "Adam";
    sample.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
    sample.durability_service.history_kind = DDS::KEEP_LAST_HISTORY_QOS;
    sample.durability_service.history_depth = 15;
    sample.durability_service.service_cleanup_delay.sec = 0;
    sample.durability_service.service_cleanup_delay.nanosec = 10;
    sample.durability_service.max_samples = 0;
    sample.durability_service.max_instances = 0;
    sample.durability_service.max_samples_per_instance = 0;

    DDS::StringSeq params;
    params.length(1);
    params[0] = "3";

    static const char* filters_pass[] = {"name LIKE 'Ad%'",
                                         "durability.kind = 'PERSISTENT_DURABILITY_QOS'",
                                         "durability_service.history_depth > %0",
                                         "durability_service.service_cleanup_delay.sec = 0 AND durability_service.service_cleanup_delay.nanosec >= 10",
                                         "durability_service.service_cleanup_delay.sec < durability_service.service_cleanup_delay.nanosec",
                                         "MOD(durability_service.history_depth,3) = 0"
    };

    static const char* filters_fail[] = {"name LIKE 'ZZ%'",
                                         "durability.kind = 'TRANSIENT_DURABILITY_QOS'",
                                         "durability_service.history_depth < %0",
                                         "durability_service.service_cleanup_delay.sec = 0 AND durability_service.service_cleanup_delay.nanosec BETWEEN 3 AND 5",
                                         "durability_service.service_cleanup_delay.sec = durability_service.service_cleanup_delay.nanosec",
                                         "MOD(durability_service.history_depth,4) = 0"};

    std::cout << std::boolalpha;
    TBTDTypeSupportImpl tsStat;
    DummyTypeSupport tsDyn;
    bool ok = doEvalTest(filters_pass, true, sample, params, tsStat, tsDyn);
    ok &= doEvalTest(filters_fail, false, sample, params, tsStat, tsDyn);
    return ok;

  } catch (const CORBA::BAD_PARAM&) {
    return false;
  }

}

// parsing test helpers
namespace yard_test {

  template<typename Rule_T>
  bool Test(const char* in, void (*callback)(yard::SimpleTextParser&) = 0)
  {
    const char* out = in + std::strlen(in);
    yard::SimpleTextParser parser(in, out);
    const bool pass = parser.Parse<yard::Seq<Rule_T, yard::EndOfInput> >();
    std::printf("%s test for rule %s, on input %s\n",
      pass ? "passed" : "FAILED", typeid(Rule_T).name(), in);
    if (pass && callback) callback(parser);
    return pass;
  }
}

typedef yard::TreeBuildingParser<char>::Node Node;

void printNode(Node* n, int depth = 0) {
  std::cout << std::string(depth, '+') << n->GetRuleTypeInfo().name();
  if (n->IsCompleted()) {
    std::cout << ": [" << std::string(n->GetFirstToken(), n->GetLastToken())
      << ']';
  }
  std::cout << std::endl;
  for (Node* c = n->GetFirstChild(); c; c = c->GetSibling()) {
    printNode(c, depth + 1);
  }
}

void callback(yard::SimpleTextParser& p) {
  printNode(p.GetAstRoot());
}

bool debug = false;

template<typename T, size_t N>
bool parserTest(const char* (&input)[N]) {
  bool pass = true;
  for (size_t i = 0; i < N; ++i) {
    pass &= yard_test::Test<T>(input[i], debug ? callback : 0);
  }
  return pass;
}


bool testParsing() {
  using namespace OpenDDS::DCPS;
  bool ok = true;

  try {
    FilterEvaluator eval("a = 3 AND b = 4 OR Z ! 5", false);
    ok = false;
  } catch (const std::exception& e) {
    std::cout << e.what() << std::endl;
  }

  const char* inttests[] = {"3", "+4", "-5", "1348907505135", "0x135135",
    "0X43514651", "0xABCD", "0x132fabe"};
  ok &= parserTest<FilterExpressionGrammar::IntVal>(inttests);
  ok &= parserTest<FilterExpressionGrammar::Param>(inttests);

  const char* chartests[] = {"'x'", "`x'", "'''"};
  ok &= parserTest<FilterExpressionGrammar::CharVal>(chartests);
  ok &= parserTest<FilterExpressionGrammar::Param>(chartests);

  const char* floattests[] = {"0", "+0", "-0", "0.1", "+0.1", "-0.1",
    "0.134132515", "+0.134132515", "-0.134132515", "0.23e4", "+0.23e4",
    "-0.23e4", "0.23e+4", "+0.23e+4", "-0.23e+4", "0.23e-4", "+0.23e-4",
    "-0.23e-4", ".461895", "+.461895", "-.461895", ".461895e12", "+.461895e12",
    "-.461895e12", ".461895e+12", "+.461895e+12", "-.461895e+12",
    ".461895e-12", "+.461895e-12", "-.461895e-12"};
  ok &= parserTest<FilterExpressionGrammar::FloatVal>(floattests);
  ok &= parserTest<FilterExpressionGrammar::Param>(floattests);

  const char* strtests[] = {"''", "`'", "'xy'", "`xy'",
    "'asjfh;wighghg2890276t80t2gh'"};
  ok &= parserTest<FilterExpressionGrammar::StrVal>(strtests);
  ok &= parserTest<FilterExpressionGrammar::Param>(strtests);

  const char* paramtests[] = {"%0", "%1", "%85", "%99"};
  ok &= parserTest<FilterExpressionGrammar::ParamVal>(paramtests);
  ok &= parserTest<FilterExpressionGrammar::Param>(paramtests);

  const char* fieldnames[] = {"abc", "abc123", "a_bc13_35", "a.b.c.d",
    "a_.b_.c_.d_"};
  ok &= parserTest<FilterExpressionGrammar::FieldName>(fieldnames);

  const char* primaries[] = {"MOD(inst.num, 3)",
                             "MOD(inst.num, %0)"};
  ok &= parserTest<FilterExpressionGrammar::Primary>(primaries);

  const char* calls[] = {"MOD(inst.num, 3)",
                         "MOD(inst.num, %0)"};
  ok &= parserTest<FilterExpressionGrammar::Call>(calls);

  const char* comppreds[] = {"MOD(inst.num, 3) = 0",
                             "MOD(inst.num, %0) = %1"};
  ok &= parserTest<FilterExpressionGrammar::CompPred>(comppreds);

  const char* predicates[] = {"x BETWEEN 1 AND 3",
                              "y.z NOT BETWEEN 'a' AND 'z'",
                              "a = 3",
                              "b > 5",
                              "c >= 6",
                              "d < 7",
                              "e <= 8",
                              "f <> 'z'",
                              "g.a1.b2.c3.d4 LIKE 'foo%bar'",
                              "'x' = x",
                              "a = b",
                              "MOD(durability_service.history_depth,3) = 0",
                              "MOD(durability_service.history_depth,%0) = 0"};
  ok &= parserTest<FilterExpressionGrammar::Pred>(predicates);

  const char* conditions[] = {"x=3",
                              "y = 4  AND z <> 'foo'",
                              "y = 4  OR z <> 'foo'",
                              "NOT h >= 4",
                              "(i<27)",
                              "NOT (j  =1) AND k > 3",
                              "a=1 OR a=2 OR a=3 OR a=4 OR a=5 OR a=6 OR a=7",
                              "MOD(durability_service.history_depth,3) = 0",
                              "MOD(durability_service.history_depth,%0) = 0"};
  ok &= parserTest<FilterExpressionGrammar::Cond>(conditions);

  const char* filtercompletes[] = {"x=3",
                                   "y = 4  AND z <> 'foo'",
                                   "y = 4  OR z <> 'foo'",
                                   "NOT h >= 4",
                                   "(i<27)",
                                   "NOT (j  =1) AND k > 3",
                                   "a=1 OR a=2 OR a=3 OR a=4 OR a=5 OR a=6 OR a=7",
                                   "MOD(durability_service.history_depth,3) = 0",
                                   "MOD(durability_service.history_depth,%0) = 0"};
  ok &= parserTest<FilterExpressionGrammar::FilterCompleteInput>(filtercompletes);


  const char* queries[] = {"ORDER BY x",
                           "ORDER BY x,y",
                           "ORDER BY x.a.b.cde",
                           "ORDER BY a.b.cd134.f_g5, e, h.i,   j.k.l",
                           "x > 10 ORDER BY y",
                           "MOD(durability_service.history_depth,3) = 0",
                           "MOD(durability_service.history_depth,%0) = 0"};
  ok &= parserTest<FilterExpressionGrammar::Query>(queries);

  return ok;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  if (argc > 1 && ACE_OS::strncmp(argv[1], ACE_TEXT("-d"), 2) == 0) {
    debug = true;
  }

  bool ok = testParsing();
  ok &= testEval();

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
