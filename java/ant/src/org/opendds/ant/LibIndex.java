/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.ant;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.DirectoryScanner;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.types.FileSet;

/**
 * @author  Steven Stallion
 */
public class LibIndex extends Task {
    public static String DEFAULT_FILE = "INDEX.LIBS";

    public static String ENTRY_SEPARATOR = "\n";
    public static String RESOURCE_SEPARATOR = "/";

    private File dir;
    private File file;
    private String prefix;

    private List<FileSet> filesets =
        new ArrayList<FileSet>();

    public void setDir(File dir) {
        this.dir = dir;
    }

    public File getFile() {
        if (dir != null) {
            file = new File(dir, DEFAULT_FILE);
        }
        return file;
    }

    public void setFile(File file) {
        this.file = file;
    }

    public String getPrefix() {
        return prefix;
    }

    public void setPrefix(String prefix) {
        this.prefix = prefix;
    }

    public void addFileSet(FileSet fileset) {
        filesets.add(fileset);
    }

    protected void verify() throws BuildException {
        if (dir == null && file == null) {
            throw new BuildException("dir or file must be specified!");
        }

        if (filesets.isEmpty()) {
            throw new BuildException("At least one fileset child element must be preset!");
        }
    }

    protected void createIndex(FileWriter writer) throws IOException {
        // Library-Index-Version
        writer.write("LibIndex-Version: 1.0");
        writer.write(ENTRY_SEPARATOR);

        // Creation-Stamp
        writer.write("LibIndex-Created: ");
        writer.write(String.valueOf(System.currentTimeMillis()));
        writer.write(ENTRY_SEPARATOR);

        writer.write(ENTRY_SEPARATOR);

        // Library entries
        for (FileSet fileset : filesets) {
            DirectoryScanner ds = fileset.getDirectoryScanner();
            for (String file : ds.getIncludedFiles()) {
                if (prefix != null) {
                    writer.write(prefix);

                    if (!prefix.endsWith(RESOURCE_SEPARATOR)) {
                        writer.write(RESOURCE_SEPARATOR);
                    }
                }
                writer.write(file);
                writer.write(ENTRY_SEPARATOR);
            }
        }
    }

    @Override
    public void execute() throws BuildException {
        verify();

        File file = getFile();

        log("Creating library index: " + file.getAbsolutePath());

        FileWriter writer = null;
        try {
            writer = new FileWriter(file);
            createIndex(writer);

        } catch (IOException e) {
            throw new BuildException(e);

        } finally {
            if (writer != null) {
                try {
                    writer.close();

                } catch (IOException e) {}
            }
        }
    }
}
