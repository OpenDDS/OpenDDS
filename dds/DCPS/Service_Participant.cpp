// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "debug.h"
#include "Service_Participant.h"
#include "BuiltInTopicUtils.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "tao/ORB_Core.h"
#include "tao/TAO_Singleton.h"

#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"
#include "ace/Configuration_Import_Export.h"
#include "ace/Service_Config.h"
#include "ace/Argv_Type_Converter.h"

#if ! defined (__ACE_INLINE__)
#include "Service_Participant.inl"
#endif /* __ACE_INLINE__ */


namespace OpenDDS
{
  namespace DCPS
  {
    int Service_Participant::zero_argc = 0;

    const int DEFAULT_BIT_TRANSPORT_PORT = 0; // let the OS pick the port

    const size_t DEFAULT_NUM_CHUNKS = 20;

    const size_t DEFAULT_CHUNK_MULTIPLIER = 10;

    const int BIT_LOOKUP_DURATION_MSEC = 2000;

    static ACE_TString config_fname (ACE_TEXT(""));

    static const ACE_TCHAR DEFAULT_REPO_IOR[] = ACE_TEXT("file://repo.ior");

    static const ACE_TCHAR COMMON_SECTION_NAME[] = ACE_TEXT("common");
    static const ACE_TCHAR DOMAIN_SECTION_NAME[] = ACE_TEXT("domain");
    static const ACE_TCHAR REPO_SECTION_NAME[]   = ACE_TEXT("repository");

    static bool got_debug_level = false;
    static bool got_info = false;
    static bool got_chunks = false;
    static bool got_chunk_association_multiplier = false;
    static bool got_liveliness_factor = false;
    static bool got_bit_transport_port = false;
    static bool got_bit_transport_ip = false;
    static bool got_bit_lookup_duration_msec = false;
    static bool got_bit_flag = false;

    Service_Participant::Service_Participant ()
    : orb_ (CORBA::ORB::_nil ()),
      orb_from_user_(0),
      n_chunks_ (DEFAULT_NUM_CHUNKS),
      association_chunk_multiplier_(DEFAULT_CHUNK_MULTIPLIER),
      liveliness_factor_ (80),
      bit_enabled_ (
#ifdef DDS_HAS_MINIMUM_BIT
                    false
#else
                    true
#endif
                    ),
      bit_lookup_duration_msec_ (BIT_LOOKUP_DURATION_MSEC)
    {
      this->set_repo_domain( ANY_DOMAIN, DEFAULT_REPO);
      initialize();
    }

    Service_Participant::~Service_Participant ()
    {
    }

    Service_Participant *
    Service_Participant::instance (void)
    {
      // Hide the template instantiation to prevent multiple instances
      // from being created.

      return
	TAO_Singleton<Service_Participant, TAO_SYNCH_MUTEX>::instance ();
    }

    int
    Service_Participant::svc ()
    {
        {
          bool done = false;
          while (! done)
            {
              try
                {
                  if (orb_->orb_core()->has_shutdown () == false)
                    {
                      orb_->run ();
                    }
                  done = true;
                }
              catch (const CORBA::SystemException& sysex)
                {
                  sysex._tao_print_exception (
                    "ERROR: Service_Participant::svc");
                }
              catch (const CORBA::UserException& userex)
                {
                  userex._tao_print_exception (
                    "ERROR: Service_Participant::svc");
                }
              catch (const CORBA::Exception& ex)
                {
                  ex._tao_print_exception (
                    "ERROR: Service_Participant::svc");
                }
              if (orb_->orb_core()->has_shutdown ())
                {
                  done = true;
                }
              else
                {
                  orb_->orb_core()->reactor()->reset_reactor_event_loop ();
                }
            }
        }

      return 0;
    }

    int
    Service_Participant::set_ORB (CORBA::ORB_ptr orb)
    {
      // The orb is already created by the get_domain_participant_factory() call.
      ACE_ASSERT (CORBA::is_nil (orb_.in ()));
      // The provided orb should not be nil.
      ACE_ASSERT (! CORBA::is_nil (orb));

      orb_ = CORBA::ORB::_duplicate (orb);
      orb_from_user_ = 1;
      return 0;
    }

    CORBA::ORB_ptr
    Service_Participant::get_ORB ()
    {
      // This method should be called after either set_ORB is called
      // or get_domain_participant_factory is called.
      ACE_ASSERT (! CORBA::is_nil (orb_.in ()));

      return CORBA::ORB::_duplicate (orb_.in ());
    }

    PortableServer::POA_ptr
    Service_Participant::the_poa()
    {
      if (CORBA::is_nil (root_poa_.in ()))
        {
          CORBA::Object_var obj = orb_->resolve_initial_references( "RootPOA" );
          root_poa_ = PortableServer::POA::_narrow( obj.in() );
        }
      return PortableServer::POA::_duplicate (root_poa_.in ());
    }

    void
    Service_Participant::shutdown ()
    {
      try
        {
          ACE_GUARD (TAO_SYNCH_MUTEX, guard, this->factory_lock_);
          ACE_ASSERT (! CORBA::is_nil (orb_.in ()));
          if (! orb_from_user_)
            {
              orb_->shutdown (0);
              this->wait ();
            }
        // Don't delete the participants - require the client code to delete participants
        #if 0
          //TBD return error code from this call
          // -- non-empty entity will make this call return failure
          if (dp_factory_impl_->delete_contained_participants () != ::DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT ("(%P|%t) ERROR: Service_Participant::shutdown, ")
                          ACE_TEXT ("delete_contained_participants failed.\n")));
            }
        #endif

