/*
 * $Id$
 */

import java.util.Stack;

import org.omg.CORBA.StringSeqHolder;

import DDS.DomainParticipant;
import DDS.DomainParticipantFactory;
import DDS.PARTICIPANT_QOS_DEFAULT;
import OpenDDS.DCPS.TheParticipantFactory;
import OpenDDS.DCPS.TheServiceParticipant;
import OpenDDS.DCPS.transport.TheTransportFactory;
import OpenDDS.DCPS.transport.TransportImpl;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class MultiRepoBase {
    public static final int DOMAIN1_ID = 42;
    public static final int DOMAIN2_ID = 64;
    
    private static DomainParticipantFactory dpf;
    private static TransportImpl transport;
    
    private static Stack<DomainParticipant> participants =
        new Stack<DomainParticipant>();

    protected static void init(String[] args) throws Exception {
        dpf = TheParticipantFactory.WithArgs(new StringSeqHolder(args));
        
        TheServiceParticipant.set_repo_domain(DOMAIN1_ID, 1);
        TheServiceParticipant.set_repo_ior("file://repo1.ior", 1);

        TheServiceParticipant.set_repo_domain(DOMAIN2_ID, 2);
        TheServiceParticipant.set_repo_ior("file://repo2.ior", 2);
        
        transport = TheTransportFactory.create_transport_impl(1, TheTransportFactory.AUTO_CONFIG);
        
        assert (dpf != null);
        assert (transport != null);
    }
    
    protected static DomainParticipant createParticipant(int domainId) {
        return participants.push(dpf.create_participant(domainId, PARTICIPANT_QOS_DEFAULT.get(), null));
    }
    
    protected static MultiRepoWorker createWorker(int domainId) {
        return new MultiRepoWorker(createParticipant(domainId), transport);
    }
    
    protected static void fini() {
    //TODO: Destroying participants cause the JVM to segfault

//        while (!participants.isEmpty()) {
//            DomainParticipant participant = participants.pop();
//
//            participant.delete_contained_entities();
//            dpf.delete_participant(participant);
//        }
        
        TheTransportFactory.release();
        TheServiceParticipant.shutdown();
    }
}
