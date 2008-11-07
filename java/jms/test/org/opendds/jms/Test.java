/*
 * $
 */

package org.opendds.jms;

import org.opendds.jms.resource.NativeLoader;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Test {

    @org.junit.Test
    public void test() throws Exception {
        NativeLoader loader = new NativeLoader(true);

        loader.loadLibraries();
    }
}
