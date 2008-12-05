/*
 * $Id$
 */

package org.opendds.jms;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

/**
 * Some common code to all the tests.
 *
 * @author  Weiqi Gao
 * @version $Revision$
 */
public class TestUtils {
    static boolean runWithInfoRepo() {
        // Temporary hack
        return testWithInfoRepo() && infoRepoRunning();
    }

    private static boolean testWithInfoRepo() {
        final String s = System.getProperty("opendds.test.withInfoRepo", "false");
        return s.equals("true");
    }

    private static boolean infoRepoRunning() {
        try {
            final BufferedReader bufferedReader = new BufferedReader(
                new InputStreamReader(Runtime.getRuntime().exec("netstat -an").getInputStream()));
            String line = null;
            while ((line = bufferedReader.readLine()) != null) {
                if (line.contains(":4096")) return true;
            }
            return false;
        } catch (IOException e) {
            return false;
        }
    }

}
