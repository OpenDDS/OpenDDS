#include "FACE/Sequence.h"
#include "ace/OS_main.h"
#include "test_check.h"

unsigned int assertions = 0;

using OpenDDS::FaceTypes::Sequence;
using OpenDDS::FaceTypes::Unbounded;
using OpenDDS::FaceTypes::Bounded;
using OpenDDS::FaceTypes::StringEltPolicy;

int ACE_TMAIN()
{

  struct S1 : Sequence<int, Unbounded> {} s1;
  struct S2 : Sequence<int, Bounded<5> > {} s2;
  s1.maximum();
  s2.maximum();
  s1.allocbuf(3);
  s2.allocbuf();

  struct SS : Sequence<char*, Unbounded, StringEltPolicy<char> > {} ss;
  ss.length(2);
  ss[0] = "foo";
  ss[1] = "bar";
  ss[0] = "baz"; // frees "foo" before copying "baz"

  return 0;
}
