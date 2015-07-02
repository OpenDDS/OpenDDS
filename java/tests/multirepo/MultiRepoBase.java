/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

import java.util.Stack;

import org.omg.CORBA.StringSeqHolder;

import DDS.DomainParticipant;
import DDS.DomainParticipantFactory;
import DDS.PARTICIPANT_QOS_DEFAULT;
import OpenDDS.DCPS.DEFAULT_STATUS_MASK;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.DCPS.TheServiceParticipant;

/**
 * @author  Steven Stallion
 */
public class MultiRepoBase {
    public static final int DOMAIN1_ID = 42;
    public static final int DOMAIN2_ID = 64;

    private static DomainParticipantFactory dpf;

    private static Stack<DomainParticipant> participants =
        new Stack<DomainParticipant>();

    protected static void setUp(String[] args) {
        dpf = TheParticipantFactory.WithArgs(new StringSeqHolder(args));

        TheServiceParticipant.set_repo_domain(DOMAIN1_ID, "1");
        TheServiceParticipant.set_repo_ior("file://repo1.ior", "1");

        TheServiceParticipant.set_repo_domain(DOMAIN2_ID, "2");
        TheServiceParticipant.set_repo_ior("file://repo2.ior", "2");

        assert (dpf != null);
    }

    private static DomainParticipant createParticipant(int domainId) {
        return participants.push(dpf.create_participant(domainId, PARTICIPANT_QOS_DEFAULT.get(), null, DEFAULT_STATUS_MASK.value));
    }

    protected static MultiRepoWorker createWorker(int domainId) {
        return new MultiRepoWorker(createParticipant(domainId));
    }

    protected static void tearDown() {
        while (!participants.isEmpty()) {
            DomainParticipant participant = participants.pop();

            participant.delete_contained_entities();
            dpf.delete_participant(participant);
        }

        TheServiceParticipant.shutdown();
    }
}
