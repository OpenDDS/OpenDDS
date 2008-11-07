/*
 * $Id$
 */

package org.opendds.ant;

import java.io.File;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.condition.Os;
import org.apache.tools.ant.types.selectors.BaseSelector;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class LibrarySelector extends BaseSelector {
    private boolean debug;
    private String name;

    public boolean getDebug() {
        return debug;
    }

    public void setDebug(boolean debug) {
        this.debug = debug;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    @Override
    public void verifySettings() {
        if (isReference()) {
            super.verifySettings();

        } else {
            if (name == null || "".equals(name.trim())) {
                setError("name is a required attribute!");
            }
        }
    }

    public boolean isSelected(File basedir, String filename, File file) throws BuildException {
        String libraryName = toLibraryName();

        System.out.println(libraryName + " -> " + filename);

        if (Os.isFamily(Os.FAMILY_WINDOWS)) {
            return libraryName.equalsIgnoreCase(filename);
        }
        return libraryName.equals(filename);
    }

    public String toLibraryName() {
        StringBuffer sbuf = new StringBuffer();

        if (Os.isFamily(Os.FAMILY_UNIX)) {
            sbuf.append("lib");
        }

        sbuf.append(name);

        if (debug && Os.isFamily(Os.FAMILY_WINDOWS)) {
            sbuf.append("D");
        }

        sbuf.append('.');

        if (Os.isFamily(Os.FAMILY_UNIX)) {
            sbuf.append("so");

        } else if (Os.isFamily(Os.FAMILY_MAC)) {
            sbuf.append("dylib");

        } else { // Os.FAMILY_WINDOWS
            sbuf.append("DLL");
        }

        return sbuf.toString();
    }
}
