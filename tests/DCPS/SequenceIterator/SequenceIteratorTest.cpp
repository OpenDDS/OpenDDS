#include "dds/DdsDcpsCoreC.h"
#include "dds/DCPS/SequenceIterator.h"
#include <vector>
#include <algorithm>

using namespace OpenDDS;

int Iterator_Concept ()
{
  DDS::OctetSeq s(1);
  s.length(1);
  s[0] = 1;

  typedef DCPS::SequenceIterator<DDS::OctetSeq> iter_t;
  iter_t i1 = DCPS::sequence_begin(s);
  iter_t i2(i1);
  i1 = i2;
  using std::swap;
  swap(i1, i2);
  const CORBA::Octet o = *i1;
  ACE_UNUSED_ARG(o);
  ++i1;

  return 0;
}

int StdCopy_ToVector_Success ()
{
  int retval = 0;
  DDS::OctetSeq expected;
  expected.length(4);
  expected[0] = 1;
  expected[1] = 2;
  expected[2] = 3;
  expected[3] = 4;

  std::vector<CORBA::Octet> result;

  std::copy(DCPS::sequence_begin(expected),
            DCPS::sequence_end(expected),
            std::back_inserter(result));

  if (expected.length() != result.size())
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: StdCopy_FromVector_Success: "
               "expected %d != result %d\n", expected.length(), result.size ()));
    ++retval;
  }

  for (CORBA::ULong i = 0; i < expected.length(); ++i) {
    if (expected[i] != result[i])
    {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: StdCopy_ToVector_Success: "
                "expected %d != result %d in member %d\n", expected[i], result[i], i));
      ++retval;
    }
  }

  return retval;
}

int StdCopy_FromVector_Success ()
{
  int retval = 0;
  std::vector<CORBA::Octet> expected;
  for (CORBA::Octet i = 0; i < 5; ++i) expected.push_back(i);

  DDS::OctetSeq result;

  std::copy(expected.begin(),
            expected.end(),
            DCPS::back_inserter(result));

  if (result.length() != expected.size())
  {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: StdCopy_FromVector_Success: "
               "result %d != expected %d\n", result.length(), expected.size ()));
    ++retval;
  }

  for (CORBA::ULong i = 0; i < expected.size(); ++i) {
    if (expected[i] != result[i])
    {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: StdCopy_FromVector_Success: "
                "expected %d != result %d in member %d\n", expected[i], result[i], i));
      ++retval;
    }
  }

  return retval;
}

int
ACE_TMAIN(int, ACE_TCHAR*[])
{
  int retval = 0;

  retval += Iterator_Concept ();
  retval += StdCopy_ToVector_Success ();
  retval += StdCopy_FromVector_Success ();

  return retval;
}
