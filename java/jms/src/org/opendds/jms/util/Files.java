/*
 * $Id$
 */

package org.opendds.jms.util;

import java.io.File;
import java.io.IOException;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Files {

    public static File createTempDirectory(String prefix) throws IOException {
        return createTempDirectory(prefix, null);
    }

    public static File createTempDirectory(String prefix, String suffix) throws IOException {
        return null;
    }

    //

    private Files() {}
}
