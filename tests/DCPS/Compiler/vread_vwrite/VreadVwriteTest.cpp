#include "gtest/gtest.h"

#include "VreadVwriteTestC.h"
#include "VreadVwriteTestTypeSupportImpl.h"

#include <dds/DCPS/JsonValueReader.h>
#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/XTypes/DynamicVwrite.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>

#include <fstream>

#if OPENDDS_HAS_JSON_VALUE_WRITER
TEST(VreadVwriteTest, ParseTest)
{
  std::ifstream ifs("VreadVwriteTest.json", std::ios_base::in | std::ios_base::binary);

  std::stringstream stream;
  stream << ifs.rdbuf();
  ASSERT_EQ(ifs.good(), true);

  Mod::SampleTypeSupport_var ts = new Mod::SampleTypeSupportImpl;
  OpenDDS::DCPS::RepresentationFormat_var format = ts->make_format(OpenDDS::DCPS::JSON_DATA_REPRESENTATION);
  Mod::Sample_var samplev;
  std::string as_string = stream.str();
  ASSERT_EQ(ts->decode_from_string(as_string.c_str(), samplev, format), DDS::RETCODE_OK);
  const Mod::Sample sample = *samplev;

  const unsigned int len = static_cast<unsigned int>(as_string.size());
  DDS::OctetSeq octets(len, len, reinterpret_cast<unsigned char*>(&as_string[0]));
  ASSERT_EQ(ts->decode_from_bytes(octets, samplev, format), DDS::RETCODE_OK);
  const Mod::Sample sample2 = *samplev;
  // sample2 should be exactly the same as sample, a few of the members are checked below

  ASSERT_EQ(sample.id, 5);
  ASSERT_EQ(sample2.id, 5);
  ASSERT_EQ(std::string(sample.data.in()), "The most rapid of JSONs");
  ASSERT_EQ(std::string(sample2.data.in()), "The most rapid of JSONs");
  ASSERT_EQ(sample.enu, Mod::three);
  ASSERT_EQ(sample2.enu, Mod::three);
  ASSERT_EQ(sample.enu2, Mod::two);
  ASSERT_EQ(sample2.enu2, Mod::two);
  ASSERT_EQ(sample.bt.o, 129);
#if OPENDDS_HAS_EXPLICIT_INTS
  ASSERT_EQ(sample.bt.u8, 130);
  ASSERT_EQ(sample.bt.i8, -50);
#endif
  ASSERT_EQ(sample.bt.us, 32777);
  ASSERT_EQ(sample.bt.s, -32765);
  ASSERT_EQ(sample.bt.ul, 2147483690);
  ASSERT_EQ(sample.bt.l, -2147483632);
  ASSERT_EQ(sample.bt.ull, 9223372036854775810ull);
  ASSERT_EQ(sample.bt.ll, -9223372036854775806ll);
  ASSERT_LT(sample.bt.f, 0.00141 + 1e-6);
  ASSERT_GT(sample.bt.f, 0.00141 - 1e-6);
  ASSERT_LT(sample.bt.d, -0.00000141 + 1e-9);
  ASSERT_GT(sample.bt.d, -0.00000141 - 1e-9);
  ASSERT_DOUBLE_EQ(double(sample.bt.ld), 1e34);
  ASSERT_EQ(sample.bt.b, false);
  ASSERT_EQ(sample.bt.c, '\r');
  ASSERT_EQ(std::string(sample.bt.str.in()), "The most JSON of rapids");
  std::wstring wstr_file(sample.bt.wstr.in());
  //This doesn't work on Windows because of how Visual Studio wants to treat the unicode it finds in a source file. We'll have to manually encode all unicode characters
  //const wchar_t* expected = L"Τηισ ισ α τεστ οφ τηε εμεργενψυ βροαδψαστ συστεμ. פךקשדק לקקפ טםור ישמגד ןמדןגק איק הקיןבךק שא שךך אןצקד";
  //                             Τ     η     ι     σ           ι     σ           α           τ     ε     σ     τ          ο      φ           τ     η     ε           ε     μ     ε     ρ     γ     ε     ν     ψ     υ           β     ρ     ο     α     δ     ψ     α     σ     τ           σ     υ     σ     τ     ε     μ     .           p     l     e     a     s     e           k     e     e     p           y     o     u     r           h     a     n     d     s           i     n     s     i     d     e           t     h     e           v     e     h     i     c     l     e           a     t           a     l     l           t     i     m     e     s
  const wchar_t expected[] = L"\u03a4\u03b7\u03b9\u03c3\u0020\u03b9\u03c3\u0020\u03b1\u0020\u03c4\u03b5\u03c3\u03c4\u0020\u03bf\u03c6\u0020\u03c4\u03b7\u03b5\u0020\u03b5\u03bc\u03b5\u03c1\u03b3\u03b5\u03bd\u03c8\u03c5\u0020\u03b2\u03c1\u03bf\u03b1\u03b4\u03c8\u03b1\u03c3\u03c4\u0020\u03c3\u03c5\u03c3\u03c4\u03b5\u03bc\u002e\u0020\u05e4\u05da\u05e7\u05e9\u05d3\u05e7\u0020\u05dc\u05e7\u05e7\u05e4\u0020\u05d8\u05dd\u05d5\u05e8\u0020\u05d9\u05e9\u05de\u05d2\u05d3\u0020\u05df\u05de\u05d3\u05df\u05d2\u05e7\u0020\u05d0\u05d9\u05e7\u0020\u05d4\u05e7\u05d9\u05df\u05d1\u05da\u05e7\u0020\u05e9\u05d0\u0020\u05e9\u05da\u05da\u0020\u05d0\u05df\u05e6\u05e7\u05d3";
  std::wstring wstr_expected(expected);
  ASSERT_EQ(wstr_file, wstr_expected);
  ASSERT_EQ(sample.seq1.length(), 7u);
  ASSERT_EQ(sample.seq1[0], 1);
  ASSERT_EQ(sample.seq1[1], 1);
  ASSERT_EQ(sample.seq1[2], 2);
  ASSERT_EQ(sample.seq1[3], 3);
  ASSERT_EQ(sample.seq1[4], 5);
  ASSERT_EQ(sample.seq1[5], 8);
  ASSERT_EQ(sample.seq1[6], 13);
  ASSERT_EQ(sample.seq2.length(), 3u);
  ASSERT_EQ(sample.seq2[0], 42);
  ASSERT_EQ(sample.seq2[1], 777);
  ASSERT_EQ(sample.seq2[2], 123454321);
  ASSERT_EQ(sample.seq3.length(), 8u);
  ASSERT_EQ(sample.seq3[0], 1);
  ASSERT_EQ(sample.seq3[1], 1);
  ASSERT_EQ(sample.seq3[2], 2);
  ASSERT_EQ(sample.seq3[3], 3);
  ASSERT_EQ(sample.seq3[4], 5);
  ASSERT_EQ(sample.seq3[5], 8);
  ASSERT_EQ(sample.seq3[6], 13);
  ASSERT_EQ(sample.seq3[7], 21);
  ASSERT_EQ(sample.ns.length(), 2u);
  ASSERT_EQ(sample.ns[0].length(), 3u);
  ASSERT_EQ(std::string(sample.ns[0][0].in()), "alvin");
  ASSERT_EQ(std::string(sample.ns[0][1].in()), "simon");
  ASSERT_EQ(std::string(sample.ns[0][2].in()), "theodore");
  ASSERT_EQ(sample.ns[1].length(), 4u);
  ASSERT_EQ(std::string(sample.ns[1][0].in()), "leonardo");
  ASSERT_EQ(std::string(sample.ns[1][1].in()), "donatello");
  ASSERT_EQ(std::string(sample.ns[1][2].in()), "michelangelo");
  ASSERT_EQ(std::string(sample.ns[1][3].in()), "raphael");
  ASSERT_EQ(sample.mu._d(), Mod::four);
  ASSERT_EQ(sample.mu.s().length(), 2u);
  ASSERT_EQ(sample.mu.s()[0].length(), 3u);
  ASSERT_EQ(std::string(sample.mu.s()[0][0].in()), "alpha");
  ASSERT_EQ(std::string(sample.mu.s()[0][1].in()), "beta");
  ASSERT_EQ(std::string(sample.mu.s()[0][2].in()), "gamma");
  ASSERT_EQ(sample.mu.s()[1].length(), 4u);
  ASSERT_EQ(std::string(sample.mu.s()[1][0].in()), "alef");
  ASSERT_EQ(std::string(sample.mu.s()[1][1].in()), "bet");
  ASSERT_EQ(std::string(sample.mu.s()[1][2].in()), "gimel");
  ASSERT_EQ(std::string(sample.mu.s()[1][3].in()), "dalet");
  ASSERT_EQ(sample.ca[0], 'f');
  ASSERT_EQ(sample.ca[1], 'e');
  ASSERT_EQ(sample.ca[2], 'd');
  ASSERT_EQ(sample.ca[3], 'C');
  ASSERT_EQ(sample.ca[4], 'B');
  ASSERT_EQ(sample.ca[5], 'A');
  ASSERT_EQ(std::string(sample.sa[0].in()), "north");
  ASSERT_EQ(std::string(sample.sa[1].in()), "east");
  ASSERT_EQ(std::string(sample.sa[2].in()), "south");
  ASSERT_EQ(std::string(sample.sa[3].in()), "west");
}

