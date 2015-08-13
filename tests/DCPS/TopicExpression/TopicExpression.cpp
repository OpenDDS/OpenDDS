#include "stdio.h" // yard references printf() without including this
#include "string.h" // yard references strncpy() without including this

#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/TopicExpressionGrammar.h"
#include "dds/DCPS/yard/yard_parser.hpp"

#include "ace/OS_main.h"
#include "ace/OS_NS_string.h"

#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>


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

  const char* topicnametests[] = {"t0p1c_n4m3", "topicName", "t"};
  ok &= parserTest<TopicExpressionGrammar::TopicName>(topicnametests);
  ok &= parserTest<TopicExpressionGrammar::JoinItem>(topicnametests);
  ok &= parserTest<TopicExpressionGrammar::Selection>(topicnametests);

  const char* selectiontests[] = {"t1 INNER NATURAL JOIN t2",
    "t3 NATURAL JOIN t4", "t5 NATURAL INNER JOIN t6 INNER NATURAL JOIN t7",
    "t8 NATURAL JOIN (t9 NATURAL INNER JOIN t0)",
    "a NATURAL JOIN (b NATURAL JOIN (c NATURAL JOIN d NATURAL JOIN e))"};
  ok &= parserTest<TopicExpressionGrammar::Selection>(selectiontests);

  const char* aggtests[] = {"*", "f1", "f2.f3", "f3 f4", "f5 AS f6", "f7,  f8",
    "f9, f10, f11, f12", "f13.f14, f15.f16 f17.f18, f19.f20 AS f21.f22, aa, b"};
  ok &= parserTest<TopicExpressionGrammar::Aggregation>(aggtests);

  const char* selfromtests[] = {"SELECT * FROM t1", "SELECT  a, b FROM t2",
    "SELECT a.b.c, d.e.f, g.h FROM t3 NATURAL JOIN t4"};
  ok &= parserTest<TopicExpressionGrammar::SelectFrom>(selfromtests);

  const char* topictests[] = {"SELECT * FROM t WHERE a < 100;",
    "SELECT a FROM b;", "SELECT c x FROM d", "SELECT * FROM e NATURAL JOIN f;",
    "SELECT y h, w AS i FROM g INNER NATURAL JOIN h WHERE z = 3",
    "SELECT flight_name, x, y, z AS height FROM Location NATURAL JOIN "
    "FlightPlan WHERE height < 1000 AND x<23"}; // from spec (fixed)
  ok &= parserTest<TopicExpressionGrammar::TopicCompleteInput>(topictests);

  return ok;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  if (argc > 1 && ACE_OS::strncmp(argv[1], ACE_TEXT("-d"), 2) == 0) {
    debug = true;
  }

  return testParsing() ? EXIT_SUCCESS : EXIT_FAILURE;
}