          if (! orb_from_user_)
            {
              root_poa_->destroy (1, 1);
              orb_->destroy ();
            }
          orb_ = CORBA::ORB::_nil ();
          dp_factory_ = ::DDS::DomainParticipantFactory::_nil ();
        }
      catch (const CORBA::Exception& ex)
        {
          ex._tao_print_exception ("ERROR: Service_Participant::shutdown");
          return;
        }
    }


    ::DDS::DomainParticipantFactory_ptr
    Service_Participant::get_domain_participant_factory (int &argc,
                                                         ACE_TCHAR *argv[])
    {
      if (CORBA::is_nil (dp_factory_.in ()))
        {
          ACE_GUARD_RETURN (TAO_SYNCH_MUTEX,
                            guard,
                            this->factory_lock_,
                            ::DDS::DomainParticipantFactory::_nil ());

          if (CORBA::is_nil (dp_factory_.in ()))
            {
              try
                {
                  if (CORBA::is_nil (orb_.in ()))
                    {
                      ACE_Argv_Type_Converter converter (argc, argv);

                      //TBD: allow user to specify the ORB id

                      // Use a unique ORB for the ::DDS Service
                      // to avoid conflicts with other CORBA code
                      orb_ = CORBA::ORB_init (argc,
                                              converter.get_ASCII_argv(),
                                              "TAO_DDS_DCPS");
                    }

                  if (parse_args (argc, argv) != 0)
                    {
                      return ::DDS::DomainParticipantFactory::_nil ();
                    }

                  ACE_ASSERT ( ! CORBA::is_nil (orb_.in ()));

                  if (config_fname == ACE_TEXT(""))
                    {
                      ACE_DEBUG ((LM_INFO,
                        ACE_TEXT ("(%P|%t)INFO: not using file configuration - no configuration "
                        "file specified.\n")));
                    }
                  else
                    {
                      // Load configuration only if the configuration file exists.
                      FILE* in = ACE_OS::fopen (config_fname.c_str(), ACE_LIB_TEXT ("r"));
                      if (!in)
                        {
                          ACE_DEBUG ((LM_INFO,
                                      ACE_TEXT("(%P|%t)INFO: not using file configuration - "
                                      "can not open \"%s\" for reading. %p\n"),
                                      config_fname.c_str(), ACE_TEXT("fopen")));
                        }
                      else
                        {
                          ACE_OS::fclose (in);

                          if (this->load_configuration () != 0)
                            {
                              ACE_ERROR ((LM_ERROR,
                                          ACE_TEXT("(%P|%t)Service_Participant::get_domain_participant_factory: ")
                                          ACE_TEXT("load_configuration() failed.\n")));
                              return ::DDS::DomainParticipantFactory::_nil ();
                            }
                        }
                    }

                  CORBA::Object_var poa_object =
                    orb_->resolve_initial_references("RootPOA");

                  root_poa_ = PortableServer::POA::_narrow (poa_object.in ());

                  if (CORBA::is_nil (root_poa_.in ()))
                    {
                      ACE_ERROR ((LM_ERROR,
                                  ACE_TEXT ("(%P|%t) ERROR: ")
                                  ACE_TEXT ("Service_Participant::get_domain_participant_factory, ")
                                  ACE_TEXT ("nil RootPOA\n")));
                      return ::DDS::DomainParticipantFactory::_nil ();
                    }

                  ACE_NEW_RETURN (dp_factory_servant_,
                                  DomainParticipantFactoryImpl (),
                                  ::DDS::DomainParticipantFactory::_nil ());

                  dp_factory_ = dp_factory_servant_;

                  // Give ownership to poa.
                  //REMOVE SHH ???? dp_factory_servant_->_remove_ref ();

                  if (CORBA::is_nil (dp_factory_.in ()))
                    {
                      ACE_ERROR ((LM_ERROR,
                                  ACE_TEXT ("(%P|%t) ERROR: ")
                                  ACE_TEXT ("Service_Participant::get_domain_participant_factory, ")
                                  ACE_TEXT ("nil DomainParticipantFactory. \n")));
                      return ::DDS::DomainParticipantFactory::_nil ();
                    }


                  if (! this->orb_from_user_)
                    {
                      PortableServer::POAManager_var poa_manager =
                        root_poa_->the_POAManager ();

                      poa_manager->activate ();

                      if (activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1)
                        {
                          ACE_ERROR ((LM_ERROR,
                                      ACE_TEXT ("ERROR: Service_Participant::get_domain_participant_factory, ")
                                      ACE_TEXT ("Failed to activate the orb task.")));
                          return ::DDS::DomainParticipantFactory::_nil ();
                        }
                    }
                }
              catch (const CORBA::Exception& ex)
                {
                  ex._tao_print_exception (
                    "ERROR: Service_Participant::get_domain_participant_factory");
                  return ::DDS::DomainParticipantFactory::_nil ();
                }
            }
        }

      return ::DDS::DomainParticipantFactory::_duplicate (dp_factory_.in ());
    }


    int
    Service_Participant::parse_args (int &argc, ACE_TCHAR *argv[])
    {
      ACE_Arg_Shifter arg_shifter (argc, argv);

      while (arg_shifter.is_anything_left ())
        {
          const ACE_TCHAR *currentArg = 0;

          if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSDebugLevel"))) != 0)
            {
              set_DCPS_debug_level (ACE_OS::atoi (currentArg));
              arg_shifter.consume_arg ();
              got_debug_level = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSInfoRepo"))) != 0)
            {
              this->set_repo_ior( currentArg, DEFAULT_REPO);
              arg_shifter.consume_arg ();
              got_info = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSInfo"))) != 0)
            {
              // Deprecated, use -DCPSInfoRepo
              this->set_repo_ior( currentArg, DEFAULT_REPO);
              arg_shifter.consume_arg ();
              got_info = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSChunks"))) != 0)
            {
              n_chunks_ = ACE_OS::atoi (currentArg);
              arg_shifter.consume_arg ();
              got_chunks = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSChunkAssociationMutltiplier"))) != 0)
            {
              association_chunk_multiplier_ = ACE_OS::atoi (currentArg);
              arg_shifter.consume_arg ();
              got_chunk_association_multiplier = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSConfigFile"))) != 0)
            {
              config_fname = currentArg;
              arg_shifter.consume_arg ();
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSLivelinessFactor"))) != 0)
            {
              liveliness_factor_ = ACE_OS::atoi (currentArg);
              arg_shifter.consume_arg ();
              got_liveliness_factor = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBitTransportPort"))) != 0)
            {
              this->bitTransportPortMap_[ DEFAULT_REPO] = ACE_OS::atoi (currentArg);
              arg_shifter.consume_arg ();
              got_bit_transport_port = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBitTransportIPAddress"))) != 0)
            {
              this->bitTransportIpMap_[ DEFAULT_REPO] = currentArg;
              arg_shifter.consume_arg ();
              got_bit_transport_ip = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBitLookupDurationMsec"))) != 0)
            {
              bit_lookup_duration_msec_ = ACE_OS::atoi (currentArg);
              arg_shifter.consume_arg ();
              got_bit_lookup_duration_msec = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-DCPSBit"))) != 0)
            {
              bit_enabled_ = ACE_OS::atoi (currentArg);
              arg_shifter.consume_arg ();
              got_bit_flag = true;
            }
          else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-TransportDebugLevel"))) != 0)
            {
              ::OpenDDS::DCPS::Transport_debug_level = ACE_OS::atoi (currentArg);
              arg_shifter.consume_arg ();
            }
          else
            {
              arg_shifter.ignore_arg ();
            }
        }

      // Indicates sucessful parsing of the command line
      return 0;
    }

    void
    Service_Participant::initialize ()
    {
      //NOTE: in the future these initial values may be configurable
      //      (to override the Specification's default values
      //       hmm - I guess that would be OK since the user
      //       is overriding them.)
      initial_TransportPriorityQosPolicy_.value = 0;
      initial_LifespanQosPolicy_.duration.sec = ::DDS::DURATION_INFINITY_SEC;
      initial_LifespanQosPolicy_.duration.nanosec = ::DDS::DURATION_INFINITY_NSEC;

      initial_DurabilityQosPolicy_.kind = ::DDS::VOLATILE_DURABILITY_QOS;
      initial_DurabilityQosPolicy_.service_cleanup_delay.sec = ::DDS::DURATION_ZERO_SEC;
      initial_DurabilityQosPolicy_.service_cleanup_delay.nanosec = ::DDS::DURATION_ZERO_NSEC;

      initial_DurabilityServiceQosPolicy_.service_cleanup_delay.sec =
        ::DDS::DURATION_ZERO_SEC;
      initial_DurabilityServiceQosPolicy_.service_cleanup_delay.nanosec =
        ::DDS::DURATION_ZERO_NSEC;
      initial_DurabilityServiceQosPolicy_.history_kind =
        ::DDS::KEEP_LAST_HISTORY_QOS;
      initial_DurabilityServiceQosPolicy_.history_depth = 1;
      initial_DurabilityServiceQosPolicy_.max_samples =
        ::DDS::LENGTH_UNLIMITED;
      initial_DurabilityServiceQosPolicy_.max_instances =
        ::DDS::LENGTH_UNLIMITED;
      initial_DurabilityServiceQosPolicy_.max_samples_per_instance =
        ::DDS::LENGTH_UNLIMITED;

      initial_PresentationQosPolicy_.access_scope = ::DDS::INSTANCE_PRESENTATION_QOS;
      initial_PresentationQosPolicy_.coherent_access = 0;
      initial_PresentationQosPolicy_.ordered_access = 0;

      initial_DeadlineQosPolicy_.period.sec = ::DDS::DURATION_INFINITY_SEC;
      initial_DeadlineQosPolicy_.period.nanosec = ::DDS::DURATION_INFINITY_NSEC;

      initial_LatencyBudgetQosPolicy_.duration.sec = ::DDS::DURATION_ZERO_SEC;
      initial_LatencyBudgetQosPolicy_.duration.nanosec = ::DDS::DURATION_ZERO_NSEC;

      initial_OwnershipQosPolicy_.kind = ::DDS::SHARED_OWNERSHIP_QOS;
      initial_OwnershipStrengthQosPolicy_.value = 0;

      initial_LivelinessQosPolicy_.kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
      initial_LivelinessQosPolicy_.lease_duration.sec = ::DDS::DURATION_INFINITY_SEC;
      initial_LivelinessQosPolicy_.lease_duration.nanosec = ::DDS::DURATION_INFINITY_NSEC;

      initial_TimeBasedFilterQosPolicy_.minimum_separation.sec = ::DDS::DURATION_ZERO_SEC;
      initial_TimeBasedFilterQosPolicy_.minimum_separation.nanosec = ::DDS::DURATION_ZERO_NSEC;

      initial_ReliabilityQosPolicy_.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      // The spec does not provide the default max_blocking_time.
      initial_ReliabilityQosPolicy_.max_blocking_time.sec = ::DDS::DURATION_INFINITY_SEC;
      initial_ReliabilityQosPolicy_.max_blocking_time.nanosec = ::DDS::DURATION_INFINITY_NSEC;

      initial_DestinationOrderQosPolicy_.kind = ::DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

      initial_HistoryQosPolicy_.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      initial_HistoryQosPolicy_.depth = 1;

      initial_ResourceLimitsQosPolicy_.max_samples = ::DDS::LENGTH_UNLIMITED;
      initial_ResourceLimitsQosPolicy_.max_instances = ::DDS::LENGTH_UNLIMITED;
      initial_ResourceLimitsQosPolicy_.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;

      initial_EntityFactoryQosPolicy_.autoenable_created_entities = 1;

      initial_WriterDataLifecycleQosPolicy_.autodispose_unregistered_instances = 1;

      initial_ReaderDataLifecycleQosPolicy_.autopurge_nowriter_samples_delay.sec = ::DDS::DURATION_ZERO_SEC;
      initial_ReaderDataLifecycleQosPolicy_.autopurge_nowriter_samples_delay.nanosec = ::DDS::DURATION_ZERO_NSEC;

      initial_DomainParticipantQos_.user_data = initial_UserDataQosPolicy_;
      initial_DomainParticipantQos_.entity_factory = initial_EntityFactoryQosPolicy_;

      initial_TopicQos_.topic_data = initial_TopicDataQosPolicy_;
      initial_TopicQos_.durability = initial_DurabilityQosPolicy_;
      initial_TopicQos_.durability_service = initial_DurabilityServiceQosPolicy_;
      initial_TopicQos_.deadline = initial_DeadlineQosPolicy_;
      initial_TopicQos_.latency_budget = initial_LatencyBudgetQosPolicy_;
      initial_TopicQos_.liveliness = initial_LivelinessQosPolicy_;
      initial_TopicQos_.reliability = initial_ReliabilityQosPolicy_;
      initial_TopicQos_.destination_order = initial_DestinationOrderQosPolicy_;
      initial_TopicQos_.history = initial_HistoryQosPolicy_;
      initial_TopicQos_.resource_limits = initial_ResourceLimitsQosPolicy_;
      initial_TopicQos_.transport_priority = initial_TransportPriorityQosPolicy_;
      initial_TopicQos_.lifespan = initial_LifespanQosPolicy_;
      initial_TopicQos_.ownership = initial_OwnershipQosPolicy_;

      initial_DataWriterQos_.durability = initial_DurabilityQosPolicy_;
      initial_DataWriterQos_.durability_service = initial_DurabilityServiceQosPolicy_;
      initial_DataWriterQos_.deadline = initial_DeadlineQosPolicy_;
      initial_DataWriterQos_.latency_budget = initial_LatencyBudgetQosPolicy_;
      initial_DataWriterQos_.liveliness = initial_LivelinessQosPolicy_;
      initial_DataWriterQos_.reliability = initial_ReliabilityQosPolicy_;
      initial_DataWriterQos_.destination_order = initial_DestinationOrderQosPolicy_;
      initial_DataWriterQos_.history = initial_HistoryQosPolicy_;
      initial_DataWriterQos_.resource_limits = initial_ResourceLimitsQosPolicy_;
      initial_DataWriterQos_.transport_priority = initial_TransportPriorityQosPolicy_;
      initial_DataWriterQos_.lifespan = initial_LifespanQosPolicy_;
      initial_DataWriterQos_.user_data = initial_UserDataQosPolicy_;
      initial_DataWriterQos_.ownership_strength = initial_OwnershipStrengthQosPolicy_;
      initial_DataWriterQos_.writer_data_lifecycle = initial_WriterDataLifecycleQosPolicy_;

      initial_PublisherQos_.presentation = initial_PresentationQosPolicy_;
      initial_PublisherQos_.partition = initial_PartitionQosPolicy_;
      initial_PublisherQos_.group_data = initial_GroupDataQosPolicy_;
      initial_PublisherQos_.entity_factory = initial_EntityFactoryQosPolicy_;

      initial_DataReaderQos_.durability = initial_DurabilityQosPolicy_;
      initial_DataReaderQos_.deadline = initial_DeadlineQosPolicy_;
      initial_DataReaderQos_.latency_budget = initial_LatencyBudgetQosPolicy_;
      initial_DataReaderQos_.liveliness = initial_LivelinessQosPolicy_;
      initial_DataReaderQos_.reliability = initial_ReliabilityQosPolicy_;
      initial_DataReaderQos_.destination_order = initial_DestinationOrderQosPolicy_;
      initial_DataReaderQos_.history = initial_HistoryQosPolicy_;
      initial_DataReaderQos_.resource_limits = initial_ResourceLimitsQosPolicy_;
      initial_DataReaderQos_.user_data = initial_UserDataQosPolicy_;
      initial_DataReaderQos_.time_based_filter = initial_TimeBasedFilterQosPolicy_;
      initial_DataReaderQos_.reader_data_lifecycle = initial_ReaderDataLifecycleQosPolicy_;

      initial_SubscriberQos_.presentation = initial_PresentationQosPolicy_;
      initial_SubscriberQos_.partition = initial_PartitionQosPolicy_;
      initial_SubscriberQos_.group_data = initial_GroupDataQosPolicy_;
      initial_SubscriberQos_.entity_factory = initial_EntityFactoryQosPolicy_;
    }

    void
    Service_Participant::set_repo_ior( const ACE_TCHAR* ior, const RepoKey key)
    {
      if( DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Repo[ %d] == %s\n"),
          key, ior
        ));
      }

      got_info = true;
      RepoMap::const_iterator location = this->repoMap_.find( key);
      if( location != this->repoMap_.end()) {
        // Repository is already loaded.  Remove the repository only if
        // it is bound to no domains.
        for( DomainRepoMap::const_iterator current = this->domainRepoMap_.begin();
             current != this->domainRepoMap_.end();
             ++current
           ) {
          if( current->second == location->first) {
            ACE_ERROR((LM_ERROR,
              ACE_TEXT( "(%P|%t) ERROR: ")
              ACE_TEXT( "Service_Participant::set_repo_ior, ")
              ACE_TEXT( "attempt to set new repository for key %d ")
              ACE_TEXT( "while previous repository still bound. \n"),
              key
            ));
            return; // throw;
          }
        }
        // @TODO: determine if (how?) we need to destroy the existing transport.
      }

      CORBA::Object_var obj
        = orb_->string_to_object(ACE_TEXT_ALWAYS_CHAR (ior));
      if( CORBA::is_nil( obj.in())) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT ("(%P|%t) ERROR: ")
                    ACE_TEXT ("Service_Participant::set_repo_ior, ")
                    ACE_TEXT ("nil DCPSInfo (%s) for key %d. \n"), ior, key));
        return; // throw;

      } else {
        this->repoMap_[ key] = DCPSInfo::_narrow( obj.in());
        if( CORBA::is_nil( this->repoMap_[ key].in())) {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT ("(%P|%t) ERROR: ")
                      ACE_TEXT ("Service_Participant::set_repo_ior, ")
                      ACE_TEXT ("unable to narrow DCPSInfo (%s) for key %d. \n"),
                      ior, key));
          return; // throw;
        }
      }

      // Create and install the Built In Topic transport for this
      // repository.
      if (this->bit_enabled_) this->init_bit_transport_impl( key);
    }

    void
    Service_Participant::set_repo( DCPSInfo_ptr repo, const RepoKey key)
    {
      this->repoMap_[ key] = DCPSInfo::_duplicate( repo);
    }

    void
    Service_Participant::set_repo_domain( const ::DDS::DomainId_t domain, const RepoKey repo)
    {
      DomainRepoMap::const_iterator where = this->domainRepoMap_.find( domain);
      if( where != this->domainRepoMap_.end()) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) WARNING: ")
          ACE_TEXT("Service_Participant::set_repo_domain, ")
          ACE_TEXT("discarding previous mapping for domain %d.\n"),
          domain
        ));
        /// @TODO: Determine correct action here.
      }
      this->domainRepoMap_[ domain] = repo;
      if( DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Domain[ %d] == Repo[ %d]\n"),
          domain, repo
        ));
      }
    }

    DCPSInfo_ptr
    Service_Participant::get_repository( const ::DDS::DomainId_t domain)
    {
      RepoKey repo = DEFAULT_REPO;
      DomainRepoMap::const_iterator where = this->domainRepoMap_.find( domain);
      if( where != this->domainRepoMap_.end()) {
        repo = where->second;
      }
      RepoMap::const_iterator location = this->repoMap_.find( repo);
      if( location == this->repoMap_.end()) {
        if( repo == DEFAULT_REPO) {
          // Set the default repository IOR if it hasn't already happened
          // by this point.  This is why this can't be const.
          this->set_repo_ior( DEFAULT_REPO_IOR, DEFAULT_REPO);
          location = this->repoMap_.find( repo);
          if( location == this->repoMap_.end()) {
            // The default IOR was invalid.
            return OpenDDS::DCPS::DCPSInfo::_nil();

          } else {
            // Found the default!
            return OpenDDS::DCPS::DCPSInfo::_duplicate( location->second);
          }

        } else {
          // Non-default repositories _must_ be loaded by application.
          return OpenDDS::DCPS::DCPSInfo::_nil();
        }
      }
      return OpenDDS::DCPS::DCPSInfo::_duplicate( location->second);
    }

    int
    Service_Participant::bit_transport_port ( RepoKey repo) const
    {
       RepoTransportPortMap::const_iterator where = this->bitTransportPortMap_.find( repo);
       if( where == this->bitTransportPortMap_.end()) {
         return -1;
       }
       return where->second;
    }

    void
    Service_Participant::bit_transport_port (int port, RepoKey repo)
    {
       this->bitTransportPortMap_[ repo] = port;
       got_bit_transport_port = true;
    }

    int
    Service_Participant::init_bit_transport_impl ( RepoKey repo)
    {
#if !defined (DDS_HAS_MINIMUM_BIT)
      // Assign BIT transport key values starting from BIT_ALL_TRAFFIC as a base.
      // Allow for the DEFAULT_REPO value of -1 here as well.
      OpenDDS::DCPS::TransportIdType transportKey = BIT_ALL_TRAFFIC + 1 + repo;

      if( false == TheTransportFactory->obtain( transportKey).is_nil()) {
        // The transport for this repo has already been created/configured.
        if( DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) Repo[ %d].transport already loaded.\n"),
            repo
          ));
        }
        return 0;
      }

      this->bitTransportMap_[ repo]
        = TheTransportFactory->create_transport_impl (transportKey, ACE_TEXT("SimpleTcp"), DONT_AUTO_CONFIG);

      if( DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Repo[ %d].transport == %x\n"),
          repo, this->bitTransportMap_[ repo].in()
        ));
      }

      TransportConfiguration_rch config
        = TheTransportFactory->get_or_create_configuration (transportKey, ACE_TEXT("SimpleTcp"));

      SimpleTcpConfiguration* tcp_config
        = static_cast <SimpleTcpConfiguration*> (config.in ());

      if (0 == this->bitTransportIpMap_[ repo].length ())
        {
          tcp_config->local_address_.set_port_number( this->bitTransportPortMap_[ repo]);
        }
      else
        {
          tcp_config->local_address_
            = ACE_INET_Addr(
                this->bitTransportPortMap_[ repo],
                this->bitTransportIpMap_[ repo].c_str()
              );
        }

      if( this->bitTransportMap_[ repo]->configure(config.in()) != 0)
        {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::init_bit_transport_impl: ")
            ACE_TEXT("Failed to configure transport for repo %d.\n"),
            repo
          ));
          return -1;
        }
      else
        {
          return 0;
        }