void initialize_sample(Mod::Sample& sample)
{
  sample.id = 5;
  sample.data = "The most rapid of JSONs";
  sample.enu = Mod::three;
  sample.enu2 = Mod::two;
  sample.bt.o = 129;
#if OPENDDS_HAS_EXPLICIT_INTS
  sample.bt.u8 = 130;
  sample.bt.i8 = -50;
#endif
  sample.bt.us = 32777u;
  sample.bt.s = -32765;
  sample.bt.ul = 2147483690u;
  sample.bt.l = -2147483632;
  sample.bt.ull = 9223372036854775810ull;
  sample.bt.ll = -9223372036854775806ll;
  sample.bt.f = 0.00141f;
  sample.bt.d = -0.00000141;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(sample.bt.ld, 1e34);
  sample.bt.b = false;
  sample.bt.c = '\r';
  sample.bt.str = "The most JSON of rapids";
  sample.bt.wstr = L"\u03a4\u03b7\u03b9\u03c3\u0020\u03b9\u03c3\u0020\u03b1\u0020\u03c4\u03b5\u03c3\u03c4\u0020\u03bf\u03c6\u0020\u03c4\u03b7\u03b5\u0020\u03b5\u03bc\u03b5\u03c1\u03b3\u03b5\u03bd\u03c8\u03c5\u0020\u03b2\u03c1\u03bf\u03b1\u03b4\u03c8\u03b1\u03c3\u03c4\u0020\u03c3\u03c5\u03c3\u03c4\u03b5\u03bc\u002e\u0020\u05e4\u05da\u05e7\u05e9\u05d3\u05e7\u0020\u05dc\u05e7\u05e7\u05e4\u0020\u05d8\u05dd\u05d5\u05e8\u0020\u05d9\u05e9\u05de\u05d2\u05d3\u0020\u05df\u05de\u05d3\u05df\u05d2\u05e7\u0020\u05d0\u05d9\u05e7\u0020\u05d4\u05e7\u05d9\u05df\u05d1\u05da\u05e7\u0020\u05e9\u05d0\u0020\u05e9\u05da\u05da\u0020\u05d0\u05df\u05e6\u05e7\u05d3";
  sample.seq1.length(7);
  sample.seq1[0] = 1;
  sample.seq1[1] = 1;
  sample.seq1[2] = 2;
  sample.seq1[3] = 3;
  sample.seq1[4] = 5;
  sample.seq1[5] = 8;
  sample.seq1[6] = 13;
  sample.seq2.length(3);
  sample.seq2[0] = 42;
  sample.seq2[1] = 777;
  sample.seq2[2] = 123454321;
  sample.seq3.length(8);
  sample.seq3[0] = 1;
  sample.seq3[1] = 1;
  sample.seq3[2] = 2;
  sample.seq3[3] = 3;
  sample.seq3[4] = 5;
  sample.seq3[5] = 8;
  sample.seq3[6] = 13;
  sample.seq3[7] = 21;
  sample.ns.length(2);
  sample.ns[0].length(3);
  sample.ns[0][0] = "alvin";
  sample.ns[0][1] = "simon";
  sample.ns[0][2] = "theodore";
  sample.ns[1].length(4);
  sample.ns[1][0] = "leonardo";
  sample.ns[1][1] = "donatello";
  sample.ns[1][2] = "michelangelo";
  sample.ns[1][3] = "raphael";
  sample.mu._d(Mod::three);
  sample.mu.d(5678);
  sample.ca[0] = 'f';
  sample.ca[1] = 'e';
  sample.ca[2] = 'd';
  sample.ca[3] = 'C';
  sample.ca[4] = 'B';
  sample.ca[5] = 'A';
  sample.sa[0] = "north";
  sample.sa[1] = "east";
  sample.sa[2] = "south";
  sample.sa[3] = "west";
}

