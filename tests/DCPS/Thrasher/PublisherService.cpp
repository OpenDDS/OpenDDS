
#include "PublisherService.h"

#include <string>

PublisherService::PublisherService(const long domain_id, std::size_t samples_per_thread, bool durable)
  : domain_id_(domain_id)
  , samples_per_thread_(samples_per_thread)
  , durable_(durable)
  , mutex_()
  , thread_index_(0)
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> PublisherService::PublisherService\n")));
}

PublisherService::~PublisherService()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- PublisherService::~PublisherService\n")));
}

bool PublisherService::start(const int n_threads, const long flags)
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> PublisherService::start (%d threads)\n"), n_threads));
  const int r = activate(flags, n_threads);
  if (r != 0) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) ERROR: PublisherService::start - activate() returned %d\n"), r));
  }
  return r == 0;
}

bool PublisherService::end()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- PublisherService::end\n")));
  const int r = wait();
  if (r != 0) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) ERROR: PublisherService::end - wait() returned %d\n"), r));
  }
  return r == 0;
}

int PublisherService::svc()
{
  try {
    Publisher::Ptr pub(createPublisher());
    return pub->publish();
  } catch (...) {
    ACE_ERROR((LM_ERROR, ("(%P|%t) ERROR: PublisherService::svc exception\n")));
  }
  return 1;
}

// Note: With std::unique_ptr, this method should return Publisher::Ptr.
Publisher* PublisherService::createPublisher()
{
  Lock lock(mutex_);
  return new Publisher(domain_id_, samples_per_thread_, durable_, thread_index_++);
}
