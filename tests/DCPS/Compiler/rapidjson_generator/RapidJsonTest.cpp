#include "gtest/gtest.h"

#include "RapidJsonTestC.h"
#include "RapidJsonTestTypeSupportImpl.h"

#include <dds/DCPS/RapidJsonWrapper.h>
#include <dds/DCPS/Definitions.h>

#include <fstream>

TEST(RapidJsonTest, ParseTest)
{
  std::ifstream ifs("RapidJsonTest.json", std::ios_base::in | std::ios_base::binary);

  ASSERT_EQ(ifs.good(), true);

  rapidjson::Document doc;
  rapidjson::IStreamWrapper isw(ifs);

  doc.ParseStream<rapidjson::kParseDefaultFlags , rapidjson::UTF8<>, rapidjson::IStreamWrapper>(isw);
  ASSERT_EQ(doc.IsObject(), true);

  Mod::Sample sample;
  OpenDDS::DCPS::copyFromRapidJson(doc, sample);

  ASSERT_EQ(sample.id, 5);
  ASSERT_EQ(std::string(sample.data.in()), "The most rapid of JSONs");
  ASSERT_EQ(sample.enu, Mod::three);
  ASSERT_EQ(sample.enu2, Mod::two);
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

TEST(RapidJsonTest, SerializeTest)
{
  Mod::Sample sample;

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

  rapidjson::Value val(rapidjson::kObjectType);
  rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> alloc;
  OpenDDS::DCPS::copyToRapidJson(sample, val, alloc);

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
  ASSERT_EQ(val["mu"]["_d"], "three");
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

  //rapidjson::StringBuffer buffer;
  //rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  //val.Accept(writer);
  //std::string output(buffer.GetString());
  //std::cout << output << std::endl;
}
