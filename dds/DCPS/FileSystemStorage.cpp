/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_SAFETY_PROFILE

#include "FileSystemStorage.h"

#include "DirentWrapper.h"

#include <ace/Vector_T.h>
#include <ace/OS_NS_sys_stat.h>
#include <ace/OS_NS_macros.h>
#include <ace/OS_NS_unistd.h>
#include <ace/OS_NS_stdio.h>

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <fstream>

typedef size_t String_Index_t;

namespace {

const size_t FSS_MAX_FILE_NAME = 150, FSS_MAX_FILE_NAME_ENCODED = 240,
  FSS_MAX_OVERFLOW_DIR = 9999;
const ACE_TCHAR FSS_DEFAULT_FILE_NAME[] = ACE_TEXT("F00000000000000");
const ACE_TCHAR FSS_DEFAULT_DIR_NAME[]  = ACE_TEXT("D00000000000000");

void add_slash(ACE_TString& str)
{
  if (str.length() == 0) return;

  if (str[str.length() - 1] == ACE_TEXT('\\')) {
    str[str.length() - 1] = ACE_TEXT('/');

  } else if (str[str.length() - 1] != ACE_TEXT('/')) {
    str += ACE_TEXT('/');
  }
}

// increment a filename in a lexicographical ordering
bool increment(ACE_TString& logical)
{
  for (size_t i = logical.length() - 1; i >= 1; --i) {
    //we could use the entire range of 255 values but we'll keep
    //these names to 7-bit ASCII
    if (logical[i] < 0x7F) {
      ++logical[i];
      return true;
    }
  }

  return false;
}

// support long paths (> 260 chars in absolute path) on Win32
/* Windows Mobile / Windows CE (WinCE) porting notes:
 *   These platforms have no concept of "current directory"
 *   and the Win32-specific \\?\ syntax may or may not work.
 *   This file has been set up to compile under WinCE but we
 *   don't expect that persistent storage to work well at runtime
 *   so PERSISTENT_DURABILITY_QOS may not actually work.
 */
#if defined ACE_WIN32 && !defined ACE_HAS_WINCE

void fwd_slash_to_back_slash(ACE_WString& str)
{
  for (String_Index_t idx = str.find(L'/'); idx != ACE_WString::npos;
       idx = str.find(L'/', idx + 1)) {
    str[idx] = L'\\';
  }
}

bool is_relative(const ACE_TCHAR* path)
{
  return !path[0] ||
         (path[1] != ACE_TEXT(':') &&
          (ACE_OS::strncmp(ACE_TEXT("\\\\?\\"), path, 4) != 0));
}

/// @returns \\?\<absolute_path>
ACE_WString to_win32_long_path(const ACE_TCHAR* path)
{
  ACE_WString wpath = ACE_TEXT_ALWAYS_WCHAR(path);
  fwd_slash_to_back_slash(wpath);
  ACE_WString dir;

  if (is_relative(path)) {
    DWORD sz = ::GetCurrentDirectoryW(0, 0);
    ACE_Vector<wchar_t> cur(sz);
    cur.resize(sz, L'\0');
    ::GetCurrentDirectoryW(sz, &cur[0]);
    dir = &cur[0];

    if (dir[dir.length() - 1] != L'\\') dir += L'\\';
  }

  if (dir.substr(0, 4) != L"\\\\?\\" && wpath.substr(0, 4) != L"\\\\?\\")
    dir = L"\\\\?\\" + dir;

  dir += wpath;
  return dir;
}

int dds_mkdir(const ACE_TCHAR* path)
{
  ACE_WString wpath = to_win32_long_path(path);
  ACE_WIN32CALL_RETURN(ACE_ADAPT_RETVAL(::CreateDirectoryW(wpath.c_str(), 0),
                                        ace_result_), int, -1);
}

int dds_chdir(const ACE_TCHAR* path)
{
  // In all the other cases we are going to \\?\<absolute> long paths, but
  // SetCurrentDirectory() doesn't allow this workaround.
  ACE_WString wpath = to_win32_long_path(path);
  wchar_t spath[MAX_PATH];

  if (0 == ::GetShortPathNameW(wpath.c_str(), spath, MAX_PATH)) {
    throw std::runtime_error("GetShortPathNameW failed.");
  }

  return ::_wchdir(spath);
}

bool is_dir(const ACE_TCHAR* path)
{
  ACE_WString wpath = to_win32_long_path(path);
  DWORD attrib = ::GetFileAttributesW(wpath.c_str());

  if (attrib != INVALID_FILE_ATTRIBUTES) {
    return attrib & FILE_ATTRIBUTE_DIRECTORY;
  }

  return false;
}

int dds_rmdir(const ACE_TCHAR* path)
{
  ACE_WString wpath = to_win32_long_path(path);
  ACE_WIN32CALL_RETURN(ACE_ADAPT_RETVAL(::RemoveDirectoryW(wpath.c_str()),
                                        ace_result_), int, -1);
}

#else // !ACE_WIN32

inline int dds_mkdir(const ACE_TCHAR* path)
{
  return ACE_OS::mkdir(path);
}

inline int dds_chdir(const ACE_TCHAR* path)
{
  return ACE_OS::chdir(path);
}

inline bool is_dir(const ACE_TCHAR* path)
{
  ACE_stat st;
  return ACE_OS::stat(path, &st) != -1 && (st.st_mode & S_IFDIR);
}

inline int dds_rmdir(const ACE_TCHAR* path)
{
  return ACE_OS::rmdir(path);
}

#endif // ACE_WIN32

ACE_TString overflow_dir_name(unsigned int n)
{
  const size_t buf_size=32;
  ACE_TCHAR of_name[buf_size];
  ACE_OS::snprintf(of_name, buf_size, ACE_TEXT("_overflow.%04u/"), n);
  return of_name;
}

struct CwdGuard {
  ACE_TString cwd_;

