/*
 * $Id$
 */

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class MultiRepoMaster extends MultiRepoBase {
    
    public static void main(String[] args) throws Exception {
        init(args);
        try {
            // Master process initiates a write on DOMAIN1
            MultiRepoWorker writer = createWorker(DOMAIN1_ID);
            writer.write("Photography?");
            
            MultiRepoWorker reader = createWorker(DOMAIN2_ID);
            reader.read();
            
            Thread.sleep(5000); // Wait for workers to finish
        
        } finally {
            fini();
        }
    }
}
