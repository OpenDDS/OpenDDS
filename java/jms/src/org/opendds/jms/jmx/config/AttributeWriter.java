/*
 * $Id$
 */

package org.opendds.jms.jmx.config;

import java.util.ArrayList;
import java.util.List;
import java.util.StringTokenizer;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class AttributeWriter {
    private Attributes attributes;

    private List<String> args =
        new ArrayList<String>();

    public AttributeWriter(Attributes attributes) {
        this.attributes = attributes;
    }

    public void writeIfSet(String arg, String attribute) {
        if (attributes.isSet(attribute)) {
            write(arg, attributes.get(attribute));
        }
    }

    public void writeDelimited(String attribute) {
        if (attributes.isSet(attribute)) {
            StringTokenizer stok = new StringTokenizer(
                (String) attributes.get(attribute), Attributes.DELIMITERS);

            while (stok.hasMoreTokens()) {
                write(stok.nextToken());
            }
        }
    }

    public void write(String arg, Object value) {
        if (value instanceof Boolean) {
            write(arg);

        } else {
            write(arg);
            write(value.toString());
        }
    }

    public void write(SvcConfDirective directive) {
        write("-ORBSvcConfDirective");
        write(directive.toString());
    }

    public void write(String value) {
        args.add(value);
    }

    public void writeTo(List<String> args) {
        args.addAll(this.args);
    }
}