#else
      ACE_UNUSED_ARG(repo);
      return -1;
#endif // DDS_HAS_MINIMUM_BIT
    }

    TransportImpl_rch
    Service_Participant::bit_transport_impl(::DDS::DomainId_t domain)
    {
      RepoKey repo = DEFAULT_REPO;
      DomainRepoMap::const_iterator where = this->domainRepoMap_.find( domain);
      if( where != this->domainRepoMap_.end()) {
        repo = where->second;
      }
      if( this->bitTransportMap_[ repo].is_nil ()) {
        if( DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) Initializing BIT transport for domain %d\n"),
            domain
          ));
        }
        init_bit_transport_impl( repo);
      }

      return this->bitTransportMap_[ repo];
    }

    int
    Service_Participant::bit_lookup_duration_msec () const
    {
      return bit_lookup_duration_msec_;
    }

    void
    Service_Participant::bit_lookup_duration_msec (int sec)
    {
      bit_lookup_duration_msec_ = sec;
      got_bit_lookup_duration_msec = true;
    }

    size_t
    Service_Participant::n_chunks () const
    {
      return n_chunks_;
    }

    void
    Service_Participant::n_chunks (size_t chunks)
    {
      n_chunks_ = chunks;
      got_chunks = true;
    }

    size_t
    Service_Participant::association_chunk_multiplier () const
    {
      return association_chunk_multiplier_;
    }

    void
    Service_Participant::association_chunk_multiplier (size_t multiplier)
    {
      association_chunk_multiplier_ = multiplier;
      got_chunk_association_multiplier = true;
    }

    void
    Service_Participant::liveliness_factor (int factor)
    {
      liveliness_factor_ = factor;
      got_liveliness_factor = true;
    }

    int
    Service_Participant::liveliness_factor () const
    {
      return liveliness_factor_;
    }

    int
    Service_Participant::load_configuration ()
    {
      int status = 0;
      if ((status = this->cf_.open ()) != 0)
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("(%P|%t)Service_Participant::load_configuration "
                                     "open() returned %d\n"),
                           status),
                           -1);

      ACE_Ini_ImpExp import (this->cf_);
      status = import.import_config (config_fname.c_str ());

      if (status != 0) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("(%P|%t)Service_Participant::load_configuration "
                           "import_config () returned %d\n"),
                           status),
                           -1);
      }

      status = this->load_common_configuration ();
      if (status != 0) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("(%P|%t)Service_Participant::load_configuration "
                           "load_common_configuration () returned %d\n"),
                           status),
                           -1);
      }

      status = this->load_domain_configuration ();
      if (status != 0) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("(%P|%t) Service_Participant::load_configuration "
                           "load_domain_configuration () returned %d\n"),
                           status),
                           -1);
      }
      status = this->load_repo_configuration ();
      if (status != 0) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("(%P|%t) Service_Participant::load_configuration "
                           "load_repo_configuration () returned %d\n"),
                           status),
                           -1);
      }
      status = TheTransportFactory->load_transport_configuration (this->cf_);
      if (status != 0) {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT ("(%P|%t)Service_Participant::load_configuration "
                           "load_transport_configuration () returned %d\n"),
                           status),
                           -1);
      }
      return 0;
    }

    int
    Service_Participant::load_common_configuration ()
    {
      const ACE_Configuration_Section_Key &root = this->cf_.root_section ();
      ACE_Configuration_Section_Key sect;
      if (this->cf_.open_section (root, COMMON_SECTION_NAME, 0, sect) != 0)
        {
          if (DCPS_debug_level > 0)
            {
              // This is not an error if the configuration file does not have
              // a common section. The code default configuration will be used.
              ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT ("(%P|%t)Service_Participant::load_common_configuration "
                          "failed to open section %s\n"),
                          COMMON_SECTION_NAME));
            }
          return 0;
        }
      else
        {
          if (got_debug_level)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSDebugLevel config value, use command option.\n")));
            }
          else
            {
              GET_CONFIG_VALUE (this->cf_, sect, ACE_TEXT("DCPSDebugLevel"), DCPS_debug_level, int)
            }
          if (got_info)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSInfoRepo config value, use command option.\n")));
            }
          else
            {
              ACE_TString value;
              GET_CONFIG_STRING_VALUE (this->cf_, sect, ACE_TEXT("DCPSInfoRepo"), value)
              this->set_repo_ior( value.c_str(), DEFAULT_REPO);
            }
          if (got_chunks)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSChunks config value, use command option.\n")));
            }
          else
            {
              GET_CONFIG_VALUE (this->cf_, sect, ACE_TEXT("DCPSChunks"), this->n_chunks_, size_t)
            }
          if (got_chunk_association_multiplier)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSChunkAssociationMutltiplier config value, use command option.\n")));
            }
          else
            {
              GET_CONFIG_VALUE (this->cf_, sect, ACE_TEXT("DCPSChunkAssociationMutltiplier"), this->association_chunk_multiplier_, size_t)
            }
          if (got_bit_transport_port)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSBitTransportPort config value, use command option.\n")));
            }
          else
            {
              GET_CONFIG_VALUE (this->cf_, sect, ACE_TEXT("DCPSBitTransportPort"), this->bitTransportPortMap_[ DEFAULT_REPO], int)
            }
          if (got_bit_transport_ip)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSBitTransportIPAddress config value, use command option.\n")));
            }
          else
            {
              GET_CONFIG_STRING_VALUE (this->cf_, sect, ACE_TEXT("DCPSBitTransportIPAddress"), this->bitTransportIpMap_[ DEFAULT_REPO])
            }
          if (got_liveliness_factor)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSLivelinessFactor config value, use command option.\n")));
            }
          else
            {
              GET_CONFIG_VALUE (this->cf_, sect, ACE_TEXT("DCPSLivelinessFactor"), this->liveliness_factor_, int)
            }
          if (got_bit_lookup_duration_msec)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSBitLookupDurationMsec config value, use command option.\n")));
            }
          else
            {
              GET_CONFIG_VALUE (this->cf_, sect, ACE_TEXT("DCPSBitLookupDurationMsec"), this->bit_lookup_duration_msec_, int)
            }
          if (got_bit_flag)
            {
              ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("(%P|%t)ignore DCPSBit config value, use command option.\n")));
            }
          else
            {
              GET_CONFIG_VALUE (this->cf_, sect, ACE_TEXT("DCPSBit"), this->bit_enabled_, int)
            }
        }

      return 0;
    }

    int
    Service_Participant::load_domain_configuration ()
    {
      const ACE_Configuration_Section_Key &root = this->cf_.root_section ();
      ACE_Configuration_Section_Key domainKey;
      if( this->cf_.open_section (root, DOMAIN_SECTION_NAME, 0, domainKey) != 0) {
        if( DCPS_debug_level > 0) {
          // This is not an error if the configuration file does not have
          // any domain (sub)section. The code default configuration will be used.
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration ")
            ACE_TEXT("failed to open [%s] section.\n"),
            DOMAIN_SECTION_NAME
          ));
        }
        return 0;

      } else {
        ACE_TString sectionName;
        for( int index = 0;
             (0 == this->cf_.enumerate_sections( domainKey, index, sectionName));
             ++index) {
          if( DCPS_debug_level > 0) {
            ACE_DEBUG(( LM_DEBUG, ACE_TEXT("(%P|%t) Examining section: %s\n"), sectionName.c_str()));
          }

          ACE_Configuration_Section_Key sectionKey;
          if( 0 != this->cf_.open_section( domainKey, sectionName.c_str(), 0, sectionKey)) {
            ACE_ERROR(( LM_ERROR,
              ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration ")
              ACE_TEXT("Unable to open [%s] section.\n"),
              sectionName.c_str()
            ));
            continue;
          }

          ACE_TString domainIdString;
          if( 0 != this->cf_.get_string_value( sectionKey, ACE_TEXT("DomainId"), domainIdString)) {
            ACE_ERROR(( LM_ERROR,
              ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration ")
              ACE_TEXT("Unable to obtain value for DomainId in [%s] section\n"),
              sectionName.c_str()));
            continue;
          }

          /// @TODO: Check this conversion.
          ::DDS::DomainId_t domainId = ACE_OS::atoi(domainIdString.c_str());
          if( DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %s: DomainId == %d\n"),
              sectionName.c_str(), domainId
            ));
          }

          ACE_TString keyString;
          if( 0 != this->cf_.get_string_value( sectionKey, ACE_TEXT("DomainRepoKey"), keyString)) {
            ACE_ERROR(( LM_ERROR,
              ACE_TEXT("(%P|%t) Service_Participant::load_domain_configuration ")
              ACE_TEXT("Unable to obtain value for DomainRepoKey in [%s] section\n"),
              sectionName.c_str()));
            continue;

          }

          RepoKey repoKey = DEFAULT_REPO;
          if( keyString != ACE_TEXT("DEFAULT_REPO")) {
            /// @TODO: Check this conversion.
            repoKey = ACE_OS::atoi(keyString.c_str());
          }
          if( DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %s: DomainRepoKey == %d\n"),
              sectionName.c_str(), repoKey
            ));
          }

          this->set_repo_domain( domainId, repoKey);
        }
      }

      return 0;
    }

    int
    Service_Participant::load_repo_configuration ()
    {
      const ACE_Configuration_Section_Key &root = this->cf_.root_section ();
      ACE_Configuration_Section_Key domainKey;
      if( this->cf_.open_section (root, REPO_SECTION_NAME, 0, domainKey) != 0) {
        if( DCPS_debug_level > 0) {
          // This is not an error if the configuration file does not have
          // any domain (sub)section. The code default configuration will be used.
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) Service_Participant::load_repo_configuration ")
            ACE_TEXT("failed to open [%s] section.\n"),
            REPO_SECTION_NAME
          ));
        }
        return 0;

      } else {
        ACE_TString sectionName;
        for( int index = 0;
             (0 == this->cf_.enumerate_sections( domainKey, index, sectionName));
             ++index) {
          if( DCPS_debug_level > 0) {
            ACE_DEBUG(( LM_DEBUG, ACE_TEXT("(%P|%t) Examining section: %s\n"), sectionName.c_str()));
          }

          ACE_Configuration_Section_Key sectionKey;
          if( 0 != this->cf_.open_section( domainKey, sectionName.c_str(), 0, sectionKey)) {
            ACE_ERROR(( LM_ERROR,
              ACE_TEXT("(%P|%t) Service_Participant::load_repo_configuration ")
              ACE_TEXT("Unable to open [%s] section.\n"),
              sectionName.c_str()
            ));
            continue;
          }

          ACE_TString keyString;
          if( 0 != this->cf_.get_string_value( sectionKey, ACE_TEXT("RepositoryKey"), keyString)) {
            ACE_ERROR(( LM_ERROR,
              ACE_TEXT("(%P|%t) Service_Participant::load_repo_configuration ")
              ACE_TEXT("Unable to obtain value for RepositoryKey in [%s] section\n"),
                       sectionName.c_str()));
            continue;
          }

          /// @TODO: Check this conversion.
          RepoKey repoKey = ACE_OS::atoi(keyString.c_str());
          if( DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %s: RepositoryKey == %d\n"),
              sectionName.c_str(), repoKey
            ));
          }

          ACE_TString repoIor;
          if( 0 != this->cf_.get_string_value( sectionKey, ACE_TEXT("RepositoryIor"), repoIor)) {
            ACE_ERROR(( LM_ERROR,
              ACE_TEXT("(%P|%t) Service_Participant::load_repo_configuration ")
              ACE_TEXT("Unable to obtain value for RepositoryIor in [%s] section\n"),
              sectionName.c_str()));
            continue;

          }
          if( DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %s: RepositoryIor == %s\n"),
              sectionName.c_str(), repoIor.c_str()
            ));
          }

          this->set_repo_ior( repoIor.c_str(), repoKey);

          ACE_TString bitIp;
          this->cf_.get_string_value( sectionKey, ACE_TEXT("DCPSBitTransportIPAddress"), bitIp);
          if( DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %s: DCPSBitTransportIPAddress == %s\n"),
              sectionName.c_str(), bitIp.c_str()
            ));
          }
          this->bitTransportIpMap_[ repoKey]   = bitIp;

          ACE_TString portString;
          this->cf_.get_string_value( sectionKey, ACE_TEXT("DCPSBitTransportPort"), portString);

          int bitPort = ACE_OS::atoi(portString.c_str());
          if( DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) %s: DCPSBitTransportPort == %d\n"),
              sectionName.c_str(), bitPort
            ));
          }
          this->bitTransportPortMap_[ repoKey] = bitPort;
        }
      }

      return 0;
    }


  } // namespace DCPS
} // namespace OpenDDS


// gcc on AIX needs explicit instantiation of the singleton templates
#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION) || (defined (__GNUC__) && defined (_AIX))

template class TAO_Singleton<Service_Participant, TAO_SYNCH_MUTEX>;

#elif defined (ACE_HAS_TEMPLATENSTANTIATION_PRAGMA)

#pragma instantiate TAO_Singleton<Service_Participant, TAO_SYNCH_MUTEX>

#endif /*ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
