#include "FACE/Sequence.h"
#include "FACE/SequenceVar.h"

#include <tao/Array_VarOut_T.h>
#include <tao/Seq_Var_T.h>
#include <tao/Seq_Out_T.h>

#include "ace/OS_main.h"
#include "ace/Log_Msg.h"

#include "test_check.h"

#include <algorithm>
#include <iostream>
#include <typeinfo>

namespace {
  unsigned int assertions = 0;
  unsigned int failed = 0;
}

using OpenDDS::FaceTypes::Sequence;
using OpenDDS::FaceTypes::SequenceVar;
using OpenDDS::FaceTypes::Unbounded;
using OpenDDS::FaceTypes::Bounded;
using OpenDDS::FaceTypes::StringEltPolicy;
using OpenDDS::FaceTypes::ArrayEltPolicy;
using OpenDDS::FaceTypes::VariEltPolicy;
using OpenDDS::FaceTypes::String_mgr;

template <typename Seq, typename Val>
void assignTo0(Seq& seq, const Val& v) { seq[0] = v; }

template <typename Seq, typename Slice, unsigned N>
void assignTo0(Seq& seq, const Slice (&a)[N])
{
  for (unsigned i = 0u; i < N; ++i) {
    seq[0][i] = a[i];
  }
}

template <typename Elt, typename Val>
bool equal(const Elt& e, const Val& v) { return e == v; }

template <typename Slice, unsigned N>
bool equal(const Slice (&lhs)[N], const Slice (&rhs)[N])
{
  for (unsigned i = 0; i < N; ++i) {
    if (lhs[i] != rhs[i]) return false;
  }
  return true;
}

template <typename Seq>
void testSeq(Seq& seq, const typename Seq::value_type& t)
{
  std::cerr << "Testing Seq: " << typeid(Seq).name() << '\n';
  TEST_CHECK(seq.length() == 0);
  Seq cpy(seq);
  TEST_CHECK(cpy.length() == 0);
  TEST_CHECK(cpy.release());
  seq = cpy;
  seq.length(1);
  TEST_CHECK(seq.length() == 1);
  assignTo0(seq, t);
  const Seq& cseq = seq;
  TEST_CHECK(equal(cseq[0], t));
  TEST_CHECK(equal(cseq.get_buffer()[0], t));
  TEST_CHECK(cseq.length() == 1);
  TEST_CHECK(cseq.size() == 1);
  TEST_CHECK(!cseq.empty());
  TEST_CHECK(cseq.max_size());
  TEST_CHECK(cseq.release());
  TEST_CHECK(cseq.maximum());
  TEST_CHECK(cseq.begin() + 1 == cseq.end());
  TEST_CHECK(seq.begin() + 1 == seq.end());
  TEST_CHECK(cseq == seq);
  TEST_CHECK(!(cseq != seq));

  typename Seq::value_type* const buf = seq.get_buffer(true /*orphan*/);
  TEST_CHECK(equal(buf[0], t));
  Seq::freebuf(buf);
}

struct MyStru { int i1_, i2_; }; // IDL "fixed-length" struct
bool operator==(const MyStru& lhs, const MyStru& rhs)
{
  return lhs.i1_ == rhs.i1_ && lhs.i2_ == rhs.i2_;
}

struct MyStru2 { int i1_; String_mgr s1_; }; // IDL "variable-length" struct
bool operator==(const MyStru2& lhs, const MyStru2& rhs)
{
  return lhs.i1_ == rhs.i1_ && lhs.s1_ == rhs.s1_;
}

struct S1 : Sequence<int, Unbounded> {};
struct S2 : Sequence<int, Bounded<5> > {};
struct S3 : Sequence<MyStru, Bounded<3> > {};
struct S4 : Sequence<MyStru2, Unbounded, VariEltPolicy<MyStru2> > {};
struct S5 : Sequence<S4, Unbounded, VariEltPolicy<S4> > {};