void verify_parse_result(const rapidjson::Value& val)
{
  ASSERT_EQ(val.IsObject(), true);
  ASSERT_EQ(val["id"], 5);
  ASSERT_EQ(val["data"], "The most rapid of JSONs");
  ASSERT_EQ(val["enu"], "three");
  ASSERT_EQ(val["enu2"], "two");
  ASSERT_EQ(val["bt"]["o"], 129);
#if OPENDDS_HAS_EXPLICIT_INTS
  ASSERT_EQ(val["bt"]["u8"], 130);
  ASSERT_EQ(val["bt"]["i8"], -50);
#endif
  ASSERT_EQ(val["bt"]["us"], 32777u);
  ASSERT_EQ(val["bt"]["s"], -32765);
  ASSERT_EQ(val["bt"]["ul"], 2147483690u);
  ASSERT_EQ(val["bt"]["l"], -2147483632);
  ASSERT_EQ(val["bt"]["ull"].IsUint64(), true);
  ASSERT_EQ(val["bt"]["ull"].GetUint64(), 9223372036854775810ull);
  ASSERT_EQ(val["bt"]["ll"].IsInt64(), true);
  ASSERT_EQ(val["bt"]["ll"].GetInt64(), -9223372036854775806ll);
  ASSERT_EQ(val["bt"]["f"].IsFloat(), true);
  ASSERT_LT(val["bt"]["f"].GetFloat(), 0.00141 + 1e-6);
  ASSERT_GT(val["bt"]["f"].GetFloat(), 0.00141 - 1e-6);
  ASSERT_EQ(val["bt"]["d"].IsDouble(), true);
  ASSERT_LT(val["bt"]["d"].GetDouble(), -0.00000141 + 1e-9);
  ASSERT_GT(val["bt"]["d"].GetDouble(), -0.00000141 - 1e-9);
  ASSERT_EQ(val["bt"]["b"], false);
  ASSERT_EQ(val["bt"]["c"], "\r");
  ASSERT_EQ(val["bt"]["str"], "The most JSON of rapids");
  ASSERT_EQ(val["bt"]["wstr"], "Τηισ ισ α τεστ οφ τηε εμεργενψυ βροαδψαστ συστεμ. פךקשדק לקקפ טםור ישמגד ןמדןגק איק הקיןבךק שא שךך אןצקד");
  ASSERT_EQ(val["seq1"].Size(), 7u);
  ASSERT_EQ(val["seq1"][0], 1);
  ASSERT_EQ(val["seq1"][1], 1);
  ASSERT_EQ(val["seq1"][2], 2);
  ASSERT_EQ(val["seq1"][3], 3);
  ASSERT_EQ(val["seq1"][4], 5);
  ASSERT_EQ(val["seq1"][5], 8);
  ASSERT_EQ(val["seq1"][6], 13);
  ASSERT_EQ(val["seq2"].Size(), 3u);
  ASSERT_EQ(val["seq2"][0], 42);
  ASSERT_EQ(val["seq2"][1], 777);
  ASSERT_EQ(val["seq2"][2], 123454321);
  ASSERT_EQ(val["seq3"][0], 1);
  ASSERT_EQ(val["seq3"][1], 1);
  ASSERT_EQ(val["seq3"][2], 2);
  ASSERT_EQ(val["seq3"][3], 3);
  ASSERT_EQ(val["seq3"][4], 5);
  ASSERT_EQ(val["seq3"][5], 8);
  ASSERT_EQ(val["seq3"][6], 13);
  ASSERT_EQ(val["seq3"][7], 21);
  ASSERT_EQ(val["ns"].Size(), 2u);
  ASSERT_EQ(val["ns"][0].Size(), 3u);
  ASSERT_EQ(val["ns"][0][0], "alvin");
  ASSERT_EQ(val["ns"][0][1], "simon");
  ASSERT_EQ(val["ns"][0][2], "theodore");
  ASSERT_EQ(val["ns"][1].Size(), 4u);
  ASSERT_EQ(val["ns"][1][0], "leonardo");
  ASSERT_EQ(val["ns"][1][1], "donatello");
  ASSERT_EQ(val["ns"][1][2], "michelangelo");
  ASSERT_EQ(val["ns"][1][3], "raphael");
  ASSERT_EQ(val["mu"]["$discriminator"], "three");
  ASSERT_EQ(val["mu"]["d"], 5678);
  ASSERT_EQ(val["ca"][0], "f");
  ASSERT_EQ(val["ca"][1], "e");
  ASSERT_EQ(val["ca"][2], "d");
  ASSERT_EQ(val["ca"][3], "C");
  ASSERT_EQ(val["ca"][4], "B");
  ASSERT_EQ(val["ca"][5], "A");
  ASSERT_EQ(val["sa"][0], "north");
  ASSERT_EQ(val["sa"][1], "east");
  ASSERT_EQ(val["sa"][2], "south");
  ASSERT_EQ(val["sa"][3], "west");
}

