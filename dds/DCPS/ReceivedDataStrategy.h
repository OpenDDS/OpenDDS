/*
 * $Id$
 */

#ifndef DCPS_RECEIVEDDATASTRATEGY_H
#define DCPS_RECEIVEDDATASTRATEGY_H

namespace OpenDDS
{
namespace DCPS
{
class ReceivedDataElementList;
class ReceivedDataElement;

class OpenDDS_Dcps_Export ReceivedDataStrategy
{
public:
  explicit ReceivedDataStrategy(ReceivedDataElementList& rcvd_samples);

  virtual ~ReceivedDataStrategy();

  virtual void add(ReceivedDataElement* data_sample) = 0;

protected:
  ReceivedDataElementList& rcvd_samples_;
};

class OpenDDS_Dcps_Export ReceptionDataStrategy
  : public ReceivedDataStrategy
{
public:
  explicit ReceptionDataStrategy(ReceivedDataElementList& rcvd_samples);

  ~ReceptionDataStrategy();

  virtual void add(ReceivedDataElement* data_sample);
};

class OpenDDS_Dcps_Export SourceDataStrategy
  : public ReceivedDataStrategy
{
public:
  explicit SourceDataStrategy(ReceivedDataElementList& rcvd_samples);

  ~SourceDataStrategy();

  virtual void add(ReceivedDataElement* data_sample);
};

} // namespace
} // namespace

#endif /* DCPS_RECEIVEDDATASTRATEGY_H */
