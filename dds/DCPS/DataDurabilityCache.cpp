/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_PERSISTENCE_PROFILE

#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTypeSupportExtC.h"
#include "DataDurabilityCache.h"
#include "SendStateDataSampleList.h"
#include "DataSampleElement.h"
#include "WriteDataContainer.h"
#include "DataWriterImpl.h"
#include "Qos_Helper.h"
#include "debug.h"
#include "SafetyProfileStreams.h"
#include "Service_Participant.h"
#include "RcEventHandler.h"

#include "ace/Reactor.h"
#include "ace/Message_Block.h"
#include "ace/Log_Msg.h"
#include "ace/Malloc_T.h"
#include "ace/MMAP_Memory_Pool.h"
#include "ace/OS_NS_sys_time.h"

#include <fstream>
#include <algorithm>

namespace {

void cleanup_directory(const OPENDDS_VECTOR(OPENDDS_STRING) & path,
                       const ACE_CString & data_dir)
{
  if (path.empty()) return;

  using OpenDDS::FileSystemStorage::Directory;
  Directory::Ptr dir = Directory::create(data_dir.c_str());
  dir = dir->get_dir(path);
  Directory::Ptr parent = dir->parent();
  dir->remove();

  // clean up empty directories
  while (!parent.is_nil() &&
         (parent->begin_dirs() == parent->end_dirs())) {
    Directory::Ptr to_delete = parent;
    parent = parent->parent();
    to_delete->remove();
  }
}

/**
 * @class Cleanup_Handler
 *
 * @brief Event handler that is called when @c service_cleanup_delay
 *        period expires.
 */
class Cleanup_Handler : public OpenDDS::DCPS::RcEventHandler {
public:

  typedef
  OpenDDS::DCPS::DataDurabilityCache::sample_data_type data_type;
  typedef
  OpenDDS::DCPS::DataDurabilityCache::sample_list_type list_type;
  typedef ptrdiff_t list_difference_type;

  Cleanup_Handler(list_type & sample_list,
                  list_difference_type index,
                  ACE_Allocator * allocator,
                  const OPENDDS_VECTOR(OPENDDS_STRING) & path,
                  const ACE_CString & data_dir)
  : sample_list_(sample_list)
  , index_(index)
  , allocator_(allocator)
  , tid_(-1)
  , timer_ids_(0)
  , path_(path)
  , data_dir_(data_dir)
  {
  }

  virtual int handle_timeout(ACE_Time_Value const & /* current_time */,
                             void const * /* act */) {
    if (OpenDDS::DCPS::DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) OpenDDS - Cleaning up ")
                 ACE_TEXT("data durability cache.\n")));
    }

    typedef OpenDDS::DCPS::DurabilityQueue<
    OpenDDS::DCPS::DataDurabilityCache::sample_data_type>
    data_queue_type;

    // Cleanup all data samples corresponding to the cleanup delay.
    data_queue_type *& queue = this->sample_list_[this->index_];
    ACE_DES_FREE(queue,
                 this->allocator_->free,
                 data_queue_type);
    queue = 0;

    try {
      cleanup_directory(path_, this->data_dir_);

    } catch (const std::exception& ex) {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) Cleanup_Handler::handle_timout ")
                   ACE_TEXT("couldn't remove directory for PERSISTENT ")
                   ACE_TEXT("data: %C\n"), ex.what()));
      }
    }

    // No longer any need to keep track of the timer ID.
    this->timer_ids_->remove(this->tid_);

    return 0;
  }

  void timer_id(
    long tid,
    OpenDDS::DCPS::DataDurabilityCache::timer_id_list_type * timer_ids) {
    this->tid_ = tid;
    this->timer_ids_ = timer_ids;
  }

protected:

  virtual ~Cleanup_Handler() {}

