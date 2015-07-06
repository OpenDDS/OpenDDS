/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.ant;

import org.apache.tools.ant.Project;
import org.apache.tools.ant.taskdefs.condition.Os;
import org.apache.tools.ant.types.FileSet;
import org.apache.tools.ant.types.PatternSet;

/**
 * @author  Steven Stallion
 */
public class LibFileSet extends FileSet {

    public class LibPatternSet extends PatternSet {

        public NameEntry createNameEntry(NameEntry entry) {
            return new LibNameEntry(entry);
        }

        public class LibNameEntry extends NameEntry {
            private NameEntry entry;

            public LibNameEntry(NameEntry entry) {
                this.entry = entry;
            }

            @Override
            public void setName(String name) {
                StringBuffer sbuf = new StringBuffer();

                if (Os.isFamily(Os.FAMILY_UNIX)) {
                    sbuf.append("lib");
                }

                sbuf.append(name);

                if (debug && Os.isFamily(Os.FAMILY_WINDOWS)) {
                    sbuf.append("d");
                }

                sbuf.append('.');

                if (Os.isFamily(Os.FAMILY_MAC)) {
                    sbuf.append("dylib");

                } else if (Os.isFamily(Os.FAMILY_WINDOWS)) {
                    sbuf.append("dll");

                } else { // Os.FAMILY_UNIX
                    sbuf.append("so");
                }

                entry.setName(sbuf.toString());
            }

            @Override
            public String evalName(Project p) {
                return entry.evalName(p);
            }

            @Override
            public String getName() {
                return entry.getName();
            }

            @Override
            public void setIf(String cond) {
                entry.setIf(cond);
            }

            @Override
            public void setUnless(String cond) {
                entry.setUnless(cond);
            }
        }
    }

    private boolean debug;

    private LibPatternSet pattern = new LibPatternSet();

    public LibFileSet() {
        super();
    }

    public boolean getDebug() {
        return debug;
    }

    public void setDebug(boolean debug) {
        this.debug = debug;
    }

    @Override
    public synchronized PatternSet.NameEntry createInclude() {
        return pattern.createNameEntry(super.createInclude());
    }

    @Override
    public synchronized PatternSet.NameEntry createExclude() {
        return pattern.createNameEntry(super.createExclude());
    }
}