TEST(VreadVwriteTest, StaticSerializeTest)
{
  Mod::Sample sample;
  initialize_sample(sample);

  // Serialize to JSON string.
  Mod::SampleTypeSupport_var ts = new Mod::SampleTypeSupportImpl;
  OpenDDS::DCPS::RepresentationFormat_var format = ts->make_format(OpenDDS::DCPS::JSON_DATA_REPRESENTATION);
  CORBA::String_var buffer;
  ASSERT_EQ(DDS::RETCODE_OK, ts->encode_to_string(sample, buffer, format));

  DDS::OctetSeq_var bytes;
  ASSERT_EQ(DDS::RETCODE_OK, ts->encode_to_bytes(sample, bytes, format));
  const unsigned int n = bytes->length();
  ASSERT_EQ(n, std::strlen(buffer));
  ASSERT_EQ(0, std::memcmp(buffer.in(), bytes->get_buffer(), n));

  // Then parse.
  rapidjson::Document document;
  document.Parse(buffer.in());
  rapidjson::Value& val = document;
  verify_parse_result(val);

  //rapidjson::StringBuffer buffer;
  //rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  //val.Accept(writer);
  //std::string output(buffer.GetString());
  //std::cout << output << std::endl;
}

template <typename T>
void write_helper(const T& sample, CORBA::String_out out)
{
  OpenDDS::DCPS::KeyOnly<const T> keyonly(sample);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  OpenDDS::DCPS::JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);

  ASSERT_TRUE(vwrite(jvw, keyonly));
  out = buffer.GetString();
}

TEST(VreadVwriteTest, KeyOnly_StructWithNoKeys)
{
  Mod::NoExplicitKeysStruct sample;
  OpenDDS::DCPS::set_default(sample);

  // Write the KeyOnly sample to JSON. Then parse the result and compare with the original sample.
  // In this case no fields are serialized.
  CORBA::String_var result;
  write_helper(sample, result);
  rapidjson::Document document;
  document.Parse(result.in());
  rapidjson::Value& val = document;

  ASSERT_TRUE(val.IsObject());
  ASSERT_TRUE(val.ObjectEmpty());

  // Read from the JSON result with vread
  rapidjson::StringStream buffer(result.in());
  OpenDDS::DCPS::JsonValueReader<> jvr(buffer);
  Mod::NoExplicitKeysStruct sample_out;
  OpenDDS::DCPS::set_default(sample_out);
  const OpenDDS::DCPS::KeyOnly<Mod::NoExplicitKeysStruct> keyonly_out(sample_out);
  ASSERT_TRUE(vread(jvr, keyonly_out));
  ASSERT_EQ(sample.b, sample_out.b);
  ASSERT_EQ(sample.c, sample_out.c);
  ASSERT_EQ(sample.l, sample_out.l);
}

