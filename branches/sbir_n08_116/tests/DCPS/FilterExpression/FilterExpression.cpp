#include "stdio.h" // yard references printf() without including this
#include "string.h" // yard references strncpy() without including this

#include "dds/DCPS/FilterExpressionGrammar.h"
#include "dds/DCPS/yard/yard_parser.hpp"
#include "dds/DCPS/FilterEvaluator.h"

#include "ace/OS_main.h"
#include "ace/OS_NS_string.h"

#include <string>
#include <iostream>

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

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  using namespace OpenDDS::DCPS;

  if (argc > 1 && ACE_OS::strncmp(argv[1], ACE_TEXT("-d"), 2) == 0) {
    debug = true;
  }

  //scratch
  FilterEvaluator fe("name LIKE 'Ad%'", false);
  DDS::TopicBuiltinTopicData sample;
  sample.name = "Eve ";
  std::cout << std::boolalpha << fe.eval(sample, DDS::StringSeq()) << std::endl;

  bool ok = true;
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

  const char* predicates[] = {"x BETWEEN 1 AND 3",
    "y.z NOT BETWEEN 'a' AND 'z'", "a = 3", "b > 5", "c >= 6", "d < 7",
    "e <= 8", "f <> 'z'", "g.a1.b2.c3.d4 LIKE 'foo%bar'", "'x' = x", "a = b"};
  ok &= parserTest<FilterExpressionGrammar::Pred>(predicates);

  const char* conditions[] = {"x=3", "y = 4  AND z <> 'foo'",
    "y = 4  OR z <> 'foo'", "NOT h >= 4", "(i<27)", "NOT (j  =1) AND k > 3",
    "a=1 OR a=2 OR a=3 OR a=4 OR a=5 OR a=6 OR a=7"};
  ok &= parserTest<FilterExpressionGrammar::Cond>(conditions);

  const char* queries[] = {"ORDER BY x", "ORDER BY x,y", "ORDER BY x.a.b.cde",
    "ORDER BY a.b.cd134.f_g5, e, h.i,   j.k.l", "x > 10 ORDER BY y"};
  ok &= parserTest<FilterExpressionGrammar::Query>(queries);

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
