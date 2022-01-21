#include "MetaStructTestTypeSupportImpl.h"

#include <dds/DCPS/FilterEvaluator.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/Util.h>

#include <tao/SystemException.h>

#include <ace/OS_main.h>
#include <ace/OS_NS_string.h>

#include <iostream>
#include <string>
#include <cstring>
#include <cmath>

using namespace OpenDDS::DCPS;

bool checkVal(const char* lhs, const char* rhs, const char* name)
{
  if (std::strcmp(lhs, rhs)) {
    std::cout << "ERROR: target's " << name << " \"" << lhs << "\" != \""
              << rhs << "\"" << std::endl;
    return false;
  }
  return true;
}

bool checkVal(const TAO::String_Manager& lhs, const TAO::String_Manager& rhs,
             const char* name)
{
  return checkVal(lhs.in(), rhs.in(), name);
}

bool checkVal(const float& lhs, const float& rhs, const char* name)
{
  if (std::fabs((lhs - rhs) / lhs) > .001) {
    std::cout << "ERROR: target's " << name << " " << lhs << " != "
              << rhs << std::endl;
    return false;
  }
  return true;
}

template<size_t N, size_t M>
bool check_array(short (&lhs)[N][M], short (&rhs)[N][M], const char* name)
{
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      if (lhs[i][j] != rhs[i][j]) {
        std::cout << "ERROR target's " << name << "[" << i << "][" << j
                  << "] (" << lhs[i][j] << ") != " << rhs[i][j] << std::endl;
        return false;
      }
    }
  }
  return true;
}

template<typename T>
bool checkVal(const T& lhs, const T& rhs, const char* name)
{
  if (lhs != rhs) {
    std::cout << "ERROR: target's " << name << " " << lhs << " != "
              << rhs << std::endl;
    return false;
  }
  return true;
}

bool checkVal(const ACE_CDR::WChar& lhs, const ACE_CDR::WChar& rhs, const char* name)
{
  if (lhs != rhs) {
    std::cout << "ERROR: target's " << name << " " << int(lhs) << " != "
              << int(rhs) << std::endl;
    return false;
  }
  return true;
}

template<typename T, typename T2>
bool check(const T& lhs, const T& rhs, const char* name, ACE_Message_Block* amb, Value::Type type,
          const MetaStruct& ms, T2 Value::*ptrmbr, Encoding::Kind e)
{
  if (!checkVal(lhs, rhs, name)) return false;

  Message_Block_Ptr mb (amb->duplicate());
  const Encoding encoding(e);
  Serializer ser(mb.get(), encoding);
  Value val = ms.getValue(ser, name);
  if (val.type_ == type) {
    std::string ser_name = std::string("Serialized ") + name;
    if (!checkVal(T2(rhs), val.*ptrmbr, ser_name.c_str())) return false;
  } else {
    std::cout << "ERROR: Serialized type of " << name
              << " does not match. expected " << type
              << " received " << val.type_ << std::endl;
    return false;
  }
  return true;
}

template<size_t N, size_t M>
void fill_2d(short (&arr)[N][M])
{
  for (unsigned short i = 0; i < N; ++i) {
    for (unsigned short j = 0; j < M; ++j) {
      arr[i][j] = j + i * M;
    }
  }
}


