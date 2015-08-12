#include "stdio.h" // yard references printf() without including this
#include "string.h" // yard references strncpy() without including this

#include "FilterStructTypeSupportImpl.h"

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/FilterExpressionGrammar.h"
#include "dds/DCPS/yard/yard_parser.hpp"
#include "dds/DCPS/FilterEvaluator.h"

#include "ace/OS_main.h"
#include "ace/OS_NS_string.h"

#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>

template<size_t N, typename T>
bool doEvalTest(const char* (&input)[N], bool expected, const T& sample,
                const DDS::StringSeq& params) {
  bool pass = true;
  for (size_t i = 0; i < N; ++i) {
    try {
      OpenDDS::DCPS::FilterEvaluator fe(input[i], false);
      const bool result = fe.eval(sample, params);
      if (result != expected) pass = false;
      std::cout << input[i] << " => " << result << std::endl;
    } catch (const std::exception& e) {
      if (expected) pass = false;
      std::cout << input[i] << " => exception " << e.what() << std::endl;
    }
  }
  return pass;
}

bool testEval() {

  try {

    TBTD sample;
    sample.name = "Adam";
    sample.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
    sample.durability_service.history_depth = 15;
    sample.durability_service.service_cleanup_delay.sec = 0;
    sample.durability_service.service_cleanup_delay.nanosec = 10;

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
    bool ok = doEvalTest(filters_pass, true, sample, params);
    ok &= doEvalTest(filters_fail, false, sample, params);
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