private:

  /// List containing samples to be cleaned up when the cleanup timer
  /// expires.
  list_type & sample_list_;

  /// Location in list/array of queue to be deallocated.
  list_difference_type const index_;

  /// Allocator to be used when deallocating data queue.
  ACE_Allocator * const allocator_;

  /// Timer ID corresponding to this cleanup event handler.
  long tid_;

  /// List of timer IDs.
  /**
   * If the cleanup timer fires successfully, the timer ID must be
   * removed from the timer ID list so that a subsequent attempt to
   * cancel the timer during durability cache destruction does not
   * occur.
   */
  OpenDDS::DCPS::DataDurabilityCache::timer_id_list_type *
  timer_ids_;

  OPENDDS_VECTOR(OPENDDS_STRING) path_;

  ACE_CString data_dir_;
};

} // namespace

OpenDDS::DCPS::DataDurabilityCache::sample_data_type::sample_data_type()
  : length_(0)
  , sample_(0)
  , allocator_(0)
{
  this->source_timestamp_.sec = 0;
  this->source_timestamp_.nanosec = 0;
}

OpenDDS::DCPS::DataDurabilityCache::sample_data_type::sample_data_type(
  DataSampleElement & element,
  ACE_Allocator * a)
  : length_(0)
  , sample_(0)
  , allocator_(a)
{
  this->source_timestamp_.sec     = element.get_header().source_timestamp_sec_;
  this->source_timestamp_.nanosec = element.get_header().source_timestamp_nanosec_;

  // Only copy the data provided by the user.  The DataSampleHeader
  // will be reconstructed when the durable data is retrieved by a
  // DataWriterImpl instance.
  //
  // The user's data is stored in the first message block
  // continuation.
  ACE_Message_Block const * const data = element.get_sample()->cont();
  init(data);
}

OpenDDS::DCPS::DataDurabilityCache::sample_data_type::sample_data_type(
  DDS::Time_t timestamp, const ACE_Message_Block & mb, ACE_Allocator * a)
  : length_(0)
  , sample_(0)
  , source_timestamp_(timestamp)
  , allocator_(a)
{
  init(&mb);
}

void
OpenDDS::DCPS::DataDurabilityCache::sample_data_type::init
(const ACE_Message_Block * data)
{
  this->length_ = data->total_length();

  ACE_ALLOCATOR(this->sample_,
                static_cast<char *>(
                  this->allocator_->malloc(this->length_)));

  char * buf = this->sample_;

  for (ACE_Message_Block const * i = data;
       i != 0;
       i = i->cont()) {
    ACE_OS::memcpy(buf, i->rd_ptr(), i->length());
    buf += i->length();
  }
}

OpenDDS::DCPS::DataDurabilityCache::sample_data_type::sample_data_type(
  sample_data_type const & rhs)
  : length_(rhs.length_)
  , sample_(0)
  , allocator_(rhs.allocator_)
{
  this->source_timestamp_.sec     = rhs.source_timestamp_.sec;
  this->source_timestamp_.nanosec = rhs.source_timestamp_.nanosec;

  if (this->allocator_) {
    ACE_ALLOCATOR(this->sample_,
                  static_cast<char *>(
                    this->allocator_->malloc(rhs.length_)));
    ACE_OS::memcpy(this->sample_, rhs.sample_, rhs.length_);
  }
}

OpenDDS::DCPS::DataDurabilityCache::sample_data_type::~sample_data_type()
{
  if (this->allocator_)
    this->allocator_->free(this->sample_);
}

OpenDDS::DCPS::DataDurabilityCache::sample_data_type &
OpenDDS::DCPS::DataDurabilityCache::sample_data_type::operator= (
  sample_data_type const & rhs)
{
  // Strongly exception-safe copy assignment.
  sample_data_type tmp(rhs);
  std::swap(this->length_, tmp.length_);
  std::swap(this->sample_, tmp.sample_);
  std::swap(this->allocator_, tmp.allocator_);

  this->source_timestamp_.sec     = rhs.source_timestamp_.sec;
  this->source_timestamp_.nanosec = rhs.source_timestamp_.nanosec;

  return *this;
}

