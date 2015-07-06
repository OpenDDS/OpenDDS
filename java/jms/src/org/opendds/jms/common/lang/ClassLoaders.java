/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.lang;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.net.URL;

/**
 * @author  Steven Stallion
 */
public class ClassLoaders {

    public static ClassLoader getContextLoader() {
        return Thread.currentThread().getContextClassLoader();
    }

    public static URL getResource(String name) {
        return getResource(name, getContextLoader());
    }

    public static URL getResource(String name, ClassLoader cl) {
        assert name != null;

        return verifyResource(name, cl.getResource(name));
    }

    public static Reader getResourceAsReader(String name) {
        return getResourceAsReader(name, getContextLoader());
    }

    public static Reader getResourceAsReader(String name, ClassLoader cl) {
        return new InputStreamReader(getResourceAsStream(name, cl));
    }

    public static InputStream getResourceAsStream(String name) {
        return getResourceAsStream(name, getContextLoader());
    }

    public static InputStream getResourceAsStream(String name, ClassLoader cl) {
        assert name != null;
        assert cl != null;

        return verifyResource(name, cl.getResourceAsStream(name));
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
