/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import org.junit.Test;

/**
 * @author  Steven Stallion
 */
public class DCPSInfoRepoTest {

    @Test(expected = NullPointerException.class)
    public void errorWithNullArgs() {
        new DCPSInfoRepo(null);
    }

    @Test(expected = NullPointerException.class)
    public void errorWithNullArg() {
        new DCPSInfoRepo(new String[] { "1", "2", null });
    }

    @Test(expected = IllegalStateException.class)
    public void errorRunAfterFini() {
        DCPSInfoRepo repo =
            new DCPSInfoRepo(new String[] { "-NOBITS" });

        repo.fini();
        repo.run();
    }

    @Test(expected = IllegalStateException.class)
    public void errorShutdownAfterFini() {
        DCPSInfoRepo repo =
            new DCPSInfoRepo(new String[] { "-NOBITS" });

        repo.fini();
        repo.shutdown();
    }

    @Test(expected = IllegalArgumentException.class)
    public void testUsage() {
        new DCPSInfoRepo(new String[] { "-?" });
    }

    @Test
    public void testRunWithShutdown() throws Exception {
        DCPSInfoRepo repo =
            new DCPSInfoRepo(new String[] { "-NOBITS" });

        Thread t = new Thread(repo);
        t.start();

        repo.shutdown();
        t.join();
    }
}