void
OpenDDS::DCPS::DataDurabilityCache::sample_data_type::get_sample(
  char const *& s,
  size_t & len,
  DDS::Time_t & source_timestamp)
{
  s = this->sample_;
  len = this->length_;
  source_timestamp.sec     = this->source_timestamp_.sec;
  source_timestamp.nanosec = this->source_timestamp_.nanosec;
}

void
OpenDDS::DCPS::DataDurabilityCache::sample_data_type::set_allocator(
  ACE_Allocator * allocator)
{
  this->allocator_ = allocator;
}

OpenDDS::DCPS::DataDurabilityCache::DataDurabilityCache(
  DDS::DurabilityQosPolicyKind kind)
  : allocator_(new ACE_New_Allocator)
  , kind_(kind)
  , samples_(0)
  , cleanup_timer_ids_()
  , lock_()
  , reactor_(0)
{
  init();
}

OpenDDS::DCPS::DataDurabilityCache::DataDurabilityCache(
  DDS::DurabilityQosPolicyKind kind,
  ACE_CString & data_dir)
  : allocator_(new ACE_New_Allocator)
  , kind_(kind)
  , data_dir_(data_dir)
  , samples_(0)
  , cleanup_timer_ids_()
  , lock_()
  , reactor_(0)
{
  init();
}

void OpenDDS::DCPS::DataDurabilityCache::init()
{
  ACE_Allocator * const allocator = this->allocator_.get();
  ACE_NEW_MALLOC(
    this->samples_,
    static_cast<sample_map_type *>(
      allocator->malloc(sizeof(sample_map_type))),
    sample_map_type(allocator));

  typedef DurabilityQueue<sample_data_type> data_queue_type;

  if (this->kind_ == DDS::PERSISTENT_DURABILITY_QOS) {
    // Read data from the filesystem and create the in-memory data structures
    // as if we had called insert() once for each "datawriter" directory.
    using OpenDDS::FileSystemStorage::Directory;
    using OpenDDS::FileSystemStorage::File;
    Directory::Ptr root_dir = Directory::create(this->data_dir_.c_str());
    OPENDDS_VECTOR(OPENDDS_STRING) path(4);  // domain, topic, type, datawriter

    for (Directory::DirectoryIterator domain = root_dir->begin_dirs(),
         domain_end = root_dir->end_dirs(); domain != domain_end; ++domain) {
      path[0] = domain->name();
      DDS::DomainId_t domain_id;
      {
        domain_id = ACE_OS::atoi(path[0].c_str());
      }

      for (Directory::DirectoryIterator topic = domain->begin_dirs(),
           topic_end = domain->end_dirs(); topic != topic_end; ++topic) {
        path[1] = topic->name();

        for (Directory::DirectoryIterator type = topic->begin_dirs(),
             type_end = topic->end_dirs(); type != type_end; ++type) {
          path[2] = type->name();

          key_type key(domain_id, path[1].c_str(), path[2].c_str(),
                       allocator);
          sample_list_type * sample_list = 0;
          ACE_NEW_MALLOC(sample_list,
                         static_cast<sample_list_type *>(
                           allocator->malloc(sizeof(sample_list_type))),
                         sample_list_type(0, static_cast<data_queue_type *>(0),
                                          allocator));
          this->samples_->bind(key, sample_list, allocator);

          for (Directory::DirectoryIterator dw = type->begin_dirs(),
               dw_end = type->end_dirs(); dw != dw_end; ++dw) {
            path[3] = dw->name();

            size_t old_len = sample_list->size();
            sample_list->size(old_len + 1);
            data_queue_type *& slot = (*sample_list)[old_len];

            // This variable is called "samples" in the insert() method be
            // we already have a "samples_" which is the overall data structure.
            data_queue_type * sample_queue = 0;
            ACE_NEW_MALLOC(sample_queue,
                           static_cast<data_queue_type *>(
                             allocator->malloc(sizeof(data_queue_type))),
                           data_queue_type(allocator));

            slot = sample_queue;
            sample_queue->fs_path_ = path;

            for (Directory::FileIterator file = dw->begin_files(),
                 file_end = dw->end_files(); file != file_end; ++file) {
              std::ifstream is;

              if (!file->read(is)) {
                if (DCPS_debug_level) {
                  ACE_ERROR((LM_ERROR,
                             ACE_TEXT("(%P|%t) DataDurabilityCache::init ")
                             ACE_TEXT("couldn't open file for PERSISTENT ")
                             ACE_TEXT("data: %C\n"), file->name().c_str()));
                }
                continue;
              }

              DDS::Time_t timestamp;
              is >> timestamp.sec >> timestamp.nanosec >> std::noskipws;
              is.get(); // consume separator

              const size_t CHUNK = 4096;
              ACE_Message_Block mb(CHUNK);
              ACE_Message_Block * current = &mb;

              while (!is.eof()) {
                is.read(current->wr_ptr(), current->space());

                if (is.bad()) break;

                current->wr_ptr((size_t)is.gcount());

                if (current->space() == 0) {
                  ACE_Message_Block * old = current;
                  current = new ACE_Message_Block(CHUNK);
                  old->cont(current);
                }
              }

              sample_queue->enqueue_tail(
                sample_data_type(timestamp, mb, allocator));

              if (mb.cont()) mb.cont()->release();    // delete the cont() chain
            }
          }
        }
      }
    }
  }

  this->reactor_ = TheServiceParticipant->timer();
}

