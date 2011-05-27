namespace OpenDDS
{
  namespace DCPS
  {
    ACE_INLINE
    DCPSInfo_ptr 
    Service_Participant::get_repository() const
    {
      ACE_ASSERT ( ! CORBA::is_nil (repo_.in ()));
      return DCPSInfo::_duplicate (repo_.in ());
    }

    ACE_INLINE
    ::DDS::UserDataQosPolicy               
    Service_Participant::initial_UserDataQosPolicy () const
    {
      return initial_UserDataQosPolicy_;
    }

    ACE_INLINE
    ::DDS::TopicDataQosPolicy              
    Service_Participant::initial_TopicDataQosPolicy () const
    {
      return initial_TopicDataQosPolicy_;
    }

    ACE_INLINE
    ::DDS::GroupDataQosPolicy              
    Service_Participant::initial_GroupDataQosPolicy () const
    {
      return initial_GroupDataQosPolicy_;
    }

    ACE_INLINE
    ::DDS::TransportPriorityQosPolicy      
    Service_Participant::initial_TransportPriorityQosPolicy () const
    {
      return initial_TransportPriorityQosPolicy_;
    }

    ACE_INLINE
    ::DDS::LifespanQosPolicy               
    Service_Participant::initial_LifespanQosPolicy () const
    {
      return initial_LifespanQosPolicy_;
    }

    ACE_INLINE
    ::DDS::DurabilityQosPolicy             
    Service_Participant::initial_DurabilityQosPolicy () const
    {
      return initial_DurabilityQosPolicy_;
    }

    ACE_INLINE
    ::DDS::PresentationQosPolicy           
    Service_Participant::initial_PresentationQosPolicy () const
    {
      return initial_PresentationQosPolicy_;
    }

    ACE_INLINE
    ::DDS::DeadlineQosPolicy               
    Service_Participant::initial_DeadlineQosPolicy () const
    {
      return initial_DeadlineQosPolicy_;
    }

    ACE_INLINE
    ::DDS::LatencyBudgetQosPolicy          
    Service_Participant::initial_LatencyBudgetQosPolicy () const
    {
      return initial_LatencyBudgetQosPolicy_;
    }

    ACE_INLINE
    ::DDS::OwnershipQosPolicy              
    Service_Participant::initial_OwnershipQosPolicy () const
    {
      return initial_OwnershipQosPolicy_;
    }

    ACE_INLINE
    ::DDS::OwnershipStrengthQosPolicy      
    Service_Participant::initial_OwnershipStrengthQosPolicy () const
    {
      return initial_OwnershipStrengthQosPolicy_;
    }

    ACE_INLINE
    ::DDS::LivelinessQosPolicy             
    Service_Participant::initial_LivelinessQosPolicy () const
    {
      return initial_LivelinessQosPolicy_;
    }

    ACE_INLINE
    ::DDS::TimeBasedFilterQosPolicy        
    Service_Participant::initial_TimeBasedFilterQosPolicy () const
    {
      return initial_TimeBasedFilterQosPolicy_;
    }

    ACE_INLINE
    ::DDS::PartitionQosPolicy              
    Service_Participant::initial_PartitionQosPolicy () const
    {
      return initial_PartitionQosPolicy_;
    }

    ACE_INLINE
    ::DDS::ReliabilityQosPolicy            
    Service_Participant::initial_ReliabilityQosPolicy () const
    {
      return initial_ReliabilityQosPolicy_;
    }

    ACE_INLINE
    ::DDS::DestinationOrderQosPolicy       
    Service_Participant::initial_DestinationOrderQosPolicy () const
    {
      return initial_DestinationOrderQosPolicy_;
    }

    ACE_INLINE
    ::DDS::HistoryQosPolicy                
    Service_Participant::initial_HistoryQosPolicy () const
    {
      return initial_HistoryQosPolicy_;
    }

    ACE_INLINE
    ::DDS::ResourceLimitsQosPolicy         
    Service_Participant::initial_ResourceLimitsQosPolicy () const
    {
      return initial_ResourceLimitsQosPolicy_;
    }

    ACE_INLINE
    ::DDS::EntityFactoryQosPolicy          
    Service_Participant::initial_EntityFactoryQosPolicy () const
    {
      return initial_EntityFactoryQosPolicy_;
    }

    ACE_INLINE
    ::DDS::WriterDataLifecycleQosPolicy    
    Service_Participant::initial_WriterDataLifecycleQosPolicy () const
    {
      return initial_WriterDataLifecycleQosPolicy_;
    }

    ACE_INLINE
    ::DDS::ReaderDataLifecycleQosPolicy    
    Service_Participant::initial_ReaderDataLifecycleQosPolicy () const
    {
      return initial_ReaderDataLifecycleQosPolicy_;
    }

    ACE_INLINE
    ::DDS::DomainParticipantQos            
    Service_Participant::initial_DomainParticipantQos () const
    {
      return initial_DomainParticipantQos_;
    }

    ACE_INLINE
    ::DDS::TopicQos                        
    Service_Participant::initial_TopicQos () const
    {
      return initial_TopicQos_;
    }

    ACE_INLINE
    ::DDS::DataWriterQos                   
    Service_Participant::initial_DataWriterQos () const
    {
      return initial_DataWriterQos_;
    }

    ACE_INLINE
    ::DDS::PublisherQos                    
    Service_Participant::initial_PublisherQos () const
    {
      return initial_PublisherQos_;
    }

    ACE_INLINE
    ::DDS::DataReaderQos                   
    Service_Participant::initial_DataReaderQos () const
    {
      return initial_DataReaderQos_;
    }

    ACE_INLINE
    ::DDS::SubscriberQos                   
    Service_Participant::initial_SubscriberQos () const
    {
      return initial_SubscriberQos_;
    }

  } // namespace ::DDS
} // namespace OpenDDS
