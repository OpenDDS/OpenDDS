/*
 * $Id$
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
 * @version $Revision$
 */
public class NativeLoader {
    private static Logger logger = Logger.getLogger(NativeLoader.class);

    private File nativeDir;

    private List<File> loadedLibs =
        new ArrayList<File>();

    public NativeLoader(String dirName) throws IOException {
        nativeDir = Files.verifyDirectory(dirName);
        logger.debug("Using native directory: %s", nativeDir.getAbsolutePath());
    }

    public File getNativeDirectory() {
        return nativeDir;
    }

    public String getNativePath() {
        return nativeDir.getAbsolutePath();
    }

    public File[] getLoadedLibraries() {
        return loadedLibs.toArray(new File[loadedLibs.size()]);
    }

    public void loadLibraries() throws IOException {
        loadLibraries(ClassLoaders.getContextLoader());
    }

    public void loadLibraries(ClassLoader loader) throws IOException {
        assert loader != null;
        
        Enumeration<URL> en = loader.getResources(LibIndex.DEFAULT_RESOURCE);
        while (en.hasMoreElements()) {
            URL url = en.nextElement();

            LibIndex index = new LibIndex(url.openStream());
            long created = index.getCreated();

            for (LibIndex.Entry entry : index.getEntries()) {
                String name = entry.getName();

                File file = new File(nativeDir, name);
                logger.info("Loading native library: %s", file.getAbsolutePath());

                if (file.exists() && file.lastModified() > created) {
                    continue; // library is up to date
                }

                FileOutputStream out = null;
                try {
                    out = new FileOutputStream(file);
                    Streams.tie(entry.openStream(loader), out);

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