  explicit CwdGuard(const ACE_TString& dir) {
    ACE_Vector<ACE_TCHAR> cwd_buf(128);
    cwd_buf.resize(128, ACE_TCHAR(0));

    while (ACE_OS::getcwd(&cwd_buf[0], cwd_buf.size()) == 0) {
      if (errno == ERANGE) {
        cwd_buf.resize(cwd_buf.size() * 2, ACE_TCHAR(0));

      } else break;
    }

    if (cwd_buf[0]) cwd_ = &cwd_buf[0];

    dds_chdir(dir.c_str());
  }

  ~CwdGuard() {
    dds_chdir(cwd_.c_str());
  }
};

void recursive_remove(const ACE_TString& dirname)
{
  using namespace OpenDDS::FileSystemStorage;
  DDS_Dirent dir(dirname.c_str());
  {
    CwdGuard cg(dirname);

    for (DDS_DIRENT* ent = dir.read(); ent; ent = dir.read()) {
      if (ent->d_name[0] == ACE_TEXT('.') && (!ent->d_name[1] ||
          (ent->d_name[1] == ACE_TEXT('.') && !ent->d_name[2]))) {
        continue; // skip '.' and '..'
      }

      if (is_dir(ent->d_name)) {
        recursive_remove(ent->d_name);

      } else { // regular file
        ACE_OS::unlink(ent->d_name);
      }
    }
  }
  dds_rmdir(dirname.c_str());
}

} // namespace

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace FileSystemStorage {

#ifdef ACE_WIN32

// DDS_DIR

struct DDS_DIR {
  /// The name of the directory we are looking into
  wchar_t* directory_name_;

  /// Remember the handle between calls.
  HANDLE current_handle_;

  /// The struct for the results
  ACE_DIRENT* dirent_;

  /// The struct for intermediate results.
  WIN32_FIND_DATAW fdata_;

  /// A flag to remember if we started reading already.
  int started_reading_;
};

// DDS_Dirent

DDS_Dirent::DDS_Dirent(const ACE_TCHAR* path)
  : dirp_(0)
{
  if (path) open(path);
}

int DDS_Dirent::open(const ACE_TCHAR* path)
{
  close();
  ACE_WString wpath =
#ifdef ACE_HAS_WINCE
    path;
#else
    to_win32_long_path(path);
#endif

  // taken from ACE_OS::opendir_emulation() and translated to wchar

  wchar_t extra[3] = {0, 0, 0};
  const wchar_t* filename = wpath.c_str();

  // Check if filename is a directory.
  DWORD fileAttribute = ::GetFileAttributesW(filename);

  if (fileAttribute == INVALID_FILE_ATTRIBUTES) {
    ACE_OS::set_errno_to_last_error();
    return -1;
  }

  if (!(fileAttribute & FILE_ATTRIBUTE_DIRECTORY)) {
    errno = ENOTDIR;
    return -1;
  }

  size_t const lastchar = ACE_OS::strlen(filename);

  if (lastchar > 0) {
    if (filename[lastchar-1] != L'*') {
      if (filename[lastchar-1] != L'/' && filename[lastchar-1] != L'\\')
        ACE_OS::strcpy(extra, L"\\*");

      else
        ACE_OS::strcpy(extra, L"*");
    }
  }

  ACE_NEW_RETURN(dirp_, DDS_DIR, -1);
  ACE_NEW_RETURN(dirp_->directory_name_,
                 wchar_t[lastchar + ACE_OS::strlen(extra) + 1],
                 -1);
  ACE_OS::strcpy(dirp_->directory_name_, filename);

  if (extra[0])
    ACE_OS::strcat(dirp_->directory_name_, extra);

  dirp_->current_handle_ = INVALID_HANDLE_VALUE;
  dirp_->started_reading_ = 0;
  dirp_->dirent_ = 0;
  return 0;
}

DDS_DIRENT* DDS_Dirent::read()
{
  if (!dirp_) return 0;

  // taken from ACE_OS::readdir_emulation() and translated to wchar
  //   note that the file name inside ACE_DIRENT is still in ACE_TCHARs as long
  //   as ACE_HAS_TCHAR_DIRENT is defined (true for MSVC, false for GCC/MinGW)

  if (dirp_->dirent_ != 0) {
#ifdef ACE_HAS_TCHAR_DIRENT
    ACE_OS::free(dirp_->dirent_->d_name);
#endif
    ACE_OS::free(dirp_->dirent_);
    dirp_->dirent_ = 0;
  }

  if (!dirp_->started_reading_) {
    dirp_->current_handle_ = ::FindFirstFileW(dirp_->directory_name_,
                                              &dirp_->fdata_);
    dirp_->started_reading_ = 1;

  } else {
    int retval = ::FindNextFileW(dirp_->current_handle_, &dirp_->fdata_);

    if (retval == 0) {
      // Make sure to close the handle explicitly to avoid a leak!
      ::FindClose(dirp_->current_handle_);
      dirp_->current_handle_ = INVALID_HANDLE_VALUE;
    }
  }

  if (dirp_->current_handle_ != INVALID_HANDLE_VALUE) {
    dirp_->dirent_ = (DDS_DIRENT*) ACE_Allocator::instance()->malloc(sizeof(DDS_DIRENT));

    if (dirp_->dirent_ != 0) {
#ifdef ACE_HAS_TCHAR_DIRENT
      dirp_->dirent_->d_name =
        (ACE_TCHAR*) ACE_Allocator::instance()->malloc((ACE_OS::strlen(dirp_->fdata_.cFileName)
                                     + 1) * sizeof(ACE_TCHAR));
      ACE_OS::strcpy(dirp_->dirent_->d_name,
                     ACE_TEXT_WCHAR_TO_TCHAR(dirp_->fdata_.cFileName));
#else // MinGW: d_name is a fixed-size char array
      ACE_OS::strncpy(dirp_->dirent_->d_name,
                      ACE_Wide_To_Ascii(dirp_->fdata_.cFileName).char_rep(),
                      sizeof(dirp_->dirent_->d_name));
#endif
      dirp_->dirent_->d_reclen = sizeof(DDS_DIRENT);
    }

    return dirp_->dirent_;

  } else
    return 0;
}

void DDS_Dirent::close()
{
  if (dirp_) {
    if (dirp_->current_handle_ != INVALID_HANDLE_VALUE)
      ::FindClose(dirp_->current_handle_);

    dirp_->current_handle_ = INVALID_HANDLE_VALUE;
    dirp_->started_reading_ = 0;

    if (dirp_->dirent_ != 0) {
#ifdef ACE_HAS_TCHAR_DIRENT
      ACE_OS::free(dirp_->dirent_->d_name);
#endif
      ACE_OS::free(dirp_->dirent_);
    }

    dirp_ = 0;
  }
}

DDS_Dirent::~DDS_Dirent()
{
  close();
}

#elif defined ACE_USES_WCHAR // non-Win32 uses-WChar

struct DDS_DIR {
  ACE_DIR* real_dir_;
  DDS_DIRENT ent_;

  DDS_DIR() : real_dir_(), ent_() {}
};

DDS_Dirent::DDS_Dirent(const ACE_TCHAR* path)
  : dirp_(new DDS_DIR)
{
  if (path) open(path);
}

int DDS_Dirent::open(const ACE_TCHAR* path)
{
  close();
  return (dirp_->real_dir_ = ACE_OS::opendir(path)) == 0 ? -1 : 0;
}

DDS_DIRENT* DDS_Dirent::read()
{
  if (!dirp_->real_dir_) return 0;

  dirp_->ent_.real_dirent_ = ACE_OS::readdir(dirp_->real_dir_);

  if (!dirp_->ent_.real_dirent_) return 0;

  ACE_OS::free(dirp_->ent_.d_name);
  dirp_->ent_.d_name =
    ACE_OS::strdup(ACE_TEXT_CHAR_TO_TCHAR(dirp_->ent_.real_dirent_->d_name));
  return &dirp_->ent_;
}

void DDS_Dirent::close()
{
  if (dirp_->real_dir_) {
    ACE_OS::closedir(dirp_->real_dir_);
    ACE_OS::free(dirp_->ent_.d_name);
  }

  dirp_->real_dir_ = 0;
}

DDS_Dirent::~DDS_Dirent()
{
  close();
  delete dirp_;
}

#endif // ACE_WIN32

// File

File::File(const ACE_TString& fname_phys, const ACE_TString& logical,
           const Directory::Ptr& parent)
  : physical_file_()
  , physical_dir_()
  , logical_relative_(logical)
  , parent_(parent)
{
  String_Index_t last_slash = fname_phys.rfind(ACE_TEXT('/'));

  if (last_slash == ACE_TString::npos) {
    physical_file_ = fname_phys;
    physical_dir_ = ACE_TEXT(".");

  } else {
    physical_file_ = fname_phys.c_str() + last_slash + 1;
    physical_dir_.set(fname_phys.c_str(), last_slash, true);
  }
}

bool File::write(std::ofstream& stream)
{
  CwdGuard cg(physical_dir_);
  stream.open(ACE_TEXT_ALWAYS_CHAR(physical_file_.c_str()),
              ios::binary | ios::out);
  return !stream.bad() && !stream.fail();
}

bool File::read(std::ifstream& stream)
{
  CwdGuard cg(physical_dir_);
  stream.open(ACE_TEXT_ALWAYS_CHAR(physical_file_.c_str()),
              ios::binary | ios::in);
  return !stream.bad() && !stream.fail();
}

bool File::remove()
{
  int unlink_result = -1;
  {
    CwdGuard cg(physical_dir_);
    unlink_result = ACE_OS::unlink(physical_file_.c_str());
  }

  if (unlink_result != -1) {
    parent_->removing(logical_relative_, true);
    return true;
  }

  return false;
}

OPENDDS_STRING File::name() const
{
  return ACE_TEXT_ALWAYS_CHAR(logical_relative_.c_str());
}

// Directory

/*static*/ Directory::Ptr Directory::create(const char* dirname)
{
  return DCPS::make_rch<Directory>(ACE_TEXT_CHAR_TO_TCHAR(dirname), ACE_TEXT(""), Directory::Ptr ());
}

ACE_TString Directory::full_path(const ACE_TString& relative) const
{
  return physical_dirname_ + relative;
}

Directory::FileIterator Directory::begin_files()
{
  return FileIterator(files_.begin(), rchandle_from(this));
}

Directory::FileIterator Directory::end_files()
{
  return FileIterator(files_.end(),  rchandle_from(this));
}

File::Ptr Directory::get_file(const char* name)
{
  if (std::strlen(name) >= FSS_MAX_FILE_NAME) {
    throw std::runtime_error("file name too long");
  }

  ACE_TString t_name(ACE_TEXT_CHAR_TO_TCHAR(name));
  Map::iterator it = files_.find(t_name);

  if (it == files_.end()) {
    return make_new_file(t_name);

  } else {
    return DCPS::make_rch<File>(full_path(it->second), it->first, rchandle_from(this));
  }
}

File::Ptr Directory::make_new_file(const ACE_TString& t_name)
{
  if (dirs_.find(t_name) != dirs_.end()) {
    throw std::runtime_error("Can't create a file with the same name as "
                             "an existing directory.");
  }

  ACE_TString phys = add_entry() + b32h_encode(t_name.c_str());
  files_[t_name] = phys;

  CwdGuard cg(physical_dirname_);
  // touch the file since the user has asked to create it
  std::FILE* fh = std::fopen(ACE_TEXT_ALWAYS_CHAR(phys.c_str()), "w");

  if (!fh) throw std::runtime_error("Can't create the file");

  std::fclose(fh);
  return DCPS::make_rch<File>(physical_dirname_ + phys, t_name,  rchandle_from(this));
}

File::Ptr Directory::create_next_file()
{
  ACE_TString logical;

  if (files_.empty()) {
    logical = FSS_DEFAULT_FILE_NAME;

  } else {
    Map::iterator last = --files_.end();
    logical = last->first;

    if (!increment(logical)) {
      throw std::runtime_error("out of range for create_next_file");
    }
  }

  return make_new_file(logical);
}

Directory::DirectoryIterator Directory::begin_dirs()
{
  return DirectoryIterator(dirs_.begin(),  rchandle_from(this));
}

Directory::DirectoryIterator Directory::end_dirs()
{
  return DirectoryIterator(dirs_.end(),  rchandle_from(this));
}

Directory::Ptr Directory::get_dir(const OPENDDS_VECTOR(OPENDDS_STRING)& path)
{
  Directory::Ptr dir = rchandle_from(this);
  typedef OPENDDS_VECTOR(OPENDDS_STRING)::const_iterator iterator;

  for (iterator iter = path.begin(), end = path.end(); iter != end; ++iter) {
    dir = dir->get_subdir(iter->c_str());
  }

  return dir;
}

Directory::Ptr Directory::get_subdir(const char* name)
{
  ACE_TString t_name = ACE_TEXT_CHAR_TO_TCHAR(name);
  Map::iterator it = dirs_.find(t_name);

  if (it == dirs_.end()) {
    return make_new_subdir(t_name);

  } else {
    return DCPS::make_rch<Directory>(full_path(it->second), it->first,  rchandle_from(this));
  }
}

Directory::Ptr Directory::create_next_dir()
{
  ACE_TString logical;

  if (dirs_.empty()) {
    logical = FSS_DEFAULT_DIR_NAME;

  } else {
    Map::iterator last = --dirs_.end();
    logical = last->first;

    if (!increment(logical)) {
      throw std::runtime_error("out of range for create_next_dir");
    }
  }

  return make_new_subdir(logical);
}

Directory::Ptr Directory::make_new_subdir(const ACE_TString& t_name)
{
  if (files_.find(t_name) != files_.end()) {
    throw std::runtime_error("Can't create a directory with the same "
                             "name as an existing file.");
  }

  ACE_TString logical(t_name.c_str(),
                      (std::min)(FSS_MAX_FILE_NAME, t_name.length()));
  ACE_TString phys_prefix = add_entry();
  ACE_TString phys_base = b32h_encode(logical.c_str());

  if (t_name.length() >= FSS_MAX_FILE_NAME) {
    unsigned int& counter = long_names_[phys_prefix + phys_base];

    if (counter == 99999) {
      throw std::runtime_error("Long directory name out of range");
    }

    phys_base += ACE_TEXT(".     X"); // snprintf will clobber the X with a 0
    ACE_TCHAR* buf = &phys_base[0] + phys_base.length() - 6;
    ACE_OS::snprintf(buf, 6, ACE_TEXT("%05u"), counter++);
    phys_base = phys_base.substr(0, phys_base.length() - 1); // trim the 0
  }

  ACE_TString phys = phys_prefix + phys_base;
  dirs_[t_name] = phys;
  {
    CwdGuard cg(physical_dirname_);

    if (dds_mkdir(phys.c_str()) == -1) {
      throw std::runtime_error("Can't create directory");
    }

    if ((phys_prefix.length() > 0 && dds_chdir(phys_prefix.c_str()) == -1)
        || dds_chdir(phys_base.c_str()) == -1) {
      dds_rmdir(phys.c_str());
      throw std::runtime_error("Can't change to newly created directory");
    }

    std::ofstream fn("_fullname");
    fn << t_name << '\n';
  }
  return DCPS::make_rch<Directory>(physical_dirname_ + phys, t_name,  rchandle_from(this));
}

ACE_TString Directory::add_entry()
{
  if (overflow_.empty()) {
    overflow_[0] = 1;
    return ACE_TEXT("");
  }

  typedef OPENDDS_MAP(unsigned int, unsigned int)::iterator iterator;
  // find existing overflow bucket with capacity
  bool found_gap(false);
  unsigned int last_seen(0), unused_bucket(0);

  for (iterator iter = overflow_.begin(), end = overflow_.end();
       iter != end; ++iter) {
    if (iter->second < OPENDDS_FILESYSTEMSTORAGE_MAX_FILES_PER_DIR) {
      ++iter->second;

      if (iter->first == 0) return ACE_TEXT("");

      return overflow_dir_name(iter->first);
    }

    if (!found_gap && iter->first > last_seen + 1) {
      found_gap = true;
      unused_bucket = last_seen + 1;
    }

    last_seen = iter->first;
  }

  if (!found_gap) {
    if (last_seen == FSS_MAX_OVERFLOW_DIR) {
      throw std::runtime_error("Overflow serial # out of range.");
    }

    unused_bucket = last_seen + 1;
  }

  overflow_[unused_bucket] = 1;
  ACE_TString dir_name = overflow_dir_name(unused_bucket);
  CwdGuard cg(physical_dirname_);

  if (dds_mkdir(dir_name.c_str()) == -1) {
    throw std::runtime_error("Can't create overflow directory");
  }

  return dir_name;
}

void Directory::remove()
{
  if (!parent_.is_nil()) parent_->removing(logical_dirname_, false);

  parent_.reset();
  recursive_remove(physical_dirname_);
  overflow_.clear();
  files_.clear();
  dirs_.clear();
  long_names_.clear();
}

OPENDDS_STRING Directory::name() const
{
  return ACE_TEXT_ALWAYS_CHAR(logical_dirname_.c_str());
}

Directory::Directory(const ACE_TString& dirname, const ACE_TString& logical,
                     const Directory::Ptr& parent)
  : parent_(parent)
  , physical_dirname_(dirname)
  , logical_dirname_(logical)
{
  add_slash(physical_dirname_);

  bool ok(true);
  DDS_Dirent dir;

  if (dir.open(physical_dirname_.c_str()) == -1) {
    ok = false;

    if (errno == ENOENT && dds_mkdir(physical_dirname_.c_str()) != -1
        && dir.open(physical_dirname_.c_str()) != -1) {
      ok = true;
    }
  }

  if (!ok) throw std::runtime_error("Can't open or create directory");

  scan_dir(ACE_TEXT(""), dir, 0);
}

void Directory::scan_dir(const ACE_TString& relative, DDS_Dirent& dir,
                         unsigned int overflow_index)
{
  ACE_TString path = physical_dirname_ + relative;
  add_slash(path);

  while (DDS_DIRENT* ent = dir.read()) {
    if (ent->d_name[0] == ACE_TEXT('.') && (!ent->d_name[1] ||
        (ent->d_name[1] == ACE_TEXT('.') && !ent->d_name[2]))) {
      continue; // skip '.' and '..'
    }

    ACE_TString file = path + ent->d_name;

    if (is_dir(file.c_str())) {
      ACE_TString phys(relative);
      add_slash(phys);
      phys += ent->d_name;

      if (ACE_OS::strncmp(ent->d_name, ACE_TEXT("_overflow."), 10) == 0) {
        unsigned int n = ACE_OS::atoi(ent->d_name + 10);
        DDS_Dirent overflow(file.c_str());
        scan_dir(ent->d_name, overflow, n);

      } else if (ACE_OS::strlen(ent->d_name) <= FSS_MAX_FILE_NAME_ENCODED) {
        dirs_[b32h_decode(ent->d_name)] = phys;
        ++overflow_[overflow_index];

      } else {
        CwdGuard cg(file);
        std::ifstream fn("_fullname");
        OPENDDS_STRING fullname;

        if (!std::getline(fn, fullname)) {
          throw std::runtime_error("Can't read .../_fullname");
        }

        ACE_TString full_t(ACE_TEXT_CHAR_TO_TCHAR(fullname.c_str()));
        dirs_[full_t] = phys;
        ++overflow_[overflow_index];

        String_Index_t idx = phys.rfind(ACE_TEXT('.'));

        if (idx == ACE_TString::npos) {
          throw std::runtime_error("Badly formatted long dir name");
        }

        ACE_TString prefix(phys.c_str(), idx);
        unsigned int serial = ACE_OS::atoi(&phys[idx + 1]);
        unsigned int& counter = long_names_[prefix];

        if (serial >= counter) counter = serial + 1;
      }

    } else { // regular file
      if (ent->d_name[0] != ACE_TEXT('_')) {
        files_[b32h_decode(ent->d_name)] = ent->d_name;
        ++overflow_[overflow_index];
      }
    }
  }
}

void Directory::removing(const ACE_TString& child, bool file)
{
  Map& m = file ? files_ : dirs_;
  Map::iterator iter = m.find(child);

  if (iter == m.end()) return;

  const ACE_TString& phys = iter->second;
  String_Index_t idx = phys.find(ACE_TEXT("_overflow."));
  unsigned int bucket = (idx == 0 ? ACE_OS::atoi(&phys[idx + 10]) : 0);

  if (--overflow_[bucket] == 0 && bucket > 0) {
    overflow_.erase(bucket);
    idx = phys.find(ACE_TEXT('/'));
    ACE_TString ov_dir = physical_dirname_ + ACE_TString(phys.c_str(), idx);
    dds_rmdir(ov_dir.c_str());
  }

  m.erase(iter);
}

// Base32Hex

ACE_TString b32h_encode(const ACE_TCHAR* decoded)
{
  static const ACE_TCHAR lookup[] =
    ACE_TEXT("0123456789ABCDEFGHIJKLMNOPQRSTUV");
  static const ACE_TCHAR padding[] = ACE_TEXT("======");
  static const size_t enc[] = {0, 2, 4, 5, 7}; // #input -> #non-padded output
  ACE_TString encoded;

  for (size_t len = ACE_OS::strlen(decoded); *decoded; decoded += 5, len -= 5) {
    ACE_UINT64 chunk = 0;

    for (size_t i(0); i < 5 && i < len; ++i) {
      chunk |= static_cast<ACE_UINT64>(decoded[i] & 0xFF) << ((4 - i) * 8);
    }

    size_t limit = (len < 5) ? enc[len] : 8;

    for (size_t i(0); i < limit; ++i) {
      unsigned char val =
        static_cast<unsigned char>(chunk >>((7 - i) * 5)) & 0x1F;
      encoded += lookup[val];
    }

    if (len < 5) {
      encoded.append(padding, 8 - enc[len]);
      return encoded;
    }
  }

  return encoded;
}

ACE_TString b32h_decode(const ACE_TCHAR* encoded)
{
  // #before first '=' -> #output
  static const size_t dec[] = {0, 0, 1, 0, 2, 3, 0, 4, 0};
  ACE_TString decoded;

  for (; *encoded; encoded += 8) {
    ACE_UINT64 chunk = 0;
    size_t i = 0;

    for (; i < 8 && encoded[i] != ACE_TEXT('='); ++i) {
      char idx = (encoded[i] <= ACE_TEXT('9'))
                 ? (encoded[i] - ACE_TEXT('0'))
                 : (10 + encoded[i] - ACE_TEXT('A'));
      chunk |= static_cast<ACE_UINT64>(idx) << ((7 - i) * 5);
    }

    size_t limit = (encoded[i] == ACE_TEXT('=')) ? dec[i] : 5;

    for (size_t j(0); j < limit; ++j) {
      decoded += static_cast<ACE_TCHAR>(chunk >>((4 - j) * 8)) & 0xFF;
    }
  }

  return decoded;
}

} // namespace FileSystemStorage
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
