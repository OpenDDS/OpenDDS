#include "gtest/gtest.h"

#include "RapidJsonTestC.h"
#include "RapidJsonTestTypeSupportImpl.h"

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/writer.h>

#include <fstream>

TEST(RapidJsonTest, ParseTest)
{
  std::ifstream ifs("RapidJsonTest.json");

  ASSERT_EQ(ifs.good(), true);

  rapidjson::Document doc;
  rapidjson::IStreamWrapper isw(ifs);

  doc.ParseStream(isw);
  ASSERT_EQ(doc.IsObject(), true);

  Mod::Sample sample;
  OpenDDS::DCPS::copyFromRapidJson(doc, sample);

  ASSERT_EQ(sample.id, 5);
  ASSERT_EQ(std::string(sample.data.in()), "The most rapid of JSONs");
  ASSERT_EQ(sample.enu, Mod::three);
  ASSERT_EQ(sample.enu2, Mod::two);
  ASSERT_EQ(sample.bt.o, 129);
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
  ASSERT_EQ(sample.bt.ld, 1e34);
  ASSERT_EQ(sample.bt.b, false);
  ASSERT_EQ(sample.bt.c, '\r');
  ASSERT_EQ(std::string(sample.bt.str.in()), "The most JSON of rapids");
  ASSERT_EQ(std::wstring(sample.bt.wstr.in()), L"Τηισ ισ α τεστ οφ τηε εμεργενψυ βροαδψαστ συστεμ. פךקשדק לקקפ טםור ישמגד ןמדןגק איק הקיןבךק שא שךך אןצקד");
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
  sample.bt.wstr = L"Τηισ ισ α τεστ οφ τηε εμεργενψυ βροαδψαστ συστεμ. פךקשדק לקקפ טםור ישמגד ןמדןגק איק הקיןבךק שא שךך אןצקד";
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
