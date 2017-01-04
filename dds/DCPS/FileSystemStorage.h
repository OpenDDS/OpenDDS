/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_FILESYSTEMSTORAGE_H
#define OPENDDS_FILESYSTEMSTORAGE_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/RcObject_T.h"

#include "ace/Synch_Traits.h"
#include "ace/SString.h"
#include "ace/os_include/os_dirent.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Dirent;
ACE_END_VERSIONED_NAMESPACE_DECL

#include <iosfwd>
#include "dds/DCPS/PoolAllocator.h"

#include <iterator>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

// This can be overriden by the user, but make sure that it's less than
// the actual operating system and filesystem limit so that the "overflow"
// directories can be created as needed.
#ifndef OPENDDS_FILESYSTEMSTORAGE_MAX_FILES_PER_DIR
#define OPENDDS_FILESYSTEMSTORAGE_MAX_FILES_PER_DIR 512
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace FileSystemStorage {

/// See $DDS_ROOT/docs/design/persistence.txt
/// General usage notes for FileSystemStorage:
///  - Start by calling Directory::create() to get a handle on a new or
///    existing directory.  All of its files and subdirectories must be
///    managed by this library.  The file and directory names will be encoded
///    so the application doesn't need to be aware of filesystem limitations
///    regarding special characters or length limits (for directory names).
///  - File names are limited to 150 characters, but directory names are
///    unlimited.
///  - The get_*() functions always create the file/dir if it doesn't exist.
///  - The application must take care not to create two objects representing
///    the same node in the filesystem (for example via get_file() and via
///    iteration) because their state will not be kept consistent if one is
///    modified.
///  - Locking is the responsibility of the application.
///  - These classes will throw std::runtime_error if there is a problem
///    with the underlying file system (out of space, bad permissions, etc.)
///    or other unexpected/irregular condition.

// For Win32 long path name support, used internally and not exported
#if defined ACE_WIN32 || defined ACE_USES_WCHAR

# ifdef ACE_WIN32
typedef ACE_DIRENT DDS_DIRENT;
# else // !ACE_WIN32
struct DDS_DIRENT {
  ACE_DIRENT* real_dirent_;
  ACE_TCHAR* d_name;
  DDS_DIRENT() : real_dirent_(), d_name() {}
};
# endif // ACE_WIN32

struct DDS_DIR;
class DDS_Dirent {
public:
  explicit DDS_Dirent(const ACE_TCHAR* path = 0);
  ~DDS_Dirent();
  int open(const ACE_TCHAR* path);
  void close();
  DDS_DIRENT* read();

private:
  DDS_DIR* dirp_;
};

#else // non-Win32 non-uses-wchar
#  define DDS_DIRENT ACE_DIRENT
#  define DDS_Dirent ACE_Dirent
#endif // ACE_WIN32

using OpenDDS::DCPS::RcObject;
using OpenDDS::DCPS::RcHandle;

class File;

class OpenDDS_Dcps_Export Directory : public RcObject<ACE_SYNCH_MUTEX> {
public:
  typedef RcHandle<Directory> Ptr;

  /// If root_path is relative it is up to the application to make sure the
  /// current directory is not changed while this object or any of its
  /// "child" objects are still alive.
  static Ptr create(const char* root_path);

private:
  ACE_TString full_path(const ACE_TString& relative) const;
  typedef OPENDDS_MAP(ACE_TString, ACE_TString) Map;

  template <typename Item>
  class Iterator
        : public std::iterator<std::input_iterator_tag, typename Item::Ptr> {
  public:
    typename Item::Ptr operator*() const {
      return deref();
    }

    typename Item::Ptr operator->() const {
      return deref();
    }

    Iterator& operator++() {
      ++delegate_;
      item_.reset();
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp(*this);
      ++delegate_;
      item_.reset();
      return tmp;
    }

    bool operator==(const Iterator& rhs) const {
      return delegate_ == rhs.delegate_;
    }

    bool operator!=(const Iterator& rhs) const {
      return delegate_ != rhs.delegate_;
    }

  private:
    friend class Directory;
    typedef Map::iterator IterDelegate;
    Iterator(const IterDelegate& del, const Directory::Ptr& outer)
        : delegate_(del)
        , outer_(outer)
        , item_() {}

    typename Item::Ptr deref() const {
      if (item_.is_nil()) {
        item_ = OpenDDS::DCPS::make_rch<Item>(outer_->full_path(delegate_->second),
                              delegate_->first, outer_);
      }

      return item_;
    }

    IterDelegate delegate_;
    Directory::Ptr outer_;
    mutable typename Item::Ptr item_;
  };

public:
  typedef Iterator<File> FileIterator;
  typedef Iterator<Directory> DirectoryIterator;

  FileIterator begin_files(); // files will be sorted
  FileIterator end_files();

  RcHandle<File> get_file(const char* name);  // slash is not a separator

  /// assumes all files in this dir are created with this API
  RcHandle<File> create_next_file();

  DirectoryIterator begin_dirs(); // dirs will be sorted
  DirectoryIterator end_dirs();

  Directory::Ptr get_dir(const OPENDDS_VECTOR(OPENDDS_STRING)& path);
  Directory::Ptr get_subdir(const char* name);  // slash is not a separator

  /// assumes all subdirectories in this dir are created with this API
  Directory::Ptr create_next_dir();

  void remove(); // recursive

  OPENDDS_STRING name() const;
  Directory::Ptr parent() const {
    return parent_;
  }

private:
  friend class File;
  template <typename T, typename U0, typename U1, typename U2>
  friend RcHandle<T> OpenDDS::DCPS::make_rch(const U0&, const U1&, const U2&);

  Directory(const ACE_TString& root_path, const ACE_TString& logical,
            const Directory::Ptr& parent);
  void scan_dir(const ACE_TString& relative, DDS_Dirent& dir,
                unsigned int overflow_index);
  RcHandle<File> make_new_file(const ACE_TString& t_name);
  void removing(const ACE_TString& logical_child, bool file);
  Directory::Ptr make_new_subdir(const ACE_TString& logical);
  ACE_TString add_entry(); // returns overflow directory prefix

  Directory::Ptr parent_;
  ACE_TString physical_dirname_, logical_dirname_;

  // overflow bucket (0==immediate) -> #entries
  OPENDDS_MAP(unsigned int, unsigned int) overflow_;

  Map files_, dirs_; // logical -> physical
  OPENDDS_MAP(ACE_TString, unsigned int) long_names_;
  // phys. prefix (before '.') -> next available counter #
};

class OpenDDS_Dcps_Export File : public RcObject<ACE_SYNCH_MUTEX> {
public:
  typedef RcHandle<File> Ptr;

  bool write(std::ofstream& stream);
  bool read(std::ifstream& stream);
  bool remove();
  OPENDDS_STRING name() const;
  Directory::Ptr parent() const {
    return parent_;
  }

private:
  friend class Directory;
  template <typename T, typename U0, typename U1, typename U2>
  friend OpenDDS::DCPS::RcHandle<T> OpenDDS::DCPS::make_rch(const U0&, const U1&, const U2&);
  template <typename Item> friend class Directory::Iterator;
  File(const ACE_TString& fname_phys, const ACE_TString& logical,
       const Directory::Ptr& parent);

  ACE_TString physical_file_, physical_dir_, logical_relative_;
  Directory::Ptr parent_;
};

// Encode/decode using Base32Hex as described by RFC4648

OpenDDS_Dcps_Export
ACE_TString b32h_encode(const ACE_TCHAR* decoded);

OpenDDS_Dcps_Export
ACE_TString b32h_decode(const ACE_TCHAR* encoded);

} // namespace FileSystemStorage
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
#endif
