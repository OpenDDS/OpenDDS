#include "dds/DCPS/FileSystemStorage.h"

#include "ace/Log_Msg.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

bool encode_test(const char* plaintext, const char* expected)
{
  using namespace OpenDDS::FileSystemStorage;

  ACE_TString result = b32h_encode(ACE_TEXT_CHAR_TO_TCHAR(plaintext));
  ACE_DEBUG((LM_DEBUG, "encoded {%C} =>\t{%s}\n", plaintext, result.c_str()));
  ACE_TString exp(ACE_TEXT_CHAR_TO_TCHAR(expected));
  if (exp != result)
    {
      ACE_DEBUG ((LM_DEBUG, "\tshould be\t{%C}\n", expected));
      return false;
    }
  ACE_TString round_trip = b32h_decode(result.c_str());
  ACE_TString exp2(ACE_TEXT_CHAR_TO_TCHAR(plaintext));
  if (exp2 != round_trip)
    {
      ACE_DEBUG((LM_DEBUG, "decoding returned {%s}\n", round_trip.c_str()));
      return false;
    }
  return true;
}


int ACE_TMAIN(int, ACE_TCHAR*[])
{
  using namespace OpenDDS::FileSystemStorage;

  bool ok(true);

  // From RFC 4648
  ok &= encode_test("", "");
  ok &= encode_test("f", "CO======");
  ok &= encode_test("fo", "CPNG====");
  ok &= encode_test("foo", "CPNMU===");
  ok &= encode_test("foob", "CPNMUOG=");
  ok &= encode_test("fooba", "CPNMUOJ1");
  ok &= encode_test("foobar", "CPNMUOJ1E8======");

  ok &= encode_test("The[quIck]brOwn-fox?jumPes\\oVer The/lazy dog!",
    "AHK6AMRHEL4M6QQTC9P4UTRE5LJ6UU1VD9QMQK35EDE6ULJ5E8G58Q355TM62UJP41I6UPP1");
  //^-> verified with Tcl's Base32hex encoder

  const char big_name[] =
    "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod "
    "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
    "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
    "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
    "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
    "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
    "mollit anim id est laborum.";

  ACE_DEBUG((LM_DEBUG, "Testing file and directory operations...\n"));
  try
    {
      std::vector<std::string> path;
      path.push_back("subdir2");
      path.push_back("subsubdir");
      {
        Directory::Ptr d = Directory::create("test");
        Directory::Ptr d2 = d->get_subdir(big_name);
        {
          File::Ptr f1 = d->get_file("00my/file");
          std::ofstream os;
          if (!f1->write(os))
            throw std::runtime_error("Can't write");
          os << "Hello world.\n";
        }
        for (int i(0); i < OPENDDS_FILESYSTEMSTORAGE_MAX_FILES_PER_DIR * 2; ++i)
          {
            std::ostringstream fname;
            fname << "File" << i;
            d->get_file(fname.str().c_str());
          }
        Directory::Ptr subdir2 = d->get_subdir(path[0].c_str());
        Directory::Ptr d3 = subdir2->create_next_dir();
        if (d3->parent() != subdir2)
          throw std::runtime_error("parent is inconsistent");
        d3->create_next_file();
        File::Ptr f2 = subdir2->get_subdir(path[1].c_str())->get_file("foo");
        std::ofstream os2;
        if (!f2->write(os2))
          throw std::runtime_error("Can't write");
        os2 << "Sample data.\n";
      }
      {
        Directory::Ptr d = Directory::create("test");
        for (Directory::FileIterator iter = d->begin_files(),
             end = d->end_files(); iter != end; ++iter)
          {
            if (iter == d->begin_files())
              {
                std::ifstream is;
                if (!(*iter)->read(is))
                  throw std::runtime_error("Can't read");
                std::string line;
                getline(is, line);
                if (line != "Hello world.")
                  throw std::runtime_error("read bad data");
              }
            else
              {
                if (iter->parent() != d)
                  throw std::runtime_error("parent is inconsistent");
              }
          }
        {
          Directory::Ptr subdir = d->get_dir(path);
          File::Ptr f2 = subdir->get_file("foo");
          std::ifstream is;
          if (!f2->read(is))
            throw std::runtime_error("Can't read");
          std::string line;
          getline(is, line);
          if (line != "Sample data.")
            throw std::runtime_error("read bad data");
          f2->remove();
        }

        unsigned int dircount(0);
        for (Directory::DirectoryIterator iter = d->begin_dirs(),
             end = d->end_dirs(); !(iter == end); iter++)
          {
            ++dircount;
          }
        if (dircount != 2)
          throw std::runtime_error("got wrong # of subdirs of 'test'");

        d->remove();
      }
    }
  catch (const std::exception& ex)
    {
      std::cerr << "ERROR: caught [" << ex.what() << "]\n";
      ok = false;
    }

  ACE_DEBUG((LM_DEBUG, "...done\n"));
  return ok ? 0 : 1;
}