TEST(VreadVwriteTest, KeyOnly_StructWithKeys)
{
  Mod::KeyOnlyStruct sample;
  OpenDDS::DCPS::set_default(sample);
  sample.id = 1;
  sample.eks.s = 10;
  sample.eks.str = "eks.str";
  sample.neks.b = true;
  sample.neks.c = 'a';
  sample.neks.l = 12l;
  sample.eksarr[0][0].s = 1;
  sample.eksarr[0][0].str = "eksarr[0][0].str";
  sample.eksarr[0][1].s = 2;
  sample.eksarr[0][1].str = "eksarr[0][1].str";
  sample.eksarr[1][0].s = 3;
  sample.eksarr[1][0].str = "eksarr[1][0].str";
  sample.eksarr[1][1].s = 4;
  sample.eksarr[1][1].str = "eksarr[1][1].str";
  sample.eku.c('d');
  sample.neku.b(false);
  sample.str = "hello";

  // Write the KeyOnly sample to JSON. Then parse and check the result against the input.
  CORBA::String_var result;
  write_helper(sample, result);
  rapidjson::Document document;
  document.Parse(result.in());
  rapidjson::Value& val = document;

  ASSERT_TRUE(val.IsObject());
  ASSERT_EQ(7ul, val.MemberCount());
  ASSERT_EQ(val["id"], 1);
  ASSERT_EQ(2ul, val["eks"].MemberCount());
  ASSERT_EQ(val["eks"]["s"], 10);
  ASSERT_EQ(val["eks"]["str"], "eks.str");
  ASSERT_EQ(val["neks"]["b"], true);
  ASSERT_EQ(val["neks"]["c"], "a");
  ASSERT_EQ(val["neks"]["l"], 12);
  ASSERT_EQ(2ul, val["eksarr"].Size());
  ASSERT_EQ(2ul, val["eksarr"][0].Size());
  ASSERT_EQ(2ul, val["eksarr"][1].Size());
  ASSERT_EQ(2ul, val["eksarr"][0][0].MemberCount());
  ASSERT_EQ(val["eksarr"][0][0]["s"], 1);
  ASSERT_EQ(val["eksarr"][0][0]["str"], "eksarr[0][0].str");
  ASSERT_EQ(2ul, val["eksarr"][0][1].MemberCount());
  ASSERT_EQ(val["eksarr"][0][1]["s"], 2);
  ASSERT_EQ(val["eksarr"][0][1]["str"], "eksarr[0][1].str");
  ASSERT_EQ(2ul, val["eksarr"][1][0].MemberCount());
  ASSERT_EQ(val["eksarr"][1][0]["s"], 3);
  ASSERT_EQ(val["eksarr"][1][0]["str"], "eksarr[1][0].str");
  ASSERT_EQ(2ul, val["eksarr"][1][1].MemberCount());
  ASSERT_EQ(val["eksarr"][1][1]["s"], 4);
  ASSERT_EQ(val["eksarr"][1][1]["str"], "eksarr[1][1].str");
  ASSERT_EQ(1ul, val["eku"].MemberCount());
  ASSERT_EQ(val["eku"]["$discriminator"], "two");
  ASSERT_EQ(1ul, val["neku"].MemberCount());
  ASSERT_EQ(val["neku"]["$discriminator"], "one");
  ASSERT_EQ(val["str"], "hello");

  // Read from the JSON result into a KeyOnly sample and compare with the original sample.
  rapidjson::StringStream buffer(result.in());
  OpenDDS::DCPS::JsonValueReader<> jvr(buffer);
  Mod::KeyOnlyStruct sample_out;
  OpenDDS::DCPS::set_default(sample_out);
  const OpenDDS::DCPS::KeyOnly<Mod::KeyOnlyStruct> keyonly_out(sample_out);

  ASSERT_TRUE(vread(jvr, keyonly_out));
  ASSERT_EQ(sample.id, sample_out.id);
  ASSERT_EQ(sample.eks.s, sample_out.eks.s);
  ASSERT_STREQ(sample.eks.str, sample_out.eks.str);
  ASSERT_EQ(sample.neks.b, sample_out.neks.b);
  ASSERT_EQ(sample.neks.c, sample_out.neks.c);
  ASSERT_EQ(sample.neks.l, sample_out.neks.l);
  ASSERT_EQ(sample.eksarr[0][0].s, sample_out.eksarr[0][0].s);
  ASSERT_STREQ(sample.eksarr[0][0].str, sample_out.eksarr[0][0].str);
  ASSERT_EQ(sample.eksarr[0][1].s, sample_out.eksarr[0][1].s);
  ASSERT_STREQ(sample.eksarr[0][1].str, sample_out.eksarr[0][1].str);
  ASSERT_EQ(sample.eksarr[1][0].s, sample_out.eksarr[1][0].s);
  ASSERT_STREQ(sample.eksarr[1][0].str, sample_out.eksarr[1][0].str);
  ASSERT_EQ(sample.eksarr[1][1].s, sample_out.eksarr[1][1].s);
  ASSERT_STREQ(sample.eksarr[1][1].str, sample_out.eksarr[1][1].str);
  ASSERT_EQ(sample.eku._d(), sample_out.eku._d());
  ASSERT_EQ(sample.neku._d(), sample_out.neku._d());
  ASSERT_STREQ(sample.str, sample_out.str);
}

TEST(VreadVwriteTest, KeyOnly_UnionWithNoKey)
{
  Mod::NoExplicitKeyUnion sample;
  OpenDDS::DCPS::set_default(sample);

  CORBA::String_var result;
  write_helper(sample, result);
  rapidjson::Document document;
  document.Parse(result.in());
  rapidjson::Value& val = document;

  ASSERT_TRUE(val.IsObject());
  ASSERT_TRUE(val.ObjectEmpty());

  rapidjson::StringStream buffer(result.in());
  OpenDDS::DCPS::JsonValueReader<> jvr(buffer);
  Mod::NoExplicitKeyUnion sample_out;
  OpenDDS::DCPS::set_default(sample_out);
  const OpenDDS::DCPS::KeyOnly<Mod::NoExplicitKeyUnion> keyonly_out(sample_out);
  ASSERT_TRUE(vread(jvr, keyonly_out));
  ASSERT_EQ(sample._d(), sample_out._d());
}

