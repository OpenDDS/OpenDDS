#ifndef ABSTRACTIONLAYER_H_
#define ABSTRACTIONLAYER_H_

#include "FileInfoC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/tcp/TcpInst.h>

#include "FileInfoTypeSupportImpl.h"


// Forward Declarations
class ApplicationLevel;


/** Variables that will be consistent across all the nodes in the same
    DDS Domain. **/

/// Id of the DDS domain that all the transactions will take place in.
/// This id needs to be in the domain_ids file that is given to the DCPSInfoRepo application
const int DOMAINID = 111;



/**
 * @class AbstractionLayer
 *
 * @brief Bridges the DDS and Application Level functionality.
 *
 */
class AbstractionLayer
{
public:
  AbstractionLayer();
  virtual ~AbstractionLayer();

  /**
   * Initialize the DDS system.
   *
   * @param argc    - number of command line arguments
   * @param *argv[] - command line arguments
   *
   * @return true if the DDS system was intialized correctly,
   *         false otherwise.
   */
  bool init_DDS(int& argc, ACE_TCHAR *argv[]);
  /**
   * Shutdown the DDS System
   */
  void shutdown_DDS();

  /**
   * Attach application level code to the abstraction layer.
   *
   * @param app
   */
  void attach_application(ApplicationLevel* app);

  /**
   * Handle the reception of a FileDiff object.
   * This method is called by the DDS system code.
   * This method will try to call the Application Level code.
   *
   * @param diff - FileDiff received by the DDS subscriber.
   */
  void receive_diff(const DistributedContent::FileDiff& diff);
  /**
   * Handle the sending of a FileDiff object.
   * This method is called by the Application Level code.
   * This method will try to call the DDS system code.
   *
   * @param diff - FileDiff to be sent by the DDS publisher
   *
   * @return
   */
  bool send_diff(const DistributedContent::FileDiff& diff);


private:
  /** References to DDS System Code **/

  /// The publishers DDS transport
  OpenDDS::DCPS::TransportImpl_rch        pub_tcp_impl_;
  /// The subscribers DDS transport
  OpenDDS::DCPS::TransportImpl_rch        sub_tcp_impl_;

  /// Factory for creating Domain Participants
  ::DDS::DomainParticipantFactory_var dpf_;
  /// The Domain Participant reference
  ::DDS::DomainParticipant_var        dp_;
  /// The Topic that will be published and subscribed to
  ::DDS::Topic_var                    topic_;

  /// The publisher reference
  ::DDS::Publisher_var                        pub_;
  /// The Data Writer reference (does the actual publishing)
  ::DDS::DataWriter_var                       dw_;
  /// Implementation of the Data writer (needed for sending the FileDiffs)
  DistributedContent::FileDiffDataWriter_var  filediff_writer_;
  /// Instance handle for publishing (one per key published)
  ::DDS::InstanceHandle_t                     handle_;

  /// The subscriber reference
  ::DDS::Subscriber_var         sub_;
  /// Listener for alerting that data has been published
  ::DDS::DataReaderListener_var listener_;
  /// Data Reader for receiving the published data
  ::DDS::DataReader_var         dr_;


  /// The reference to the Application Level code
  ApplicationLevel* application_;
};

#endif
