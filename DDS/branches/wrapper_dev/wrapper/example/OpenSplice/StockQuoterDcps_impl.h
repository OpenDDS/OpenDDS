#ifndef STOCKQUOTERDCPS_IMPL_H_
#define STOCKQUOTERDCPS_IMPL_H_

#include "ccpp.h"
#include "ccpp_StockQuoter.h"
#include "ccpp_TypeSupport_impl.h"
#include "ccpp_DataWriter_impl.h"
#include "ccpp_DataReader_impl.h"
#include "os_if.h"

#ifndef DDS_API
#define DDS_API
#endif

namespace TimeBase {

}

namespace StockQuoter {

    class DDS_API QuoteTypeSupportFactory : public ::DDS::TypeSupportFactory_impl
    {
    public:
        QuoteTypeSupportFactory() {};
        virtual ~QuoteTypeSupportFactory() {};
    private:
        ::DDS::DataWriter_ptr 
        create_datawriter (gapi_dataWriter handle);
    
        ::DDS::DataReader_ptr 
        create_datareader (gapi_dataReader handle);
    };
    
    class DDS_API QuoteTypeSupport : public virtual QuoteTypeSupportInterface,
                                   public ::DDS::TypeSupport_impl
    {
    public:
        virtual ::DDS::ReturnCode_t register_type(
            ::DDS::DomainParticipant_ptr participant,
            const char * type_name) THROW_ORB_EXCEPTIONS;
    
        virtual char * get_type_name() THROW_ORB_EXCEPTIONS;    
    
        QuoteTypeSupport (void);
        virtual ~QuoteTypeSupport (void);
    
    private:
        QuoteTypeSupport (const QuoteTypeSupport &);
        void operator= (const QuoteTypeSupport &);
    
        static const char *metaDescriptor;
    };
    
    typedef QuoteTypeSupportInterface_var QuoteTypeSupport_var;
    typedef QuoteTypeSupportInterface_ptr QuoteTypeSupport_ptr;
    
