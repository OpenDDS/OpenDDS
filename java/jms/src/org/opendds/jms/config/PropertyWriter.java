/*
 * $Id$
 */

package org.opendds.jms.config;

import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class PropertyWriter {
    private Configuration config;

    private List<String> args =
        new ArrayList<String>();

    public PropertyWriter(Configuration config) {
        this.config = config;
    }

    public void writeIfSet(String arg, String property) {
        if (config.isSet(property)) {
            write(arg, config.get(property));
        }
    }

    public void writeDelimited(String property) {
        if (config.isSet(property)) {
            StringTokenizer stok = new StringTokenizer(
                (String) config.get(property), Configuration.DELIMITERS);

            while (stok.hasMoreTokens()) {
                write(stok.nextToken());
            }
        }
    }

    public void write(String arg, Object value) {
        write(arg);

        if (!(value instanceof Boolean)) {
            write(value.toString());
        }
    }

    public void write(String value) {
        args.add(value);
    }

    public void writeTo(List<String> args) {
        args.addAll(this.args);
    }
}
