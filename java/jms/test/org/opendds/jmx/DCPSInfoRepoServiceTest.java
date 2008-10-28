/*
 * $Id$
 */

package org.opendds.jmx;

import org.junit.Before;
import org.junit.Test;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DCPSInfoRepoServiceTest {
    private DCPSInfoRepoServiceMBean service;

    @Before
    public void setUp() {
        service = new DCPSInfoRepoService();
        service.setNOBITS(true);
    }

    @Test(expected = IllegalStateException.class)
    public void errorStartAfterStart() throws Exception {
        try {
            service.start();
            service.start();

        } finally {
            service.stop();
        }
    }

    @Test(expected = IllegalStateException.class)
    public void errorStopWithoutStart() throws Exception {
        service.stop();
    }

    @Test
    public void testStartWithStop() throws Exception {
        service.start();
        service.stop();
    }

    // TODO Known Test Failures
    /*
    @Test
    public void testRestartWithStart() throws Exception {
        service.start();
        service.restart();
    }

    @Test
    public void testRestartWithoutStart() throws Exception {
        service.restart();
    }
    */
}
