/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import DDS.*;
import OpenDDS.DCPS.*;
import OpenDDS.DCPS.transport.*;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import org.omg.CORBA.StringSeqHolder;

import Complex_Idl.*;

/**
 * @author  Steven Stallion
 */
public class ComplexIDLTest extends QuoteSupport {
    private static final int DOMAIN_ID = 42;

    private static DomainParticipantFactory dpf;
    private static DomainParticipant participant;

    private static Topic topic;

    private static Publisher publisher;
    private static Subscriber subscriber;

    protected static void setUp(String[] args) {
        dpf = TheParticipantFactory.WithArgs(new StringSeqHolder(args));
        assert (dpf != null);

        participant = dpf.create_participant(DOMAIN_ID, PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        assert (participant != null);

        DataTypeSupport typeSupport = new DataTypeSupportImpl();

        int result = typeSupport.register_type(participant, "Complex::Data");
        assert (result != RETCODE_ERROR.value);

        topic = participant.create_topic("Complex::Topic", "Complex::Data",
                                         TOPIC_QOS_DEFAULT.get(), null,
                                         DEFAULT_STATUS_MASK.value);
        assert (topic != null);

        publisher = participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        assert (publisher != null);

        // Use the API to configure the transport
        TransportConfig transport_config =
            TheTransportRegistry.create_config("Config");
        assert (transport_config != null);

        TransportInst transport_inst =
            TheTransportRegistry.create_inst("myctp",
                                             TheTransportRegistry.TRANSPORT_TCP);
        assert (transport_inst != null);
        transport_config.addLast(transport_inst);

        TheTransportRegistry.bind_config(transport_config, publisher);

        subscriber = participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);
        assert (subscriber != null);

        TheTransportRegistry.bind_config(transport_config, subscriber);
    }

    protected static void testQuotes() throws Exception {
        System.out.println("And now for something completely different...");

        final AtomicInteger count = new AtomicInteger();

        final Lock lock = new ReentrantLock();
        final Condition finished = lock.newCondition();

        publisher.create_datawriter(topic, DATAWRITER_QOS_DEFAULT.get(),
            new DDS._DataWriterListenerLocalBase() {
                public void on_liveliness_lost(DataWriter dw, LivelinessLostStatus status) {}

                public void on_offered_deadline_missed(DataWriter dw, OfferedDeadlineMissedStatus status) {}

                public void on_offered_incompatible_qos(DataWriter dw, OfferedIncompatibleQosStatus status) {}

                public void on_publication_matched(DataWriter dw, PublicationMatchedStatus status) {
                    try {
                        if (status.current_count == 0) return;
                        // Don't run the rest of this method if the callback is
                        // due to the datareader going away.

                        DataDataWriter writer = DataDataWriterHelper.narrow(dw);

                        //NOTE: Since we are testing a complex type which contains a
                        //      union, both variants (DATA_IDL, DATA_STREAM) must be
                        //      tested on the same set of data:

                        List<Data> dataItems = new ArrayList<Data>();

                        for (Quote quote : quotes) {
                            // DATA_IDL
                            dataItems.add(createData(quote));

                            // DATA_STREAM
                            ByteArrayOutputStream out = new ByteArrayOutputStream();

                            ObjectOutputStream os = new ObjectOutputStream(out);
                            os.writeObject(quote.line); // Quote is not Serializable

                            dataItems.add(createData(out.toByteArray()));
                        }

                        count.set(dataItems.size());

                        for (Data data : dataItems) {
                            int result = writer.write(data, HANDLE_NIL.value);
                            assert (result != RETCODE_ERROR.value);
                        }

                    } catch (Throwable t) {
                        t.printStackTrace();
                    }
                };
            }, DEFAULT_STATUS_MASK.value
        );

        lock.lock();
        try {
            subscriber.create_datareader(topic, DATAREADER_QOS_DEFAULT.get(),
                new DDS._DataReaderListenerLocalBase() {
                    public void on_liveliness_changed(DataReader dr, LivelinessChangedStatus status) {}

                    public void on_requested_deadline_missed(DataReader dr, RequestedDeadlineMissedStatus status) {}

                    public void on_requested_incompatible_qos(DataReader dr, RequestedIncompatibleQosStatus status) {}

                    public void on_sample_lost(DataReader dr, SampleLostStatus status) {}

                    public void on_sample_rejected(DataReader dr, SampleRejectedStatus status) {}

                    public void on_subscription_matched(DataReader dr, SubscriptionMatchedStatus status) {}

                    public void on_data_available(DataReader dr) {
                        try {
                            DataDataReader reader = DataDataReaderHelper.narrow(dr);

                            DataHolder dh = new DataHolder(createDefaultData());

                            SampleInfo si = new SampleInfo();
                            si.source_timestamp = new Time_t();

                            // Verify idl2jni is "optimizing" object copies
                            CastMember cast_pre_take = dh.value.payload.idl_quote().cast_member;

                            int result = reader.take_next_sample(dh, new SampleInfoHolder(si));
                            assert (result != RETCODE_ERROR.value);

                            Data data = dh.value;

                            if (si.valid_data) {
                                switch (data.payload.discriminator().value()) {
                                    case DataType._DATA_IDL:
                                        assert data.payload.idl_quote().cast_member == cast_pre_take;
                                        printQuote(data.payload.idl_quote());
                                        break;

                                    case DataType._DATA_STREAM:
                                        ByteArrayInputStream in =
                                            new ByteArrayInputStream(data.payload.stream());

                                        ObjectInputStream os = new ObjectInputStream(in);
                                        Object obj = os.readObject();

                                        assert (obj instanceof String);
                                }
                            }

                            if (count.decrementAndGet() == 0) {
                                // Signal main thread
                                lock.lock();
                                try {
                                    finished.signalAll();
                                } finally {
                                    lock.unlock();
                                }
                            }

                        } catch (Throwable t) {
                            t.printStackTrace();
                        }
                    }
                }, DEFAULT_STATUS_MASK.value
            );

            // Wait for DataReader
            finished.await();

        } finally {
            lock.unlock();
        }

        System.out.println("(Those responsible have been sacked.)");
    }

    public static void main(String[] args) throws Exception {
        setUp(args);
        try {
            testQuotes();

        } finally {
            tearDown();
        }
    }

    protected static void tearDown() {
        participant.delete_contained_entities();
        dpf.delete_participant(participant);

        TheServiceParticipant.shutdown();
    }

    //

    private static Data createData(byte[] bytes) {
        Data data = new Data();
        data.payload = new DataUnion();

        data.payload.stream(bytes);

        return data;
    }

    private static Data createData(Quote quote) {
        Data data = new Data();
        data.payload = new DataUnion();

        data.payload.idl_quote(quote);

        return data;
    }

    private static Data createDefaultData() {
        Quote quote = new Quote();
        quote.cast_member = new CastMember();

        return createData(quote);
    }
}
