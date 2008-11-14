/*
 * $Id$
 */

package org.opendds.jms;

import java.util.ArrayList;
import java.util.List;

import javax.jms.Connection;
import javax.jms.ConnectionConsumer;
import javax.jms.ConnectionMetaData;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.ServerSessionPool;
import javax.jms.Session;
import javax.jms.Topic;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ConnectionImpl implements Connection {
    private List<TemporaryTopicImpl> temporaryTopics;

    public ConnectionImpl() {
        this.temporaryTopics = new ArrayList<TemporaryTopicImpl>();
    }

    public String getClientID() throws JMSException {
        return null;
    }

    public void setClientID(String clientID) throws JMSException {
    }

    public ExceptionListener getExceptionListener() throws JMSException {
        return null;
    }

    public void setExceptionListener(ExceptionListener listener) throws JMSException {
    }

    public ConnectionMetaData getMetaData() throws JMSException {
        return null;
    }

    public void start() throws JMSException {
    }

    public ConnectionConsumer createConnectionConsumer(Destination destination,
                                                       String messageSelector,
                                                       ServerSessionPool sessionPool,
                                                       int maxMessages) throws JMSException {
        throw new UnsupportedOperationException();
    }

    public ConnectionConsumer createDurableConnectionConsumer(Topic topic,
                                                              String subscriptionName,
                                                              String messageSelector,
                                                              ServerSessionPool sessionPool,
                                                              int maxMessages) throws JMSException {
        throw new UnsupportedOperationException();
    }

    public Session createSession(boolean transacted, int acknowledgeMode) throws JMSException {
        return null;
    }

    public void stop() throws JMSException {
    }

    public void close() throws JMSException {
    }

    public void addTemporaryTopic(TemporaryTopicImpl temporaryTopic) {
        this.temporaryTopics.add(temporaryTopic);
    }

    public void removeTemporaryTopic(TemporaryTopicImpl temporaryTopic) {
        this.temporaryTopics.remove(temporaryTopic);
    }

    /* Below are helpers to get the PARTITION QoS according to this scheme:

       - The OpenDDS JMS engine will not expose PARTITION QoS as
         user-configurable, it will be reserved for the implementation.

       - The OpenDDS JMS engine will use a unique ID for each Connection object
         -- we will call these ConnectionIDs.  ConnectionIDs are strings
         that are always 16 characters long.  Each character is a hex digit
         ('0'..'9','a'..'f').  They are unique for each DomainParticipant and
         obtained using an OpenDDS-specific method.

       - Each DDS Publisher (owned by the JMS Connection) will have a PARTITION
         list with one entry: its ConnectionID.

       - Each JMS Connection will have two DDS Subscribers: I'll call them
         LocalOK and NoLocal.  (These can be created on-demand instead of
         eagerly.)  The Connection will determine which Subscriber to use when
         creating DataReaders based on the JMS user's setting of the "noLocal"
         flag:
         - A LocalOK Subscriber will have a PARTITION list with one entry: "*"
         - A NoLocal Subscriber will have a PARTITION list with 16 entries,
           which is the logical negation of its connection ID.
           Each entry follows this pattern (shown here for length == 3):
             ConnectionID = "ABC" PARTITION = "[!A]??", "?[!B]?", "??[!C]"
    */

    private static DDS.PartitionQosPolicy makePublisherPartition(DDS.DomainParticipant dp) {
        return new DDS.PartitionQosPolicy(new String[]{getConnectionId(dp)});
    }

    private static DDS.PartitionQosPolicy makeSubscriberPartition(DDS.DomainParticipant dp, boolean noLocal) {
        if (!noLocal) return new DDS.PartitionQosPolicy(new String[]{"*"});

        String cid = getConnectionId(dp);
        final int N = 16;
        assert cid.length() == N; //TODO: assert?
        String[] parts = new String[N];
        String question = "???????????????";
        for (int i = 0; i < N; ++i) {
            StringBuilder sb = new StringBuilder(N + 3);
            sb.append(question, 0, i) // i question marks
                .append("[!").append(cid.charAt(i)).append("]") // [!c]
                .append(question, 0, N - 1 - i); // 15 - i question marks
            parts[i] = sb.toString();
        }
        return new DDS.PartitionQosPolicy(parts);
    }

    private static String getConnectionId(DDS.DomainParticipant dp) {
        OpenDDS.DCPS.DomainParticipantExt ext =
            OpenDDS.DCPS.DomainParticipantExtHelper.narrow(dp);
        assert ext != null; //TODO: assert?
        return String.format("%08x%08x", ext.get_federation_id(),
                             ext.get_participant_id());
    }

    //TODO: comment this out or move it to a real JUnit test
    public static void main(String[] args) throws Exception {
        DDS.DomainParticipantFactory dpf =
            OpenDDS.DCPS.TheParticipantFactory.WithArgs(new org.omg.CORBA.StringSeqHolder(args));
        DDS.DomainParticipant dp =
            dpf.create_participant(411,
                                   DDS.PARTICIPANT_QOS_DEFAULT.get(), null);

        System.out.println("CID = {" + getConnectionId(dp) + "}");
        DDS.PartitionQosPolicy part = makeSubscriberPartition(dp, true);
        System.out.println("noLocal subscriber: " + part.name.length);
        for (int i = 0; i < part.name.length; ++i) {
            System.out.println("\t{" + part.name[i] + "}");
        }
    }
}
