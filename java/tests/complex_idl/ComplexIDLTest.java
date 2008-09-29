/*
 * $Id$
 */

import DDS.*;
import OpenDDS.DCPS.*;
import OpenDDS.DCPS.transport.*;

import Complex.*;

import org.omg.CORBA.StringSeqHolder;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ComplexIDLTest extends QuoteSupport {
    private static final int DOMAIN_ID = 42;
    
    private static DomainParticipantFactory dpf;
    private static DomainParticipant participant;

    private static Topic topic;
    
    private static Publisher publisher;
    private static Subscriber subscriber;
    
    protected static void setUp(String[] args) throws Exception {
        dpf = TheParticipantFactory.WithArgs(new StringSeqHolder(args));
        
        participant = dpf.create_participant(DOMAIN_ID, PARTICIPANT_QOS_DEFAULT.get(), null);
        
        DataTypeSupport typeSupport = new DataTypeSupportImpl();
        assert (typeSupport.register_type(participant, "Complex::Data") == RETCODE_OK.value);

        topic = participant.create_topic("Complex::Topic", typeSupport.get_type_name(),
                                         TOPIC_QOS_DEFAULT.get(), null);

        publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null);
        
        TransportImpl transport1 = 
            TheTransportFactory.create_transport_impl(1, TheTransportFactory.AUTO_CONFIG);
        assert (transport1.attach_to_publisher(publisher).value() == AttachStatus._ATTACH_OK);
        
        subscriber = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null);
        
        TransportImpl transport2 =
            TheTransportFactory.create_transport_impl(2, TheTransportFactory.AUTO_CONFIG);
        assert (transport2.attach_to_subscriber(subscriber).value() == AttachStatus._ATTACH_OK);
    }
    
    protected static void testIDLQuote(final Quote quote) throws Exception {
        final Object lock = new Object();
        
        publisher.create_datawriter(topic, DATAWRITER_QOS_DEFAULT.get(),
            new DDS._DataWriterListenerLocalBase() {
                public void on_liveliness_lost(DataWriter dw, LivelinessLostStatus status) {}

                public void on_offered_deadline_missed(DataWriter dw, OfferedDeadlineMissedStatus status) {}

                public void on_offered_incompatible_qos(DataWriter dw, OfferedIncompatibleQosStatus status) {}

                public void on_publication_match(DataWriter dw, PublicationMatchStatus status) {
                    assert (status.total_count > 0);

                    DataDataWriter writer = DataDataWriterHelper.narrow(dw);
                    
                    Data data = new Data();
                    
                    data.payload = new DataUnion();
                    data.payload.idl_quote(quote);
                    
                    assert (writer.write(data, HANDLE_NIL.value) != RETCODE_OK.value);
                }
            }
        );

        synchronized (lock) {
            subscriber.create_datareader(topic, DATAREADER_QOS_DEFAULT.get(),
                new DDS._DataReaderListenerLocalBase() {
                    public void on_liveliness_changed(DataReader dr, LivelinessChangedStatus status) {}

                    public void on_requested_deadline_missed(DataReader dr, RequestedDeadlineMissedStatus status) {}

                    public void on_requested_incompatible_qos(DataReader dr, RequestedIncompatibleQosStatus status) {}

                    public void on_sample_lost(DataReader dr, SampleLostStatus status) {}

                    public void on_sample_rejected(DataReader dr, SampleRejectedStatus status) {}

                    public void on_subscription_match(DataReader dr, SubscriptionMatchStatus status) {}

                    public void on_data_available(DataReader dr) {
                        DataDataReader reader = DataDataReaderHelper.narrow(dr);

                        Data data = createDefaultData();

                        SampleInfo si = new SampleInfo();
                        si.source_timestamp = new Time_t();

                        assert (reader.take_next_sample(new DataHolder(data), new SampleInfoHolder(si)) == RETCODE_OK.value);

                        printQuote(data.payload.idl_quote());

                        // Notify main thread
                        synchronized (lock) {
                            lock.notifyAll();
                        }
                    }
                }
            );
        
            lock.wait();
        }
    }

    public static void main(String[] args) throws Exception {
        setUp(args);
        try {
            for (Quote quote : quotes) {
                testIDLQuote(quote);
                break;
            }
            
        } finally {
            tearDown();
        }
    }

    protected static void tearDown() {
        participant.delete_contained_entities();
        dpf.delete_participant(participant);
        
        TheTransportFactory.release();
        TheServiceParticipant.shutdown();
        
        System.out.println("(Those responsible have been sacked.)");
    }
    
    //
    
    private static Data createDefaultData() {
        Data data = new Data();
        data.payload = new DataUnion();
                    
        Quote quote = new Quote();
        quote.cast_member = new CastMember();
                    
        data.payload.idl_quote(quote);
        data.payload.stream(new byte[0]);

        return data;
    }
}