TEST(VreadVwriteTest, KeyOnly_UnionWithKey)
{
  Mod::ExplicitKeyUnion sample;
  sample.o(0x22);
  sample._d(Mod::three);

  CORBA::String_var result;
  write_helper(sample, result);
  rapidjson::Document document;
  document.Parse(result.in());
  rapidjson::Value& val = document;

  ASSERT_TRUE(val.IsObject());
  ASSERT_EQ(1ul, val.MemberCount());
  ASSERT_EQ(val["$discriminator"], "three");

  rapidjson::StringStream buffer(result.in());
  OpenDDS::DCPS::JsonValueReader<> jvr(buffer);
  Mod::ExplicitKeyUnion sample_out;
  OpenDDS::DCPS::set_default(sample_out);
  const OpenDDS::DCPS::KeyOnly<Mod::ExplicitKeyUnion> keyonly_out(sample_out);
  ASSERT_TRUE(vread(jvr, keyonly_out));
  ASSERT_EQ(sample._d(), sample_out._d());
}

#ifndef OPENDDS_SAFETY_PROFILE

DDS::DynamicType_var get_dynamic_type(OpenDDS::XTypes::TypeLookupService& tls)
{
  using namespace OpenDDS;
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::Mod_Sample_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::Mod_Sample_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  tls.add(type_map.begin(), type_map.end());
  return tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());
}

TEST(VreadVwriteTest, DynamicSerializeTest)
{
  // Set up the dynamic data object
  Mod::Sample sample;
  initialize_sample(sample);

  OpenDDS::XTypes::TypeLookupService tls;
  DDS::DynamicType_var dt = get_dynamic_type(tls);
  OpenDDS::XTypes::DynamicDataImpl dd(dt);
  DDS::DynamicTypeMember_var dtm;
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "id"));
  ASSERT_EQ(DDS::RETCODE_OK, dd.set_int32_value(dtm->get_id(), sample.id));
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "data"));
  ASSERT_EQ(DDS::RETCODE_OK, dd.set_string_value(dtm->get_id(), sample.data));
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "enu"));
  ASSERT_EQ(DDS::RETCODE_OK, dd.set_int32_value(dtm->get_id(), sample.enu));
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "enu2"));
  ASSERT_EQ(DDS::RETCODE_OK, dd.set_int32_value(dtm->get_id(), sample.enu2));

  // bt
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "bt"));
  DDS::DynamicData_var bt_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(bt_dd, dtm->get_id()));
  DDS::DynamicType_var bt_dt = bt_dd->type();

  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "o"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_byte_value(dtm->get_id(), sample.bt.o));
