/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

/**
 * @author  Steven Stallion
 */
public class MultiRepoMaster extends MultiRepoBase {

    public static void main(String[] args) throws Exception {
        setUp(args);
        try {
            // Master process initiates a write on DOMAIN1
            MultiRepoWorker writer = createWorker(DOMAIN1_ID);
            writer.write("Photography?");

            MultiRepoWorker reader = createWorker(DOMAIN2_ID);
            reader.read();

            Thread.sleep(5000);
            assert (reader.isRead());

        } finally {
            tearDown();
        }
    }
}
