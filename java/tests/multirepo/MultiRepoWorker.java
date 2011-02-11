/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import DDS.DATAREADER_QOS_DEFAULT;
import DDS.DATAWRITER_QOS_DEFAULT;
import DDS.DataReader;
import DDS.DataWriter;
import DDS.DomainParticipant;
import DDS.LivelinessChangedStatus;
import DDS.LivelinessLostStatus;
import DDS.OfferedDeadlineMissedStatus;
import DDS.OfferedIncompatibleQosStatus;
import DDS.PUBLISHER_QOS_DEFAULT;
import DDS.PublicationMatchedStatus;
import DDS.Publisher;
import DDS.RETCODE_OK;
import DDS.RequestedDeadlineMissedStatus;
import DDS.RequestedIncompatibleQosStatus;
import DDS.SUBSCRIBER_QOS_DEFAULT;
import DDS.SampleInfo;
import DDS.SampleInfoHolder;
import DDS.SampleLostStatus;
import DDS.SampleRejectedStatus;
import DDS.Subscriber;
import DDS.SubscriptionMatchedStatus;
import DDS.TOPIC_QOS_DEFAULT;
import DDS.Topic;
import DDS._DataReaderListenerLocalBase;
import DDS._DataWriterListenerLocalBase;
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;
import OpenDDS.DCPS.transport.AttachStatus;
import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportImpl;

import MultiRepo.Message;
import MultiRepo.MessageDataReader;
import MultiRepo.MessageDataReaderHelper;
import MultiRepo.MessageDataWriter;
import MultiRepo.MessageDataWriterHelper;
import MultiRepo.MessageHolder;
import MultiRepo.MessageTypeSupport;
import MultiRepo.MessageTypeSupportImpl;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class MultiRepoWorker {
    private static volatile int transportId;

    private DomainParticipant participant;

    private Topic topic;
    private TransportImpl transport;

    private boolean read;

    public MultiRepoWorker(DomainParticipant participant) {
        assert (participant != null);

        this.participant = participant;

        MessageTypeSupport typeSupport = new MessageTypeSupportImpl();
        if (typeSupport.register_type(participant, "MultiRepo::Message") != RETCODE_OK.value) {
            throw new IllegalStateException("Unable to register type!");
        }

        topic = participant.create_topic("MultiRepo::Topic", typeSupport.get_type_name(),
                                         TOPIC_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

        transport = TheTransportFactory.create_transport_impl(++transportId, TheTransportFactory.AUTO_CONFIG);

        assert (topic != null);
        assert (transport != null);
    }

    public DomainParticipant getParticipant() {
        return participant;
    }

    public void write(final String text) {
        Publisher publisher =
            participant.create_publisher(PUBLISHER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

        assert (publisher != null);

        AttachStatus status = transport.attach_to_publisher(publisher);
        if(status.value() != AttachStatus._ATTACH_OK) {
            throw new IllegalStateException("Unable to attach publisher to transport!");
        }

        publisher.create_datawriter(topic, DATAWRITER_QOS_DEFAULT.get(),
            new _DataWriterListenerLocalBase() {
                public void on_liveliness_lost(DataWriter dw, LivelinessLostStatus status) {}

                public void on_offered_deadline_missed(DataWriter dw, OfferedDeadlineMissedStatus status) {}

                public void on_offered_incompatible_qos(DataWriter dw, OfferedIncompatibleQosStatus status) {}

                public void on_publication_matched(DataWriter dw, PublicationMatchedStatus status) {
                    if (status.total_count < 1) {
                        throw new IllegalArgumentException("Unable to match publication!");
                    }

                    MessageDataWriter writer = MessageDataWriterHelper.narrow(dw);

                    Message message = new Message();
                    message.text = text;

                    int handle = writer.register_instance(message);
                    if (writer.write(message, handle) != RETCODE_OK.value) {
                        throw new IllegalStateException("Unable to write message!");
                    }

                    System.out.printf("[%s] wrote %s\n", participant, message);
                }
            }, DEFAULT_STATUS_MASK.value
        );
    }

    public void read() {
        Subscriber subscriber =
            participant.create_subscriber(SUBSCRIBER_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value);

        assert (subscriber != null);

        AttachStatus status = transport.attach_to_subscriber(subscriber);
        if (status.value() != AttachStatus._ATTACH_OK) {
            throw new IllegalStateException("Unable to attach subscriber to transport!");
        }

        subscriber.create_datareader(topic, DATAREADER_QOS_DEFAULT.get(),
            new _DataReaderListenerLocalBase () {

                public void on_liveliness_changed(DataReader dr, LivelinessChangedStatus status) {}

                public void on_requested_deadline_missed(DataReader dr, RequestedDeadlineMissedStatus status) {}

                public void on_requested_incompatible_qos(DataReader dr, RequestedIncompatibleQosStatus status) {}

                public void on_sample_lost(DataReader reader, SampleLostStatus status) {}

                public void on_sample_rejected(DataReader dr, SampleRejectedStatus status) {}

                public void on_subscription_matched(DataReader dr, SubscriptionMatchedStatus status) {}

                public void on_data_available(DataReader dr) {
                    MessageDataReader reader = MessageDataReaderHelper.narrow(dr);

                    MessageHolder mh = new MessageHolder(new Message());

                    SampleInfo si = new SampleInfo();
                    si.source_timestamp = new DDS.Time_t();

                    SampleInfoHolder sih = new SampleInfoHolder(si);
                    reader.take_next_sample(mh, sih);

                    read = true;

                    if (si.valid_data) {
                        System.out.printf("[%s] read: \"%s\"\n", participant, mh.value.text);
                    }
                }
            }, DEFAULT_STATUS_MASK.value
        );
    }

    public boolean isRead() {
        return read;
    }
}
