/*
 * $Id$
 */

package org.opendds.jms.loader;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URL;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.opendds.jms.util.ClassLoaders;
import org.opendds.jms.util.Files;
import org.opendds.jms.util.Streams;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class NativeLoader {
    private static Log log = LogFactory.getLog(NativeLoader.class);

    private File nativeDir;

    private List<File> loadedLibs =
        new ArrayList<File>();

    public NativeLoader(String dirName) throws IOException {
        nativeDir = Files.verifyDirectory(dirName);

        if (log.isDebugEnabled()) {
            log.debug("Using native directory: " + nativeDir.getAbsolutePath());
        }
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
        Enumeration<URL> en = loader.getResources(LibIndex.DEFAULT_RESOURCE);
        while (en.hasMoreElements()) {
            URL url = en.nextElement();

            LibIndex index = new LibIndex(url.openStream());
            long created = index.getCreated();

            for (LibIndex.Entry entry : index.getEntries()) {
                String name = entry.getName();

                File file = new File(nativeDir, name);

                if (log.isInfoEnabled()) {
                    log.info("Loading native library: " + file.getAbsolutePath());
                }

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
