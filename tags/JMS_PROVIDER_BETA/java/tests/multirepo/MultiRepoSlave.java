/*
 * $Id$
 */

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class MultiRepoSlave extends MultiRepoBase {

    public static void main(String[] args) throws Exception {
        setUp(args);
        try {
            // Slave process responds to a write on DOMAIN1
            MultiRepoWorker reader = createWorker(DOMAIN1_ID);
            reader.read();
            
            MultiRepoWorker writer = createWorker(DOMAIN2_ID);
            writer.write("Snap snap, grin grin, wink wink, nudge nudge, say no more?");
            
            Thread.sleep(5000);
            assert (reader.isRead());
        
        } finally {
            tearDown();
        }
    }
}