OpenDDS::DCPS::DataDurabilityCache::~DataDurabilityCache()
{
  // Cancel timers that haven't expired yet.
  timer_id_list_type::const_iterator const end(
    this->cleanup_timer_ids_.end());

  for (timer_id_list_type::const_iterator i(
         this->cleanup_timer_ids_.begin());
       i != end;
       ++i) {
    (void) this->reactor_->cancel_timer(*i);
  }

  // Clean up memory that isn't automatically managed.
  if (this->allocator_.get() != 0) {
    sample_map_type::iterator const map_end = this->samples_->end();

    for (sample_map_type::iterator s = this->samples_->begin();
         s != map_end;
         ++s) {
      sample_list_type * const list = (*s).int_id_;

      size_t const len = list->size();;

      for (size_t l = 0; l != len; ++l) {
        ACE_DES_FREE((*list)[l],
                     this->allocator_->free,
                     DurabilityQueue<sample_data_type>);
      }

      ACE_DES_FREE(list,
                   this->allocator_->free,
                   DurabilityArray<DurabilityQueue<sample_data_type> *>);
    }

    // Yes, this looks strange but please leave it in place.  The third param
    // to ACE_DES_FREE must be the actual class name since it's used in an
    // explicit desturctor call (~T()).  Typedefs are not allowed here.  This
    // is why the two ACE_DES_FREE's above are not the typedefs.  Below we use
    // a macro to hide the internal comma from ACE_DES_FREE's macro expansion.
#define OPENDDS_MAP_TYPE ACE_Hash_Map_With_Allocator<key_type, sample_list_type *>
    ACE_DES_FREE(this->samples_, this->allocator_->free, OPENDDS_MAP_TYPE);
#undef OPENDDS_MAP_TYPE
  }
}