#ifdef OPENDDS_HAS_EXPLICIT_INTS
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "u8"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_uint8_value(dtm->get_id(), sample.bt.u8));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "i8"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_int8_value(dtm->get_id(), sample.bt.i8));
#endif
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "us"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_uint16_value(dtm->get_id(), sample.bt.us));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "s"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_int16_value(dtm->get_id(), sample.bt.s));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "ul"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_uint32_value(dtm->get_id(), sample.bt.ul));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "l"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_int32_value(dtm->get_id(), sample.bt.l));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "ull"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_uint64_value(dtm->get_id(), sample.bt.ull));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "ll"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_int64_value(dtm->get_id(), sample.bt.ll));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "f"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_float32_value(dtm->get_id(), sample.bt.f));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "d"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_float64_value(dtm->get_id(), sample.bt.d));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "ld"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_float128_value(dtm->get_id(), sample.bt.ld));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "b"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_boolean_value(dtm->get_id(), sample.bt.b));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "c"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_char8_value(dtm->get_id(), sample.bt.c));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "str"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_string_value(dtm->get_id(), sample.bt.str));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dt->get_member_by_name(dtm, "wstr"));
  ASSERT_EQ(DDS::RETCODE_OK, bt_dd->set_wstring_value(dtm->get_id(), sample.bt.wstr));

  // seq1
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "seq1"));
  DDS::DynamicData_var seq1_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(seq1_dd, dtm->get_id()));
  ASSERT_EQ(DDS::RETCODE_OK, seq1_dd->set_int16_value(seq1_dd->get_member_id_at_index(0), sample.seq1[0]));
  ASSERT_EQ(DDS::RETCODE_OK, seq1_dd->set_int16_value(seq1_dd->get_member_id_at_index(1), sample.seq1[1]));
  ASSERT_EQ(DDS::RETCODE_OK, seq1_dd->set_int16_value(seq1_dd->get_member_id_at_index(2), sample.seq1[2]));
  ASSERT_EQ(DDS::RETCODE_OK, seq1_dd->set_int16_value(seq1_dd->get_member_id_at_index(3), sample.seq1[3]));
  ASSERT_EQ(DDS::RETCODE_OK, seq1_dd->set_int16_value(seq1_dd->get_member_id_at_index(4), sample.seq1[4]));
  ASSERT_EQ(DDS::RETCODE_OK, seq1_dd->set_int16_value(seq1_dd->get_member_id_at_index(5), sample.seq1[5]));
  ASSERT_EQ(DDS::RETCODE_OK, seq1_dd->set_int16_value(seq1_dd->get_member_id_at_index(6), sample.seq1[6]));

  // seq2
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "seq2"));
  DDS::DynamicData_var seq2_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(seq2_dd, dtm->get_id()));
  ASSERT_EQ(DDS::RETCODE_OK, seq2_dd->set_int32_value(seq2_dd->get_member_id_at_index(0), sample.seq2[0]));
  ASSERT_EQ(DDS::RETCODE_OK, seq2_dd->set_int32_value(seq2_dd->get_member_id_at_index(1), sample.seq2[1]));
  ASSERT_EQ(DDS::RETCODE_OK, seq2_dd->set_int32_value(seq2_dd->get_member_id_at_index(2), sample.seq2[2]));

  // seq3
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "seq3"));
  DDS::DynamicData_var seq3_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(seq3_dd, dtm->get_id()));
  ASSERT_EQ(DDS::RETCODE_OK, seq3_dd->set_int16_value(seq3_dd->get_member_id_at_index(0), sample.seq3[0]));
  ASSERT_EQ(DDS::RETCODE_OK, seq3_dd->set_int16_value(seq3_dd->get_member_id_at_index(1), sample.seq3[1]));
  ASSERT_EQ(DDS::RETCODE_OK, seq3_dd->set_int16_value(seq3_dd->get_member_id_at_index(2), sample.seq3[2]));
  ASSERT_EQ(DDS::RETCODE_OK, seq3_dd->set_int16_value(seq3_dd->get_member_id_at_index(3), sample.seq3[3]));
  ASSERT_EQ(DDS::RETCODE_OK, seq3_dd->set_int16_value(seq3_dd->get_member_id_at_index(4), sample.seq3[4]));
  ASSERT_EQ(DDS::RETCODE_OK, seq3_dd->set_int16_value(seq3_dd->get_member_id_at_index(5), sample.seq3[5]));
  ASSERT_EQ(DDS::RETCODE_OK, seq3_dd->set_int16_value(seq3_dd->get_member_id_at_index(6), sample.seq3[6]));
  ASSERT_EQ(DDS::RETCODE_OK, seq3_dd->set_int16_value(seq3_dd->get_member_id_at_index(7), sample.seq3[7]));

  // ns
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "ns"));
  DDS::DynamicData_var ns_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(ns_dd, dtm->get_id()));
  DDS::DynamicData_var ns_elem_dd;
  ASSERT_EQ(DDS::RETCODE_OK, ns_dd->get_complex_value(ns_elem_dd, ns_dd->get_member_id_at_index(0)));
  ASSERT_EQ(DDS::RETCODE_OK, ns_elem_dd->set_string_value(ns_elem_dd->get_member_id_at_index(0), sample.ns[0][0]));
  ASSERT_EQ(DDS::RETCODE_OK, ns_elem_dd->set_string_value(ns_elem_dd->get_member_id_at_index(1), sample.ns[0][1]));
  ASSERT_EQ(DDS::RETCODE_OK, ns_elem_dd->set_string_value(ns_elem_dd->get_member_id_at_index(2), sample.ns[0][2]));
  ASSERT_EQ(DDS::RETCODE_OK, ns_dd->get_complex_value(ns_elem_dd, ns_dd->get_member_id_at_index(1)));
  ASSERT_EQ(DDS::RETCODE_OK, ns_elem_dd->set_string_value(ns_elem_dd->get_member_id_at_index(0), sample.ns[1][0]));
  ASSERT_EQ(DDS::RETCODE_OK, ns_elem_dd->set_string_value(ns_elem_dd->get_member_id_at_index(1), sample.ns[1][1]));
  ASSERT_EQ(DDS::RETCODE_OK, ns_elem_dd->set_string_value(ns_elem_dd->get_member_id_at_index(2), sample.ns[1][2]));
  ASSERT_EQ(DDS::RETCODE_OK, ns_elem_dd->set_string_value(ns_elem_dd->get_member_id_at_index(3), sample.ns[1][3]));

  // mu
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "mu"));
  DDS::DynamicData_var mu_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(mu_dd, dtm->get_id()));
  DDS::DynamicType_var mu_dt = mu_dd->type();
  ASSERT_EQ(DDS::RETCODE_OK, mu_dt->get_member_by_name(dtm, "d"));
  ASSERT_EQ(DDS::RETCODE_OK, mu_dd->set_float64_value(dtm->get_id(), sample.mu.d()));
  CORBA::Long disc = 0;
  ASSERT_EQ(DDS::RETCODE_OK, mu_dd->get_int32_value(disc, OpenDDS::XTypes::DISCRIMINATOR_ID));
  ASSERT_EQ(Mod::three, disc);

  // ca
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "ca"));
  DDS::DynamicData_var ca_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(ca_dd, dtm->get_id()));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(0), sample.ca[0]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(1), sample.ca[1]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(2), sample.ca[2]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(3), sample.ca[3]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(4), sample.ca[4]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(5), sample.ca[5]));

  // sa
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "sa"));
  DDS::DynamicData_var sa_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(sa_dd, dtm->get_id()));
  ASSERT_EQ(DDS::RETCODE_OK, sa_dd->set_string_value(sa_dd->get_member_id_at_index(0), sample.sa[0]));
  ASSERT_EQ(DDS::RETCODE_OK, sa_dd->set_string_value(sa_dd->get_member_id_at_index(1), sample.sa[1]));
  ASSERT_EQ(DDS::RETCODE_OK, sa_dd->set_string_value(sa_dd->get_member_id_at_index(2), sample.sa[2]));
  ASSERT_EQ(DDS::RETCODE_OK, sa_dd->set_string_value(sa_dd->get_member_id_at_index(3), sample.sa[3]));

  // Serialize to JSON format
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  OpenDDS::DCPS::JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);
  vwrite(jvw, &dd);

  // Parse the result string and compare with the original data
  rapidjson::Document document;
  document.Parse(buffer.GetString());
  rapidjson::Value& val = document;
  verify_parse_result(val);
}

