/*
 */

#ifndef DCPS_THRASHER_PROGRESSINDICATOR_H
#define DCPS_THRASHER_PROGRESSINDICATOR_H

#include <cstdlib>

class ProgressIndicator
{
public:
  ProgressIndicator(const char* format,
                    const std::size_t max,
                    const std::size_t grad = 10);

  ~ProgressIndicator();

  ProgressIndicator& operator++();

private:
  const char* format_;
  const std::size_t max_;
  const std::size_t grad_;

  std::size_t last_;
  std::size_t curr_;
};

#endif /* DCPS_THRASHER_PROGRESSINDICATOR_H */
