#include "FixedTypeSupportImpl.h"

#include "ace/OS_main.h"
#include "ace/CDR_Base.h"
#include <sstream>

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  int ret = 0;
#ifdef ACE_HAS_CDR_FIXED
  try {
    // Check opendds_idl-generated code.
    M1::FixedSeq seq;
    seq.length(2);
    seq[0] = "-1236124.47";
    seq[1] = "314612157238794.12";
    M1::S1 str = {1, M1::f1, {"123.45", 678.90, 123}, seq};

    size_t size = 0, padding = 0;
    OpenDDS::DCPS::gen_find_size(str, size, padding);
    ACE_Message_Block mb(size);
    OpenDDS::DCPS::Serializer ser(&mb);
    ser << str;
    OpenDDS::DCPS::Serializer ser2(&mb);
    M1::S1 str2;
    ser2 >> str2;
    if (!(str == str2)) {
      ++ret;
      ACE_ERROR((LM_ERROR, "Serializer round-trip failed\n"));
    }

    // Check that these compile, actual functionality is tested
    // in ACE's CDR_Fixed_Text.cpp.
    FACE::LongDouble ld;
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(ld, 7123490451.12);
    M1::Myfixed f, f2(-2), f3(3u), f4(FACE::LongLong(-4999876)),
      f5(FACE::UnsignedLongLong(55555555)), f6(-678.99), f7(ld),
      f8("-8888123456.78");

    FACE::LongLong ll = f5;
    ACE_UNUSED_ARG(ll);
    FACE::LongDouble ld2 = f8;
    ACE_UNUSED_ARG(ld2);
    M1::Myfixed f9 = f8.round(0), f10 = f8.truncate(0);
    FACE::String_var s = f7.to_string();

    f10 += f3;
    f10 -= f9;
    f10 *= f6;
    f10 /= f5;

    FACE::Fixed f11 = ++f4;
    FACE::Fixed f12 = f11++;
    FACE::Fixed f13 = --f12;
    FACE::Fixed f14 = f13--;

    FACE::Fixed f15 = +f14;
    FACE::Fixed f16 = -f15;

    bool b = !f;
    ACE_UNUSED_ARG(b);
    M1::Myfixed f17 = f16;
    f16 = f2;

    M1::Myfixed f18(s);

    f18.fixed_digits();
    f18.fixed_scale();

# ifndef ACE_LACKS_IOSTREAM_TOTALLY
    std::stringstream ss;
    ss << f18;
    ss >> f17;
# endif // ACE_LACKS_IOSTREAM_TOTALLY

    FACE::Fixed f19 = f18 + f17;
    FACE::Fixed f20 = f19 - f18;
    FACE::Fixed f21 = f20 * f2;
    FACE::Fixed f22 = f21 / f20;

    bool b2 = f19 < f20, b3 = f20 > f21, b4 = f21 <= f22, b5 = f22 >= f19,
      b6 = f19 == f20, b7 = f21 != f22;
    ACE_UNUSED_ARG(b2);
    ACE_UNUSED_ARG(b3);
    ACE_UNUSED_ARG(b4);
    ACE_UNUSED_ARG(b5);
    ACE_UNUSED_ARG(b6);
    ACE_UNUSED_ARG(b7);

  } catch (const CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, "Caught exception: %C\n", e._info().c_str()));
    ++ret;
  }

#endif // ACE_HAS_CDR_FIXED
  return ret;
}