TEST(VreadVwriteTest, DynamicWithOptionalTest)
{
  Mod::Sample sample;
  initialize_sample(sample);

  using namespace OpenDDS;
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::Mod_Sample_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::Mod_Sample_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::CompleteTypeObject cto = it->second.complete;

  // Manually mark members as optional
  cto.struct_type.member_seq[1].common.member_flags |= XTypes::IS_OPTIONAL; // data
  cto.struct_type.member_seq[3].common.member_flags |= XTypes::IS_OPTIONAL; // enu2
  cto.struct_type.member_seq[4].common.member_flags |= XTypes::IS_OPTIONAL; // bt
  cto.struct_type.member_seq[5].common.member_flags |= XTypes::IS_OPTIONAL; // seq1
  cto.struct_type.member_seq[6].common.member_flags |= XTypes::IS_OPTIONAL; // seq2
  cto.struct_type.member_seq[7].common.member_flags |= XTypes::IS_OPTIONAL; // seq3
  cto.struct_type.member_seq[8].common.member_flags |= XTypes::IS_OPTIONAL; // ns
  cto.struct_type.member_seq[9].common.member_flags |= XTypes::IS_OPTIONAL; // mu
  cto.struct_type.member_seq[11].common.member_flags |= XTypes::IS_OPTIONAL; // sa

  DDS::DynamicType_var dt = tls.complete_to_dynamic(cto, DCPS::GUID_t());
  XTypes::DynamicDataImpl dd(dt);
  DDS::DynamicTypeMember_var dtm;
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "id"));
  ASSERT_EQ(DDS::RETCODE_OK, dd.set_int32_value(dtm->get_id(), sample.id));
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "enu"));
  ASSERT_EQ(DDS::RETCODE_OK, dd.set_int32_value(dtm->get_id(), sample.enu));

  // ca
  ASSERT_EQ(DDS::RETCODE_OK, dt->get_member_by_name(dtm, "ca"));
  DDS::DynamicData_var ca_dd;
  ASSERT_EQ(DDS::RETCODE_OK, dd.get_complex_value(ca_dd, dtm->get_id()));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(0), sample.ca[0]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(1), sample.ca[1]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(2), sample.ca[2]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(3), sample.ca[3]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(4), sample.ca[4]));
  ASSERT_EQ(DDS::RETCODE_OK, ca_dd->set_char8_value(ca_dd->get_member_id_at_index(5), sample.ca[5]));

  // Serialize to JSON string
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  OpenDDS::DCPS::JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);
  vwrite(jvw, &dd);

  // Parse the result
  rapidjson::Document document;
  document.Parse(buffer.GetString());
  rapidjson::Value& val = document;

  ASSERT_TRUE(val.IsObject());
  ASSERT_EQ(val["id"], 5);
  ASSERT_TRUE(val["data"].IsNull());
  ASSERT_EQ(val["enu"], "three");
  ASSERT_TRUE(val["enu2"].IsNull());
  ASSERT_TRUE(val["bt"].IsNull());
  ASSERT_TRUE(val["seq1"].IsNull());
  ASSERT_TRUE(val["seq2"].IsNull());
  ASSERT_TRUE(val["seq3"].IsNull());
  ASSERT_TRUE(val["ns"].IsNull());
  ASSERT_TRUE(val["mu"].IsNull());
  ASSERT_EQ(val["ca"][0], "f");
  ASSERT_EQ(val["ca"][1], "e");
  ASSERT_EQ(val["ca"][2], "d");
  ASSERT_EQ(val["ca"][3], "C");
  ASSERT_EQ(val["ca"][4], "B");
  ASSERT_EQ(val["ca"][5], "A");
  ASSERT_TRUE(val["sa"].IsNull());
}

#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER

TEST(VreadVwriteTest, DynamicAdapterSerializeTest)
{
  // Set up the dynamic data adapter object
  OpenDDS::XTypes::TypeLookupService tls;
  DDS::DynamicType_var dt = get_dynamic_type(tls);
  Mod::Sample sample;
  initialize_sample(sample);
  DDS::DynamicData_var dd = OpenDDS::XTypes::get_dynamic_data_adapter<Mod::Sample, Mod::Sample>(dt, sample);

  // Serialize to JSON
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  OpenDDS::DCPS::JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);
  vwrite(jvw, dd.in());

  // Parse the result string
  rapidjson::Document document;
  document.Parse(buffer.GetString());
  rapidjson::Value& val = document;
  verify_parse_result(val);
}

#endif // OPENDDS_HAS_DYNAMIC_DATA_ADAPTER

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_HAS_JSON_VALUE_WRITER
