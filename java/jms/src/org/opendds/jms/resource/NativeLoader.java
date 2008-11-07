/*
 * $Id$
 */

package org.opendds.jms.resource;

import java.io.File;
import java.io.IOException;
import java.util.jar.Manifest;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.util.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class NativeLoader {
    private static final String OPENDDS_NATIVE_DIR_PROP = "opendds.native.dir";

    private static Log log = LogFactory.getLog(NativeLoader.class);

    private File nativeDirectory;

    public NativeLoader() throws IOException {
        this(true);
    }

    public NativeLoader(boolean cleanUp) throws IOException {
        String dirName = System.getProperty(OPENDDS_NATIVE_DIR_PROP);
        if (!Strings.isEmpty(dirName)) {
            nativeDirectory = new File(dirName);
            if (!nativeDirectory.isDirectory()) {
                throw new IOException(OPENDDS_NATIVE_DIR_PROP + " is not a directory!");
            }
        }

        // Use temporary directory
        if (nativeDirectory == null) {
            //nativeDirectory = Files.createTempDirectory("opendds");
            nativeDirectory = new File("/var/tmp/bob");
            if (cleanUp) {
                nativeDirectory.deleteOnExit();
            }
        }

        if (log.isDebugEnabled()) {
            log.debug("Using native directory: " + nativeDirectory.getAbsolutePath());
        }
    }

    public File getNativeDirectory() {
        return nativeDirectory;
    }

    public void loadLibraries() throws IOException {
        Manifest manifest = new Manifest();
    }
}