typedef SequenceVar<S1> S1_var;
typedef TAO_Seq_Out_T<S1> S1_out;

typedef SequenceVar<S4> S4_var;
typedef TAO_Seq_Out_T<S4> S4_out;

struct SS : Sequence<char*, Unbounded, StringEltPolicy<char> > {};
struct SB : Sequence<char*, Bounded<2>, StringEltPolicy<char> > {
  friend void swap(SB& lhs, SB& rhs) {
    lhs.swap(rhs);
  }
};

// Code generated for IDL "typedef unsigned short Arry[4];"
typedef unsigned short Arry[4];
typedef unsigned short Arry_slice;
struct Arry_tag {};
typedef ::TAO_FixedArray_Var_T<Arry, Arry_slice, Arry_tag> Arry_var;
typedef Arry Arry_out;
typedef ::TAO_Array_Forany_T<Arry, Arry_slice, Arry_tag> Arry_forany;

TAO_BEGIN_VERSIONED_NAMESPACE_DECL
namespace TAO {
  template <>
  struct Array_Traits<Arry_forany> {
    static void copy(Arry_slice* dst, const Arry_slice* src)
    {
      std::copy(src, src + 4, dst);
    }
    static void zero(Arry_slice*) {}
    static void construct(Arry_slice*) {}
    static void destroy(Arry_slice*) {}
  };
}
TAO_END_VERSIONED_NAMESPACE_DECL

struct SA1 : Sequence<Arry, Unbounded, ArrayEltPolicy<Arry_forany> > {};

// Code generated for IDL "typedef string Aos[6];"
typedef String_mgr Aos[6];
typedef String_mgr Aos_slice;
struct Aos_tag {};
typedef ::TAO_VarArray_Var_T<Aos, Aos_slice, Aos_tag> Aos_var;
typedef ::TAO_Array_Out_T<Aos, Aos_var, Aos_slice, Aos_tag> Aos_out;
typedef ::TAO_Array_Forany_T<Aos, Aos_slice, Aos_tag> Aos_forany;

TAO_BEGIN_VERSIONED_NAMESPACE_DECL
namespace TAO {
  template <>
  struct Array_Traits<Aos_forany> {
    static void copy(Aos_slice* dst, const Aos_slice* src)
    {
      std::copy(src, src + 6, dst);
    }
    static void zero(Aos_slice* slice)
    {
      std::fill_n(slice, 6, String_mgr());
    }
    static void construct(Aos_slice* slice)
    {
      std::uninitialized_fill_n(slice, 6, Aos_slice());
    }
    static void destroy(Aos_slice* slice)
    {
      for (int i = 0; i < 6; ++i) {
        slice[i].~Aos_slice();
      }
    }
  };
}
TAO_END_VERSIONED_NAMESPACE_DECL

struct SA2 : Sequence <Aos, Bounded<2>, ArrayEltPolicy<Aos_forany> > {};
struct SA3 : Sequence <Aos, Unbounded, ArrayEltPolicy<Aos_forany> > {};

// Code generated for IDL "typedef octet SmallArry[2];"
typedef unsigned char SmallArry[2];
typedef unsigned char SmallArry_slice;
struct SmallArry_tag {};
typedef ::TAO_FixedArray_Var_T<SmallArry, SmallArry_slice, SmallArry_tag>
  SmallArry_var;
typedef SmallArry SmallArry_out;
typedef ::TAO_Array_Forany_T<SmallArry, SmallArry_slice, SmallArry_tag>
  SmallArry_forany;

TAO_BEGIN_VERSIONED_NAMESPACE_DECL
namespace TAO {
  template <>
  struct Array_Traits<SmallArry_forany> {
    static void copy(SmallArry_slice* dst, const SmallArry_slice* src)
    {
      std::memcpy(dst, src, 2);
    }
    static void zero(SmallArry_slice*) {}
    static void construct(SmallArry_slice*) {}
    static void destroy(SmallArry_slice*) {}
  };
}
TAO_END_VERSIONED_NAMESPACE_DECL

