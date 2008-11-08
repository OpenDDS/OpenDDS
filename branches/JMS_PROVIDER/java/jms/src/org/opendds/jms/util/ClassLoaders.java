/*
 * $Id$
 */

package org.opendds.jms.util;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ClassLoaders {

    public static ClassLoader getContextLoader() {
        return Thread.currentThread().getContextClassLoader();
    }

    //

    private ClassLoaders() {}
}
