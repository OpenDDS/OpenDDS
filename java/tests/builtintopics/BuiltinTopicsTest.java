/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import DDS.ANY_INSTANCE_STATE;
import DDS.ANY_SAMPLE_STATE;
import DDS.ANY_VIEW_STATE;
import DDS.DATAREADER_QOS_DEFAULT;
import DDS.DATAWRITER_QOS_DEFAULT;
import DDS.DataReader;
import DDS.DomainParticipant;
import DDS.DomainParticipantFactory;
import DDS.PARTICIPANT_QOS_DEFAULT;
import DDS.PUBLISHER_QOS_DEFAULT;
import DDS.ParticipantBuiltinTopicData;
import DDS.ParticipantBuiltinTopicDataDataReader;
import DDS.ParticipantBuiltinTopicDataDataReaderHelper;
import DDS.ParticipantBuiltinTopicDataSeqHolder;
import DDS.PublicationBuiltinTopicData;
import DDS.PublicationBuiltinTopicDataDataReader;
import DDS.PublicationBuiltinTopicDataDataReaderHelper;
import DDS.PublicationBuiltinTopicDataSeqHolder;
import DDS.Publisher;
import DDS.RETCODE_OK;
import DDS.SUBSCRIBER_QOS_DEFAULT;
import DDS.SampleInfo;
import DDS.SampleInfoSeqHolder;
import DDS.Subscriber;
import DDS.SubscriptionBuiltinTopicData;
import DDS.SubscriptionBuiltinTopicDataDataReader;
import DDS.SubscriptionBuiltinTopicDataDataReaderHelper;
import DDS.SubscriptionBuiltinTopicDataSeqHolder;
import DDS.TOPIC_QOS_DEFAULT;
import DDS.Topic;
import DDS.TopicBuiltinTopicData;
import DDS.TopicBuiltinTopicDataDataReader;
import DDS.TopicBuiltinTopicDataDataReaderHelper;
import DDS.TopicBuiltinTopicDataSeqHolder;
import OpenDDS.DCPS.BuiltinTopicUtils;
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.DCPS.TheServiceParticipant;

import org.omg.CORBA.StringSeqHolder;

import Messenger.MessageTypeSupport;
import Messenger.MessageTypeSupportImpl;

/**
 * @author  Steven Stallion
 */
public class BuiltinTopicsTest {
    private static final int DOMAIN_ID = 42;

    private static DomainParticipantFactory dpf;
    private static DomainParticipant participant;
    private static Subscriber builtinSubscriber;
    private static Topic topic;

    protected static void setUp(String[] args) {
        dpf = TheParticipantFactory.WithArgs(new StringSeqHolder(args));

        participant =
            dpf.create_participant(DOMAIN_ID, PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

        builtinSubscriber = participant.get_builtin_subscriber();

        MessageTypeSupport typeSupport = new MessageTypeSupportImpl();
        if (typeSupport.register_type(participant, "Messenger::Message") != RETCODE_OK.value) {
            throw new IllegalStateException("Unable to register type!");
        }

        topic = participant.create_topic("BuiltinTopics::Topic", typeSupport.get_type_name(),
                                         TOPIC_QOS_DEFAULT.get(), null, 0);
    }

    protected static void testParticipantBIT() throws Exception {
        //NOTE: We do not need to do anything special to verify that the
        //      Participant BIT is functioning (a DomainParticipant already exists).

        Thread.sleep(2500); // Wait for repo to settle

        DataReader dr =
            builtinSubscriber.lookup_datareader(BuiltinTopicUtils.BUILT_IN_PARTICIPANT_TOPIC);

        ParticipantBuiltinTopicDataDataReader reader =
            ParticipantBuiltinTopicDataDataReaderHelper.narrow(dr);

        assert (reader != null);

        ParticipantBuiltinTopicDataSeqHolder data =
            new ParticipantBuiltinTopicDataSeqHolder(new ParticipantBuiltinTopicData[0]);

        SampleInfoSeqHolder info = new SampleInfoSeqHolder(new SampleInfo[0]);

        reader.read(data, info, 1, ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);

        assert (data.value.length > 0);
    }

    protected static void testTopicBIT() throws Exception {
        //NOTE: We do not need to do anything special to verify that the
        //      Topic BIT is functioning (a Topic already exists).

        Thread.sleep(2500); // Wait for repo to settle

        DataReader dr =
            builtinSubscriber.lookup_datareader(BuiltinTopicUtils.BUILT_IN_TOPIC_TOPIC);

        TopicBuiltinTopicDataDataReader reader =
            TopicBuiltinTopicDataDataReaderHelper.narrow(dr);

        assert (reader != null);

        TopicBuiltinTopicDataSeqHolder data =
            new TopicBuiltinTopicDataSeqHolder(new TopicBuiltinTopicData[0]);

        SampleInfoSeqHolder info = new SampleInfoSeqHolder(new SampleInfo[0]);

        reader.read(data, info, 1, ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);

        assert (data.value.length > 0);
    }

    protected static void testSubscriptionBIT() throws Exception {
        //NOTE: We must first create a DataReader to verify that the Subscription BIT
        //      is functioning (no Subscriptions exist).

        Subscriber subscriber =
            participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null, 0);

        subscriber.create_datareader(topic, DATAREADER_QOS_DEFAULT.get(), null, 0);

        Thread.sleep(2500); // Wait for repo to settle

        //

        DataReader dr =
            builtinSubscriber.lookup_datareader(BuiltinTopicUtils.BUILT_IN_SUBSCRIPTION_TOPIC);

        SubscriptionBuiltinTopicDataDataReader reader =
            SubscriptionBuiltinTopicDataDataReaderHelper.narrow(dr);

        assert (reader != null);

        SubscriptionBuiltinTopicDataSeqHolder data =
            new SubscriptionBuiltinTopicDataSeqHolder(new SubscriptionBuiltinTopicData[0]);

        SampleInfoSeqHolder info = new SampleInfoSeqHolder(new SampleInfo[0]);

        reader.read(data, info, 1, ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);

        assert (data.value.length > 0);
    }

    protected static void testPublicationBIT() throws Exception {
        //NOTE: We must first create a DataWriter to verify that the Subscription BIT
        //      is functioning (no Subscriptions exist).

        Publisher publisher =
            participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null, 0);

        publisher.create_datawriter(topic, DATAWRITER_QOS_DEFAULT.get(), null, 0);

        Thread.sleep(2500); // Wait for repo to settle

        //

        DataReader dr =
            builtinSubscriber.lookup_datareader(BuiltinTopicUtils.BUILT_IN_PUBLICATION_TOPIC);

        PublicationBuiltinTopicDataDataReader reader =
            PublicationBuiltinTopicDataDataReaderHelper.narrow(dr);

        assert (reader != null);

        PublicationBuiltinTopicDataSeqHolder data =
            new PublicationBuiltinTopicDataSeqHolder(new PublicationBuiltinTopicData[0]);

        SampleInfoSeqHolder info = new SampleInfoSeqHolder(new SampleInfo[0]);

        reader.read(data, info, 1, ANY_SAMPLE_STATE.value, ANY_VIEW_STATE.value, ANY_INSTANCE_STATE.value);

        assert (data.value.length > 0);
    }

    public static void main(String[] args) throws Exception {
        setUp(args);
        try {
            testParticipantBIT();
            testTopicBIT();
            testSubscriptionBIT();
            testPublicationBIT();

        } finally {
            tearDown();
        }
    }

    protected static void tearDown() {
        participant.delete_contained_entities();
        dpf.delete_participant(participant);

        TheServiceParticipant.shutdown();
    }
}