bool
OpenDDS::DCPS::DataDurabilityCache::insert(
  DDS::DomainId_t domain_id,
  char const * topic_name,
  char const * type_name,
  SendStateDataSampleList & the_data,
  DDS::DurabilityServiceQosPolicy const & qos)
{
  if (the_data.size() == 0)
    return true;  // Nothing to cache.

  // Apply DURABILITY_SERVICE QoS HISTORY and RESOURCE_LIMITS related
  // settings prior to data insertion into the cache.
  int depth = qos.history_kind == DDS::KEEP_ALL_HISTORY_QOS
    ? qos.max_samples_per_instance
    : qos.history_depth;

  if (depth == DDS::LENGTH_UNLIMITED)
    depth = 0x7fffffff;

  // Iterator to first DataSampleElement to be copied.
  SendStateDataSampleList::iterator element(the_data.begin());

  if (depth < 0)
    return false; // Should never occur.

  else if (depth == 0)
    return true;  // Nothing else to do.  Discard all data.

  else if (the_data.size() > depth) {
    // N.B. Dropping data samples does not take into account
    // those samples which are not actually persisted (i.e.
    // samples with the coherent_sample_ flag set). The spec
    // does not provide any guidance in this case, therefore
    // we opt for the simplest solution and assume that there
    // are no change sets when calculating the number of
    // samples to drop.

    // Drop "old" samples.  Only keep the "depth" most recent
    // samples, i.e. those found at the tail end of the
    // SendStateDataSampleList.
    ssize_t const advance_amount = the_data.size() - depth;
    std::advance(element, advance_amount);
  }

  // -----------

  // Copy samples to the domain/topic/type-specific cache.

  key_type const key(domain_id,
                     topic_name,
                     type_name,
                     this->allocator_.get());
  SendStateDataSampleList::iterator the_end(the_data.end());
  sample_list_type * sample_list = 0;

  typedef DurabilityQueue<sample_data_type> data_queue_type;
  data_queue_type ** slot = 0;
  data_queue_type * samples = 0;  // sample_list_type::value_type

  using OpenDDS::FileSystemStorage::Directory;
  using OpenDDS::FileSystemStorage::File;
  Directory::Ptr dir;
  OPENDDS_VECTOR(OPENDDS_STRING) path;
  {
    ACE_Allocator * const allocator = this->allocator_.get();

    ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);

    if (this->kind_ == DDS::PERSISTENT_DURABILITY_QOS) {
      try {
        dir = Directory::create(this->data_dir_.c_str());

        path.push_back(to_dds_string(domain_id));
        path.push_back(topic_name);
        path.push_back(type_name);
        dir = dir->get_dir(path);
        // dir is now the "type" directory, which is shared by all datawriters
        // of the domain/topic/type.  We actually need a new directory per
        // datawriter and this assumes that insert() is called once per
        // datawriter, as is currently the case.
        dir = dir->create_next_dir();
        path.push_back(dir->name());   // for use by the Cleanup_Handler

      } catch (const std::exception& ex) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) DataDurabilityCache::insert ")
                     ACE_TEXT("couldn't create directory for PERSISTENT ")
                     ACE_TEXT("data: %C\n"), ex.what()));
        }

        dir.reset();
      }
    }

    if (this->samples_->find(key, sample_list, allocator) != 0) {
      // Create a new list (actually an ACE_Array_Base<>) with the
      // appropriate allocator passed to its constructor.
      ACE_NEW_MALLOC_RETURN(
        sample_list,
        static_cast<sample_list_type *>(
          allocator->malloc(sizeof(sample_list_type))),
        sample_list_type(1, static_cast<data_queue_type *>(0), allocator),
        false);

      if (this->samples_->bind(key, sample_list, allocator) != 0)
        return false;
    }

    data_queue_type ** const begin = &((*sample_list)[0]);
    data_queue_type ** const end =
      begin + sample_list->size();

    // Find an empty slot in the array.  This is a linear search but
    // that should be fine for the common case, i.e. a small number of
    // DataWriters that push data into the cache.
    slot = std::find(begin,
                     end,
                     static_cast<data_queue_type *>(0));

    if (slot == end) {
      // No available slots.  Grow the array accordingly.
      size_t const old_len = sample_list->size();
      sample_list->size(old_len + 1);

      data_queue_type ** new_begin = &((*sample_list)[0]);
      slot = new_begin + old_len;
    }

    ACE_NEW_MALLOC_RETURN(
      samples,
      static_cast<data_queue_type *>(
        allocator->malloc(sizeof(data_queue_type))),
      data_queue_type(allocator),
      false);

    // Insert the samples in to the sample list.
    *slot = samples;

    if (!dir.is_nil()) {
      samples->fs_path_ = path;
    }

    for (SendStateDataSampleList::iterator i(element); i != the_end; ++i) {
      DataSampleElement& elem = *i;

      // N.B. Do not persist samples with coherent changes.
      // To verify, we check the DataSampleHeader for the
      // coherent_change_ flag. The DataSampleHeader will
      // always be the first message block in the chain.
      //
      // It should be noted that persisting coherent changes
      // is a non-trivial task, and should be handled when
      // finalizing persistence profile conformance.
      if (DataSampleHeader::test_flag(COHERENT_CHANGE_FLAG, elem.get_sample())) {
        continue; // skip coherent sample
      }

      sample_data_type sample(elem, allocator);

      if (samples->enqueue_tail(sample) != 0)
        return false;

      if (!dir.is_nil()) {
        try {
          File::Ptr f = dir->create_next_file();
          std::ofstream os;

          if (!f->write(os)) return false;

          DDS::Time_t timestamp;
          const char * data;
          size_t len;
          sample.get_sample(data, len, timestamp);

          os << timestamp.sec << ' ' << timestamp.nanosec << ' ';
          os.write(data, len);

        } catch (const std::exception& ex) {
          if (DCPS_debug_level > 0) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) DataDurabilityCache::insert ")
                       ACE_TEXT("couldn't write sample for PERSISTENT ")
                       ACE_TEXT("data: %C\n"), ex.what()));
          }
        }
      }
    }
  }

  // -----------

  // Schedule cleanup timer.
  //FUTURE: The cleanup delay needs to be persisted (if QoS is persistent)
  ACE_Time_Value const cleanup_delay(
    duration_to_time_value(qos.service_cleanup_delay));

  if (cleanup_delay > ACE_Time_Value::zero) {
    if (OpenDDS::DCPS::DCPS_debug_level >= 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("OpenDDS (%P|%t) Scheduling durable data ")
                 ACE_TEXT("cleanup for\n")
                 ACE_TEXT("OpenDDS (%P|%t) (domain_id, topic, type) ")
                 ACE_TEXT("== (%d, %C, %C)\n"),
                 domain_id,
                 topic_name,
                 type_name));
    }

    Cleanup_Handler * const cleanup =
      new Cleanup_Handler(*sample_list,
                          slot - &(*sample_list)[0],
                          this->allocator_.get(),
                          path,
                          this->data_dir_);
    ACE_Event_Handler_var safe_cleanup(cleanup);   // Transfer ownership
    long const tid =
      this->reactor_->schedule_timer(cleanup,
                                     0, // ACT
                                     cleanup_delay);
    if (tid == -1) {
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);

      ACE_DES_FREE(samples,
                   this->allocator_->free,
                   DurabilityQueue<sample_data_type>);
      *slot = 0;

      return false;

    } else {
      {
        ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);
        this->cleanup_timer_ids_.push_back(tid);
      }

      cleanup->timer_id(tid,
                        &this->cleanup_timer_ids_);
    }
  }

  return true;
}

