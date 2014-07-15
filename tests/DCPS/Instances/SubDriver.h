#ifndef SUBDRIVER_H
#define SUBDRIVER_H

#include "tests/Utils/DDSApp.h"
#include "tests/Utils/Options.h"
#include "tests/Utils/ListenerRecorder.h"
#include <model/Sync.h>
#include <vector>

template<typename TypeSupportImpl>
class SubDriver
{
  public:

    typedef typename TypeSupportImpl::message_type message_type;
    typedef typename TypeSupportImpl::datareader_var datareader_var;
    typedef typename TypeSupportImpl::datareader_type datareader_type;
    typedef typename TypeSupportImpl::datareaderimpl_type datareaderimpl_type;

    typedef std::vector < ::OpenDDS::DCPS::PublicationId > PublicationIds;

    SubDriver()
    : num_writes_ (0),
    receive_delay_msec_ (0),
    verbose_ (false)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) SubDriver::SubDriver \n")));
    }

    virtual ~SubDriver()
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) SubDriver::~SubDriver \n")));
    }

    void run(::TestUtils::DDSApp& ddsApp, ::TestUtils::Options& options)
    {
        parse_args(options);
        run(ddsApp);
    }

  private:

    void parse_args(::TestUtils::Options& options)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) SubDriver::parse_args \n")));
        num_writes_         = options.get<long>("num_writes");
        receive_delay_msec_ = options.get<long>("receive_delay_msec");
        verbose_            = options.get<bool>("verbose");
    }

    void run(::TestUtils::DDSApp& ddsApp)
    {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) SubDriver::run \n")));
        ::TestUtils::DDSTopicFacade< datareaderimpl_type> topic_facade = ddsApp.topic_facade< datareaderimpl_type> ("bar");

        // Create Listener
        typedef typename ::TestUtils::ListenerRecorder< message_type, datareader_type> ListenerRecorder;
        ListenerRecorder* listener_impl = new ListenerRecorder;

        listener_impl->verbose(verbose_);
        DDS::DataReaderListener_var listener(listener_impl);

        // Create data reader for the topic
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sub Creating Reader\n")));
        DDS::DataReader_var reader = topic_facade.reader(listener);

        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Sub waiting for writer to come and go\n")));
        {
            OpenDDS::Model::ReaderSync rs(reader);
        }

        const typename ListenerRecorder::Messages msgs = listener_impl->messages();
        if (msgs.empty()) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ERROR: no messages received\n")));
        }

    }

    long               num_writes_;
    long               receive_delay_msec_;
    bool               verbose_;

};

#endif
