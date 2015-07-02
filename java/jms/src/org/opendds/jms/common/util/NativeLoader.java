/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.util;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

import org.opendds.jms.common.io.Files;
import org.opendds.jms.common.io.Streams;
import org.opendds.jms.common.lang.ClassLoaders;

/**
 * @author  Steven Stallion
 */
public class NativeLoader {
    private static final String NATIVE_DIR_PROPERTY = "opendds.native.dir";
    private static Logger logger = Logger.getLogger(NativeLoader.class);

    public static void bootstrap() {
        if (libraryLoaded("opendds-jms-native")) {
            return; // libraries already loaded
        }
        logger.warn("Broken RAR deployer detected; loading native libraries");

        PropertiesHelper helper = PropertiesHelper.getSystemPropertiesHelper();
        try {
            File parent = helper.getFileProperty(NATIVE_DIR_PROPERTY);
            if (parent == null) {
                logger.warn("%s is not set; Using temporary directory " +
                            "(HINT: This is probably not what you want!)", NATIVE_DIR_PROPERTY);
                parent = Files.createTempDirectory("opendds-native");
            }
            if (!Files.isLibraryPathSet(parent)) {
                logger.warn("%s is not set in java.library.path!", parent);
            }

            NativeLoader loader = new NativeLoader(parent);
            loader.loadLibraries();

        } catch (IOException e) {
            throw new IllegalStateException(e);
        }
    }

    public static boolean libraryLoaded(String libname) {
        try {
            System.loadLibrary(resolveLibraryName(libname));
            return true;

        } catch (UnsatisfiedLinkError e) {}
        return false;
    }

    public static String resolveLibraryName(String libname) {
        if (Boolean.getBoolean("opendds.native.debug")) {
            return libname.concat("d");
        }
        return libname;
    }

    private File parent;

    private List<File> loadedLibs =
        new ArrayList<File>();

    public NativeLoader(File parent) throws IOException {
        logger.debug("Using native directory: %s", parent.getAbsolutePath());
        this.parent = Files.verifyDirectory(parent);
    }

    public File getParentDirectory() {
        return parent;
    }

    public String getParentPath() {
        return parent.getAbsolutePath();
    }

    public File[] getLoadedLibraries() {
        return loadedLibs.toArray(new File[loadedLibs.size()]);
    }

    public void loadLibraries() throws IOException {
        loadLibraries(ClassLoaders.getContextLoader());
    }

    public void loadLibraries(ClassLoader cl) throws IOException {
        assert cl != null;

        Enumeration<URL> en = cl.getResources(LibIndex.DEFAULT_RESOURCE);
        while (en.hasMoreElements()) {
            URL url = en.nextElement();

            LibIndex index = new LibIndex(url.openStream());
            long created = index.getCreated();

            for (LibIndex.Entry entry : index.getEntries()) {
                String name = entry.getName();

                File file = new File(parent, name);
                logger.info("Loading native library: %s", file.getAbsolutePath());

                if (file.exists() && file.lastModified() > created) {
                    continue; // library is up to date
                }

                FileOutputStream out = null;
                try {
                    out = new FileOutputStream(file);
                    Streams.tie(entry.openStream(cl), out);

                    loadedLibs.add(file);

                } finally {
                    if (out != null) {
                        try {
                            out.close();
                        } catch (IOException e) {}
                    }
                }
            }
        }
    }
}
