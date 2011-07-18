// -*- C++ -*-
//
// $Id$
#ifndef SIMPLESUBSCRIBER_H
#define SIMPLESUBSCRIBER_H

#include  "SimpleDataReader.h"
#include  "dds/DCPS/Definitions.h"
#include  "dds/DCPS/AssociationData.h"


class SimpleSubscriber {
public:

  SimpleSubscriber ();
  virtual ~SimpleSubscriber();

  void init (OpenDDS::DCPS::TransportIdType          transport_id,
             OpenDDS::DCPS::RepoId                   sub_id,
             ssize_t                             num_publications,
             const OpenDDS::DCPS::AssociationData*   publications);

  void associate ();

  /// Returns 0 if the data_received() has not been called/completed.
  /// Returns 1 if the data_received() has been called, and all of
  /// the TransportReceiveListeners have been told of the data_received().
  int received_test_message() const;

  void remove_associations(ssize_t size,
                           const OpenDDS::DCPS::RepoId* remote_ids,
                           const OpenDDS::DCPS::RepoId sub_id);

protected:

  virtual void cleanup();

private:

  SimpleDataReader                    reader_;
  OpenDDS::DCPS::RepoId                   sub_id_;
  ssize_t                             num_publications_;
  const OpenDDS::DCPS::AssociationData*   publications_;
};

#endif  /* SIMPLESUBSCRIBER_H */