    class DDS_API QuoteDataWriter_impl : public virtual QuoteDataWriter,
                                        public ::DDS::DataWriter_impl
    {
    public:
    
        virtual ::DDS::InstanceHandle_t register_instance(
            const Quote & instance_data) THROW_ORB_EXCEPTIONS;
            
        virtual ::DDS::InstanceHandle_t register_instance_w_timestamp(
            const Quote & instance_data,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
            
        virtual ::DDS::ReturnCode_t unregister_instance(
            const Quote & instance_data,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t unregister_instance_w_timestamp(
            const Quote & instance_data,
            ::DDS::InstanceHandle_t handle,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t write(
            const Quote & instance_data,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t write_w_timestamp(
            const Quote & instance_data,
            ::DDS::InstanceHandle_t handle,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t dispose(
            const Quote & instance_data,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t dispose_w_timestamp(
            const Quote & instance_data,
            ::DDS::InstanceHandle_t handle,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t writedispose(
            const Quote & instance_data,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t writedispose_w_timestamp(
            const Quote & instance_data,
            ::DDS::InstanceHandle_t handle,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t get_key_value(
            Quote & key_holder,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        QuoteDataWriter_impl (
            gapi_dataWriter handle
        );
    
        virtual ~QuoteDataWriter_impl (void);
    
    private:
        QuoteDataWriter_impl(const QuoteDataWriter_impl &);
        void operator= (const QuoteDataWriter &);
    };
    
    class DDS_API QuoteDataReader_impl : public virtual QuoteDataReader,
                                        public ::DDS::DataReader_impl
    {
    public:
        virtual ::DDS::ReturnCode_t read(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t take(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t read_w_condition(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t take_w_condition(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t read_next_sample(
            Quote & received_data,
            ::DDS::SampleInfo & sample_info) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t take_next_sample(
            Quote & received_data,
            ::DDS::SampleInfo & sample_info) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t read_instance(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t take_instance(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t read_next_instance(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t take_next_instance(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t read_next_instance_w_condition(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t take_next_instance_w_condition(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t return_loan(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t get_key_value(
            Quote & key_holder,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::InstanceHandle_t lookup_instance(
            const Quote & instance) THROW_ORB_EXCEPTIONS;
    
        QuoteDataReader_impl (
            gapi_dataReader handle
        );
    
        virtual ~QuoteDataReader_impl(void);
    
    private:
        QuoteDataReader_impl(const QuoteDataReader &);
        void operator= (const QuoteDataReader &);
    
        ::DDS::ReturnCode_t check_preconditions(
            QuoteSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples
        );
    };
    
    class DDS_API ExchangeEventTypeSupportFactory : public ::DDS::TypeSupportFactory_impl
    {
    public:
        ExchangeEventTypeSupportFactory() {};
        virtual ~ExchangeEventTypeSupportFactory() {};
    private:
        ::DDS::DataWriter_ptr 
        create_datawriter (gapi_dataWriter handle);
    
        ::DDS::DataReader_ptr 
        create_datareader (gapi_dataReader handle);
    };
    
    class DDS_API ExchangeEventTypeSupport : public virtual ExchangeEventTypeSupportInterface,
                                   public ::DDS::TypeSupport_impl
    {
    public:
        virtual ::DDS::ReturnCode_t register_type(
            ::DDS::DomainParticipant_ptr participant,
            const char * type_name) THROW_ORB_EXCEPTIONS;
    
        virtual char * get_type_name() THROW_ORB_EXCEPTIONS;    
    
        ExchangeEventTypeSupport (void);
        virtual ~ExchangeEventTypeSupport (void);
    
    private:
        ExchangeEventTypeSupport (const ExchangeEventTypeSupport &);
        void operator= (const ExchangeEventTypeSupport &);
    
        static const char *metaDescriptor;
    };
    
    typedef ExchangeEventTypeSupportInterface_var ExchangeEventTypeSupport_var;
    typedef ExchangeEventTypeSupportInterface_ptr ExchangeEventTypeSupport_ptr;
    
    class DDS_API ExchangeEventDataWriter_impl : public virtual ExchangeEventDataWriter,
                                        public ::DDS::DataWriter_impl
    {
    public:
    
        virtual ::DDS::InstanceHandle_t register_instance(
            const ExchangeEvent & instance_data) THROW_ORB_EXCEPTIONS;
            
        virtual ::DDS::InstanceHandle_t register_instance_w_timestamp(
            const ExchangeEvent & instance_data,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
            
        virtual ::DDS::ReturnCode_t unregister_instance(
            const ExchangeEvent & instance_data,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t unregister_instance_w_timestamp(
            const ExchangeEvent & instance_data,
            ::DDS::InstanceHandle_t handle,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t write(
            const ExchangeEvent & instance_data,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t write_w_timestamp(
            const ExchangeEvent & instance_data,
            ::DDS::InstanceHandle_t handle,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t dispose(
            const ExchangeEvent & instance_data,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t dispose_w_timestamp(
            const ExchangeEvent & instance_data,
            ::DDS::InstanceHandle_t handle,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t writedispose(
            const ExchangeEvent & instance_data,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t writedispose_w_timestamp(
            const ExchangeEvent & instance_data,
            ::DDS::InstanceHandle_t handle,
            const ::DDS::Time_t & source_timestamp) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t get_key_value(
            ExchangeEvent & key_holder,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
    
        ExchangeEventDataWriter_impl (
            gapi_dataWriter handle
        );
    
        virtual ~ExchangeEventDataWriter_impl (void);
    
    private:
        ExchangeEventDataWriter_impl(const ExchangeEventDataWriter_impl &);
        void operator= (const ExchangeEventDataWriter &);
    };
    
    class DDS_API ExchangeEventDataReader_impl : public virtual ExchangeEventDataReader,
                                        public ::DDS::DataReader_impl
    {
    public:
        virtual ::DDS::ReturnCode_t read(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t take(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t read_w_condition(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t take_w_condition(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t read_next_sample(
            ExchangeEvent & received_data,
            ::DDS::SampleInfo & sample_info) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t take_next_sample(
            ExchangeEvent & received_data,
            ::DDS::SampleInfo & sample_info) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t read_instance(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t take_instance(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t read_next_instance(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t take_next_instance(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::SampleStateMask sample_states,
            ::DDS::ViewStateMask view_states,
            ::DDS::InstanceStateMask instance_states) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t read_next_instance_w_condition(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t take_next_instance_w_condition(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples,
            ::DDS::InstanceHandle_t a_handle,
            ::DDS::ReadCondition_ptr a_condition) THROW_ORB_EXCEPTIONS;
    
        virtual ::DDS::ReturnCode_t return_loan(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::ReturnCode_t get_key_value(
            ExchangeEvent & key_holder,
            ::DDS::InstanceHandle_t handle) THROW_ORB_EXCEPTIONS;
        
        virtual ::DDS::InstanceHandle_t lookup_instance(
            const ExchangeEvent & instance) THROW_ORB_EXCEPTIONS;
    
        ExchangeEventDataReader_impl (
            gapi_dataReader handle
        );
    
        virtual ~ExchangeEventDataReader_impl(void);
    
    private:
        ExchangeEventDataReader_impl(const ExchangeEventDataReader &);
        void operator= (const ExchangeEventDataReader &);
    
        ::DDS::ReturnCode_t check_preconditions(
            ExchangeEventSeq & received_data,
            ::DDS::SampleInfoSeq & info_seq,
            CORBA::Long max_samples
        );
    };
    
}

#endif
