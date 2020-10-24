#include <ace/OS_main.h>
#include <ace/OS_NS_string.h>
#include <tao/SystemException.h>
#include <dds/DCPS/FilterEvaluator.h>
#include <dds/DCPS/Serializer.h>

#include "MetaStructTestTypeSupportImpl.h"

#include <iostream>
#include <string>
#include <cstring>
#include <cmath>

using namespace OpenDDS::DCPS;

int checkVal(const TAO::String_Manager& lhs, const TAO::String_Manager& rhs,
             const char* name)
{
  if (std::strcmp(lhs, rhs)) {
    std::cout << "ERROR: target's " << name << " " << lhs << " != "
              << rhs << std::endl;
    return 1;
  }
  return 0;
}

int checkVal(const float& lhs, const float& rhs, const char* name)
{
  if (std::fabs((lhs - rhs) / lhs) > .001) {
    std::cout << "ERROR: target's " << name << " " << lhs << " != "
              << rhs << std::endl;
    return 1;
  }
  return 0;
}

template<size_t N, size_t M>
int check_array(short (&lhs)[N][M], short (&rhs)[N][M], const char* name)
{
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < M; ++j) {
      if (lhs[i][j] != rhs[i][j]) {
        std::cout << "ERROR target's " << name << "[" << i << "][" << j
                  << "] (" << lhs[i][j] << ") != " << rhs[i][j] << std::endl;
        return 1;
      }
    }
  }
  return 0;
}

template<typename T>
int checkVal(const T& lhs, const T& rhs, const char* name)
{
  if (lhs != rhs) {
    std::cout << "ERROR: target's " << name << " " << lhs << " != "
              << rhs << std::endl;
    return 1;
  }
  return 0;
}

template<typename T, typename T2>
int check(const T& lhs, const T& rhs, const char* name, ACE_Message_Block* amb, Value::Type type,
          const MetaStruct& ms, T2 Value::*ptrmbr)
{
  if (checkVal(lhs, rhs, name)) return 1;

  Message_Block_Ptr mb (amb->duplicate());
  const Encoding encoding(Encoding::KIND_UNALIGNED_CDR);
  Serializer ser(mb.get(), encoding);
  std::string rhs_name = name;
  rhs_name[0] = 'r';
  Value val = ms.getValue(ser, rhs_name.c_str());
  if (val.type_ == type) {
    std::string ser_name = std::string("Serialized ") + name;
    if (checkVal(T2(rhs), val.*ptrmbr, ser_name.c_str())) return 1;
  } else {
    std::cout << "ERROR: Serialized type of " << name
              << " does not match. expected " << type
              << " received " << val.type_ << std::endl;
    return 1;
  }
  return 0;
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


int run_test(int, ACE_TCHAR*[])
{
  Source src;
  src.rhs_a.s = "hello";
  src.rhs_a.l = 42;
  src.rhs_a.w = L'\x263a';
  src.rhs_a.c = 'j';
  src.rhs_sa[0] = 23;
  src.rhs_sa[1] = -16536;
  src.rhs_sa[2] = 16535;
  fill_2d(src.rhs_asa);
  src.rhs_ss.length(2);
  src.rhs_ss[0].s = "seq elt 0";
  src.rhs_ss[0].l = 9;
  src.rhs_ss[1].s = "seq elt 1";
  src.rhs_ss[1].l = -27;
  src.rhs_e = other1;
  src.rhs_u.u_f(2.17f);

  const MetaStruct& sourceMeta = getMetaStruct<Source>();
  const MetaStruct& targetMeta = getMetaStruct<Target>();

  Target tgt;
  for (const char** fields = sourceMeta.getFieldNames(); *fields; ++fields) {
    std::string tgtField = *fields;
    tgtField[0] = 'l';
    targetMeta.assign(&tgt, tgtField.c_str(), &src, *fields, sourceMeta);
  }
  const Encoding encoding(Encoding::KIND_UNALIGNED_CDR);
  Message_Block_Ptr data(new ACE_Message_Block(serialized_size(encoding, src)));
  Serializer ser(data.get(), encoding);
  if (!(ser<<src)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Tests::MetaStructTest::run_test: ")
      ACE_TEXT("Failed to serialize source.\n")));
    return 1;
  }
  // Use checkVal for types that aren't supported in MetaStruct::getValue(), such as arrays and unions
  return check(tgt.lhs_a.s, src.rhs_a.s, "lhs_a.s", data.get(), Value::VAL_STRING, sourceMeta, &Value::s_)
    + check(tgt.lhs_a.l, src.rhs_a.l, "lhs_a.l", data.get(), Value::VAL_INT, sourceMeta, &Value::i_)
    + check(tgt.lhs_a.w, src.rhs_a.w, "lhs_a.w", data.get(), Value::VAL_INT, sourceMeta, &Value::i_)
    + check(tgt.lhs_a.c, src.rhs_a.c, "lhs_a.c", data.get(), Value::VAL_CHAR, sourceMeta, &Value::c_)
    + checkVal(tgt.lhs_sa[0], src.rhs_sa[0], "lhs_sa[0]")
    + checkVal(tgt.lhs_sa[1], src.rhs_sa[1], "lhs_sa[1]")
    + checkVal(tgt.lhs_sa[2], src.rhs_sa[2], "lhs_sa[2]")
    + check_array(tgt.lhs_asa, src.rhs_asa, "lhs_asa")
    + checkVal(tgt.lhs_ss.length(), src.rhs_ss.length(), "lhs_ss.length()")
    + checkVal(tgt.lhs_ss[0].s, src.rhs_ss[0].s, "lhs_ss[0].s")
    + checkVal(tgt.lhs_ss[0].l, src.rhs_ss[0].l, "lhs_ss[0].l")
    + checkVal(tgt.lhs_ss[1].s, src.rhs_ss[1].s, "lhs_ss[1].s")
    + checkVal(tgt.lhs_ss[1].l, src.rhs_ss[1].l, "lhs_ss[1].l")
    + check(tgt.lhs_e, src.rhs_e, "lhs_e", data.get(), Value::VAL_UINT, sourceMeta, &Value::u_)
    + checkVal(tgt.lhs_u._d(), src.rhs_u._d(), "lhs_u._d()")
    + checkVal(tgt.lhs_u.u_f(), src.rhs_u.u_f(), "lhs_u.u_f()");
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int ret = 1;
  try {
    ret = run_test(argc, argv);
  } catch (const CORBA::SystemException& e) {
    e._tao_print_exception("ERROR: ");
  } catch (const std::exception& e) {
    std::cout << "ERROR: exception caught: " << e.what() << std::endl;
  } catch (...) {
    std::cout << "ERROR: unknown exception in main" << std::endl;
  }
  return ret;
}