bool
OpenDDS::DCPS::DataDurabilityCache::get_data(
  DDS::DomainId_t domain_id,
  char const * topic_name,
  char const * type_name,
  DataWriterImpl * data_writer,
  ACE_Allocator * mb_allocator,
  ACE_Allocator * db_allocator,
  DDS::LifespanQosPolicy const & /* lifespan */)
{
  key_type const key(domain_id,
                     topic_name,
                     type_name,
                     this->allocator_.get());

  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, false);

  sample_list_type * p_sample_list = 0;

  if (this->samples_->find(key,
                           p_sample_list,
                           this->allocator_.get()) == -1)
    return true;  // No durable data for this domain/topic/type.

  else if (p_sample_list == 0)
    return false; // Should never happen.

  sample_list_type & sample_list = *p_sample_list;

  // We will register an instance, and then write all of the cached
  // data to the DataWriter using that instance.

  sample_data_type * registration_data = 0;

  if (sample_list[0]->get(registration_data, 0) == -1)
    return false;

  char const * marshaled_sample = 0;
  size_t marshaled_sample_length = 0;
  DDS::Time_t registration_timestamp;

  registration_data->get_sample(marshaled_sample,
                                marshaled_sample_length,
                                registration_timestamp);

  // Don't use the cached allocator for the registered sample message
  // block.
  scoped_ptr<DataSample> registration_sample(
    new ACE_Message_Block(marshaled_sample_length,
                          ACE_Message_Block::MB_DATA,
                          0, //cont
                          0, //data
                          0, //alloc_strategy
                          data_writer->get_db_lock()));

  ACE_OS::memcpy(registration_sample->wr_ptr(),
                 marshaled_sample,
                 marshaled_sample_length);

  registration_sample->wr_ptr(marshaled_sample_length);

  DDS::InstanceHandle_t handle = DDS::HANDLE_NIL;

  /**
   * @todo Is this going to cause problems for users that set a finite
   *       DDS::ResourceLimitsQosPolicy::max_instances value when
   *       OpenDDS supports that value?
   */
  DDS::ReturnCode_t ret =
    data_writer->register_instance_from_durable_data(handle,
                                     registration_sample.get(),
                                     registration_timestamp);

  if (ret != DDS::RETCODE_OK)
    return false;

  registration_sample.release();

  typedef DurabilityQueue<sample_data_type> data_queue_type;
  size_t const len = sample_list.size();

  for (size_t i = 0; i != len; ++i) {
    data_queue_type * const q = sample_list[i];

    for (data_queue_type::ITERATOR j = q->begin();
         !j.done();
         j.advance()) {
      sample_data_type * data = 0;

      if (j.next(data) == 0)
        return false;  // Should never happen.

      char const * sample = 0;  // Sample does not include header.
      size_t sample_length = 0;
      DDS::Time_t source_timestamp;

      data->get_sample(sample, sample_length, source_timestamp);

      ACE_Message_Block * mb = 0;
      ACE_NEW_MALLOC_RETURN(mb,
                            static_cast<ACE_Message_Block*>(
                              mb_allocator->malloc(
                                sizeof(ACE_Message_Block))),
                            ACE_Message_Block(
                              sample_length,
                              ACE_Message_Block::MB_DATA,
                              0, // cont
                              0, // data
                              0, // allocator_strategy
                              data_writer->get_db_lock(), // data block locking_strategy
                              ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                              ACE_Time_Value::zero,
                              ACE_Time_Value::max_time,
                              db_allocator,
                              mb_allocator),
                            false);

      ACE_OS::memcpy(mb->wr_ptr(),
                     sample,
                     sample_length);
      mb->wr_ptr(sample_length);

      const DDS::ReturnCode_t ret = data_writer->write(mb, handle,
        source_timestamp, 0 /* no content filtering */);

      if (ret != DDS::RETCODE_OK) {
        ACE_DES_FREE(mb,
                     mb_allocator->free,
                     ACE_Message_Block);
        return false;
      }
    }

    // Data successfully written.  Empty the queue/list.
    /**
     * @todo If we don't empty the queue, we'll end up with duplicate
     *       data since the data retrieved from the cache will be
     *       reinserted.
     */
    q->reset();

    try {
      cleanup_directory(q->fs_path_, this->data_dir_);

    } catch (const std::exception& ex) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) DataDurabilityCache::get_data ")
                   ACE_TEXT("couldn't remove directory for PERSISTENT ")
                   ACE_TEXT("data: %C\n"), ex.what()));
      }
    }
  }
  return true;
}

#endif // OPENDDS_NO_PERSISTENCE_PROFILE
