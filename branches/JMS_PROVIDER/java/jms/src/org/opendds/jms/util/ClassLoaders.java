/*
 * $Id$
 */

package org.opendds.jms.util;

import java.io.InputStream;
import java.net.URL;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ClassLoaders {

    public static ClassLoader getContextLoader() {
        return Thread.currentThread().getContextClassLoader();
    }

    public static URL getResource(String name) {
        return verifyResource(name, getContextLoader().getResource(name));
    }

    public static InputStream getResourceAsStream(String name) {
        return verifyResource(name, getContextLoader().getResourceAsStream(name));
    }

    private static <T> T verifyResource(String name, T t) {
        if (t == null) {
            throw new IllegalArgumentException("Unable to find resource: " + name);
        }
        return t;
    }

    //

    private ClassLoaders() {}
}
