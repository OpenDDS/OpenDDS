#ifndef DCPS_THRASHER_PROGRESSINDICATOR_H
#define DCPS_THRASHER_PROGRESSINDICATOR_H

#include <cstdlib>

class ProgressIndicator
{
public:
  ProgressIndicator(const char* format, size_t max, size_t grad = 10);
  ~ProgressIndicator();
  ProgressIndicator& operator++();

private:
  const char* format_;
  const size_t max_;
  const size_t grad_;
  size_t last_;
  size_t curr_;
};

#endif /* DCPS_THRASHER_PROGRESSINDICATOR_H */
