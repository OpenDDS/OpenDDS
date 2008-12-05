/*
 * $Id$
 */

package org.opendds.jms.common.io;

import java.io.File;
import java.io.IOException;
import java.security.SecureRandom;
import java.util.StringTokenizer;

import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class Files {

    public static File getDefaultTempDirectory() {
        return new File(System.getProperty("java.io.tmpdir"));
    }

    public static File createTempDirectory(String prefix) throws IOException {
        return createTempDirectory(prefix, null);
    }

    public static synchronized File createTempDirectory(String prefix, String suffix) throws IOException {
        assert prefix != null;

        File temp;
        do {
            StringBuffer sbuf = new StringBuffer();

            sbuf.append(prefix);
            sbuf.append(new SecureRandom().nextInt() & 0xffff);

            if (!Strings.isEmpty(suffix)) {
                sbuf.append(suffix);
            }

            temp = new File(getDefaultTempDirectory(), sbuf.toString());

        } while (temp.exists());

        if (!temp.mkdirs()) {
            throw new IOException("Unable to create temp directory: " + temp.getAbsolutePath());
        }

        return temp;
    }

    public static File verifyDirectory(String dirName) throws IOException {
        assert dirName != null;

        File dir = new File(dirName);

        if (!dir.exists()) {
            if (!dir.mkdirs()) {
                throw new IOException("Unable to create directory: " + dir.getAbsolutePath());
            }

        } else if (!dir.isDirectory()) {
            throw new IOException(dirName + " is not a directory!");
        }

        return dir;
    }

    public static String getLibraryPath() {
        return System.getProperty("java.library.path");
    }

    public static void setLibraryPath(String libraryPath) {
        System.setProperty("java.library.path", libraryPath);
    }

    public static boolean isLibraryPathSet(String path) {
        assert path != null;

        StringTokenizer stok =
            new StringTokenizer(getLibraryPath(), File.pathSeparator);

        while (stok.hasMoreTokens()) {
            if (path.equals(stok.nextToken())) {
                return true;
            }
        }
        return false;
    }

    public static String addLibraryPath(File directory) {
        assert directory != null;
        
        return addLibraryPath(directory.getAbsolutePath());
    }

    public static String addLibraryPath(String path) {
        assert path != null;

        String libraryPath = getLibraryPath();
        if (!isLibraryPathSet(path)) {
            StringBuffer sbuf = new StringBuffer(libraryPath);

            sbuf.append(File.pathSeparator);
            sbuf.append(path);

            libraryPath = sbuf.toString();
            setLibraryPath(libraryPath);
        }

        return libraryPath;
    }

    //

    private Files() {}
}