template<typename Type>
bool run_test_i(Encoding::Kind e)
{
  Type src;
  src.a.s = "hello";
  src.a.l = 42;
  src.a.w = L'\x263a';
  src.a.c = 'j';
  src.sa[0] = 23;
  src.sa[1] = -16536;
  src.sa[2] = 16535;
  fill_2d(src.asa);
  src.ss.length(2);
  src.ss[0].s = "seq elt 0";
  src.ss[0].l = 9;
  src.ss[1].s = "seq elt 1";
  src.ss[1].l = -27;
  src.e = other1;
  src.u.u_f(2.17f);
  src.mu.u_f(2.17f);
  src.anon_seq.length(2);
  src.anon_seq[0].s = "seq elt 0";
  src.anon_seq[0].l = 9;
  src.anon_seq[1].s = "seq elt 1";
  src.anon_seq[1].l = -27;
  src.stra[0] = "hello";
  src.stra[1] = "world!";
  src.stra[2] = "goodbye";
  src.astra[0] = "hello";
  src.astra[1] = "world!";
  src.astra[2] = "goodbye";
  src.s = "hello";

  const MetaStruct& meta = getMetaStruct<Type>();

  Type tgt;
  for (const char** fields = meta.getFieldNames(); *fields; ++fields) {
    meta.assign(&tgt, *fields, &src, *fields, meta);
  }
  const Encoding encoding(e);
  Message_Block_Ptr data(new ACE_Message_Block(serialized_size(encoding, src)));
  Serializer ser(data.get(), encoding);
  if (!(ser << src)) {
    std::cout << "ERROR: Failed to serialize source" << std::endl;
    return false;
  }
  // Use checkVal for types that aren't supported in MetaStruct::getValue(), such as arrays and unions
  return
    check(tgt.a.s, src.a.s, "a.s", data.get(), Value::VAL_STRING, meta, &Value::s_, e)
    && check(tgt.a.l, src.a.l, "a.l", data.get(), Value::VAL_INT, meta, &Value::i_, e)
    && check(tgt.a.w, src.a.w, "a.w", data.get(), Value::VAL_INT, meta, &Value::i_, e)
    && check(tgt.a.c, src.a.c, "a.c", data.get(), Value::VAL_CHAR, meta, &Value::c_, e)
    && checkVal(tgt.sa[0], src.sa[0], "sa[0]")
    && checkVal(tgt.sa[1], src.sa[1], "sa[1]")
    && checkVal(tgt.sa[2], src.sa[2], "sa[2]")
    && check_array(tgt.asa, src.asa, "asa")
    && checkVal(tgt.ss.length(), src.ss.length(), "ss.length()")
    && checkVal(tgt.ss[0].s, src.ss[0].s, "ss[0].s")
    && checkVal(tgt.ss[0].l, src.ss[0].l, "ss[0].l")
    && checkVal(tgt.ss[1].s, src.ss[1].s, "ss[1].s")
    && checkVal(tgt.ss[1].l, src.ss[1].l, "ss[1].l")
    && check(tgt.e, src.e, "e", data.get(), Value::VAL_UINT, meta, &Value::u_, e)
    && checkVal(tgt.u._d(), src.u._d(), "u._d()")
    && checkVal(tgt.u.u_f(), src.u.u_f(), "u.u_f()")
    && checkVal(tgt.mu._d(), src.mu._d(), "mu._d()")
    && checkVal(tgt.mu.u_f(), src.mu.u_f(), "mu.u_f()")
    && checkVal(tgt.anon_seq.length(), src.anon_seq.length(), "anon_seq.length()")
    && checkVal(tgt.anon_seq[0].s, src.anon_seq[0].s, "anon_seq[0].s")
    && checkVal(tgt.anon_seq[0].l, src.anon_seq[0].l, "anon_seq[0].l")
    && checkVal(tgt.anon_seq[1].s, src.anon_seq[1].s, "anon_seq[1].s")
    && checkVal(tgt.anon_seq[1].l, src.anon_seq[1].l, "anon_seq[1].l")
    && checkVal(tgt.stra[0], src.stra[0], "stra[0]")
    && checkVal(tgt.stra[1], src.stra[1], "stra[1]")
    && checkVal(tgt.stra[2], src.stra[2], "stra[2]")
    && checkVal(tgt.astra[0], src.astra[0], "astra[0]")
    && checkVal(tgt.astra[1], src.astra[1], "astra[1]")
    && checkVal(tgt.astra[2], src.astra[2], "astra[2]")
    && check(tgt.s, src.s, "s", data.get(), Value::VAL_STRING, meta, &Value::s_, e);
}

Encoding::Kind encodings[] = {
  Encoding::KIND_UNALIGNED_CDR,
  //Encoding::KIND_XCDR1, // XTYPE-155
  Encoding::KIND_XCDR2,
};

template<typename Type>
bool run_test()
{
  bool success = true;
  for (size_t i = 0; i < array_count(encodings); i++) {
    const Encoding::Kind e = encodings[i];
    std::cout << "run_test_i<" << DDSTraits<Type>::type_name()
      << ">(" << Encoding::kind_to_string(e) << ")" << std::endl;
    try {
      success &= run_test_i<Type>(e);
    } catch (const CORBA::SystemException& e) {
      e._tao_print_exception("ERROR: ");
      success = false;
    } catch (const std::exception& e) {
      std::cout << "ERROR: exception caught: " << e.what() << std::endl;
      success = false;
    } catch (...) {
      std::cout << "ERROR: unknown exception in main" << std::endl;
      success = false;
    }
  }
  return success;
}

int ACE_TMAIN(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  bool success = true;
  success &= run_test<FinalStruct>();
  success &= run_test<AppenableStruct>();
  success &= run_test<MutableStruct>();
  return success ? 0 : 1;
}
