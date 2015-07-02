// -*- C++ -*-

//=============================================================================
/**
 *  @file    OpenDDS_Publication_Manager.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_OPEN_DDS_PUBLICATION_MANAGER_H_
#define DDS_WRAPPER_OPEN_DDS_PUBLICATION_MANAGER_H_

#include "Publication_Manager_Impl.h"
#include "Domain_Manager.h"

/**
 * @class OpenDDS_Publication_Manager
 * @author Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 * @brief publication manager implementation for OpenDDS
 */
class OpenDDS_Publication_Manager : public Publication_Manager_Impl
{
 public:
  /// ctor with transport impl registration
  OpenDDS_Publication_Manager (const Domain_Manager & dm,
                               const DDS::PublisherQos & qos);

  /// dtor
  virtual ~OpenDDS_Publication_Manager ();

  /// will create a topic instance using the domain manager
  /// memory management of the returned datawriter has to be done by the caller
  virtual DDS::DataWriter_ptr access_topic (
    const Topic_Manager & topic,
    const DDS::DataWriterQos & qos,
    const Publication_Manager_Ptr & ref);

  /// unregisters and deletes the topic from the domain
  virtual void remove_topic (const Topic_Manager & topic);

  /// creates and returns qos for data writers with the default values
  virtual DDS::DataWriterQos get_default_datawriter_qos ();

  /// returns the underlying subsriber instance
  /// memory management of the returned publisher reference is done by the
  /// OpenDDS_Publication_Manager itself
  virtual DDS::Publisher_ptr publisher () const;

 private:
  /// initializes the publication manager
  void init (const DDS::PublisherQos & qos);

  /// reference to the domain manager
  Domain_Manager dm_;

  /// reference to the internally used publisher
  DDS::Publisher_var pub_;
};

#if defined (__ACE_INLINE__)
#include "OpenDDS_Publication_Manager.inl"
#endif

#endif /* DDS_WRAPPER_OPEN_DDS_PUBLICATION_MANAGER_H_ */
