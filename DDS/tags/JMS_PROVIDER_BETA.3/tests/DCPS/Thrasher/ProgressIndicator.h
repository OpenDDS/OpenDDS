/*
 * $Id$
 */

#ifndef DCPS_THRASHER_PROGRESSINDICATOR_H
#define DCPS_THRASHER_PROGRESSINDICATOR_H

#include <cstdlib>

class ProgressIndicator
{
public:
  ProgressIndicator(const char* format,
                    const std::size_t max,
                    unsigned grad = 10);

  ~ProgressIndicator();

  ProgressIndicator& operator++();

private:
  const char* format_;
  const std::size_t max_;
  unsigned grad_;
  
  std::size_t curr_;
  unsigned last_; 
};

#endif /* DCPS_THRASHER_PROGRESSINDICATOR_H */
