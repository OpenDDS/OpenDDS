/*
 * $Id$
 */

package org.opendds.inforepo;

import org.junit.Test;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DCPSInfoRepoServiceTest {

    @Test(expected = NullPointerException.class)
    public void errorWithNullArgs() {
        new DCPSInfoRepoService(null);
    }

    @Test(expected = NullPointerException.class)
    public void errorWithNullArg() {
        new DCPSInfoRepoService(new String[] { "1", "2", null });
    }

    @Test(expected = IllegalStateException.class)
    public void errorRunAfterFini() {
        DCPSInfoRepoService service =
            new DCPSInfoRepoService(new String[] { "-NOBITS" });

        service.fini();
        service.run();
    }

    @Test(expected = IllegalStateException.class)
    public void errorShutdownAfterFini() {
        DCPSInfoRepoService service =
            new DCPSInfoRepoService(new String[] { "-NOBITS" });

        service.fini();
        service.shutdown();
    }

    @Test(expected = IllegalArgumentException.class)
    public void testUsage() {
        new DCPSInfoRepoService(new String[] { "-?" });
    }

    @Test
    public void testRunWithShutdown() throws Exception {
        DCPSInfoRepoService service =
            new DCPSInfoRepoService(new String[] { "-NOBITS" });

        Thread t = new Thread(service);
        t.start();

        Thread.sleep(10000);

        service.shutdown();
        t.join();
    }
}