struct SA4 : Sequence<SmallArry, Unbounded, ArrayEltPolicy<SmallArry_forany> >
{};

void basic_test()
{
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  S1 s1;
  S2 s2;
  TEST_CHECK_RETURN(s1.maximum() == 0, -1);
  TEST_CHECK_RETURN(s2.maximum() == 5, -1);
  int* const b1 = S1::allocbuf(3);
  int* const b2 = S2::allocbuf();
  S1::freebuf(b1);
  S2::freebuf(b2);

  testSeq(s1, 1);
  testSeq(s2, 1);

  S3 s3;
  testSeq(s3, MyStru());

  int i = 42;
  s2.replace(1, &i);
  TEST_CHECK_RETURN(s2[0] == i, -1);

  S4 s4;
  MyStru2 stru = {42, "42"};
  testSeq(s4, stru);

  S5 s5;
  testSeq(s5, s4);

  SS ss;
  testSeq(ss, FACE::string_dup(""));

  ss.length(2);
  TEST_CHECK_RETURN(ss[0][0] == '\0', -1);
  TEST_CHECK_RETURN(ss[1][0] == '\0', -1);
  ss[0] = "foo";
  ss[1] = "bar";
  ss[0] = "baz"; // frees "foo" before copying "baz"
  ss.length(1);

  const char* const storage[] = {"Hello", "world", "from", "sequence"};
  char* buffer[] = {const_cast<char*>(storage[0]),
                    const_cast<char*>(storage[1]),
                    const_cast<char*>(storage[2]),
                    const_cast<char*>(storage[3])};
  ss.replace(4, 2, buffer);
  TEST_CHECK_RETURN(ss.maximum() == 4, -1);
  TEST_CHECK_RETURN(ss.length() == 2, -1);
  TEST_CHECK_RETURN(!ss.release(), -1);
  TEST_CHECK_RETURN(0 == std::strcmp(ss[0], "Hello"), -1);
  TEST_CHECK_RETURN(0 == std::strcmp(ss[1], "world"), -1);
  ss.length(4);
  TEST_CHECK_RETURN(0 == std::strcmp(ss[2], "from"), -1);
  TEST_CHECK_RETURN(0 == std::strcmp(ss[3], "sequence"), -1);

  SB sb;
  testSeq(sb, FACE::string_dup(""));

  SB sb2;
  sb2.length(1);
  swap(sb, sb2);
  TEST_CHECK_RETURN(sb.length() == 1, -1);

  // Sequences of Arrays

  Arry arr1 = {1, 3, 1, 4};
  SA1 sa1;
  testSeq(sa1, arr1);

  SA2 sa2;
  Aos arr2 = {"6", "3", "1", "4", "1"}; // arr2[5] default ctor
  testSeq(sa2, arr2);

  SA3 sa3;
  testSeq(sa3, arr2);

  SA4 sa4;
  SmallArry arr4 = {217, 150};
  testSeq(sa4, arr4);

  S1_var s1v = new S1;
  s1v->length(2);
  s1v[0] = 1;
  s1v[1] = s1v[0];
  const S1_var& s1vc = s1v;
  TEST_CHECK_RETURN(s1vc[1] == 1, -1);

#if !defined __SUNPRO_CC && (!defined _MSC_VER || _MSC_VER >= 1500) && \
   (!defined __GNUC__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC__MINOR__ > 1))
  S4_var s4v = new S4;
  s4v->length(2);
  s4v[0].i1_ = 3;
  s4v[0].s1_ = "testing";
  s4v[1] = s4v[0];
  const S4_var& s4vc = s4v;
  TEST_CHECK_RETURN(s4vc[1].s1_ == "testing", -1);
#endif

  printf("%d assertions passed\n", assertions);
  return 0;
}
