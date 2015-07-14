/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management.argument;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;

/**
 * @author  Steven Stallion
 */
public class SvcConfDirective {
    public static String ARGUMENT_NAME = "-ORBSvcConfDirective";

    private boolean dynamic;
    private String serviceName;
    private String baseObjectType;
    private String library;
    private String factoryFunction;

    private List<String> options = new ArrayList<String>();

    public SvcConfDirective() {
        this(false);
    }

    public SvcConfDirective(boolean dynamic) {
        this.dynamic = dynamic;
    }

    public boolean isDynamic() {
        return dynamic;
    }

    public void setDynamic(boolean dynamic) {
        this.dynamic = dynamic;
    }

    public String getServiceName() {
        return serviceName;
    }

    public void setServiceName(String serviceName) {
        this.serviceName = serviceName;
    }

    public String getBaseObjectType() {
        return baseObjectType;
    }

    public void setBaseObjectType(String baseObjectType) {
        this.baseObjectType = baseObjectType;
    }

    public String getLibrary() {
        return library;
    }

    public void setLibrary(String library) {
        this.library = library;
    }

    public String getFactoryFunction() {
        return factoryFunction;
    }

    public void setFactoryFunction(String factoryFunction) {
        this.factoryFunction = factoryFunction;
    }

    public List<String> getOptions() {
        return Collections.unmodifiableList(options);
    }

    public void addOption(String option) {
        options.add(option);
    }

    public void addOptions(String... options) {
        for (String option : options) {
            addOption(option);
        }
    }

    @Override
    public String toString() {
        StringBuffer sbuf = new StringBuffer();

        if (dynamic) {
            sbuf.append("dynamic ");

        } else {
            sbuf.append("static ");
        }

        sbuf.append(serviceName).append(' ');

        if (dynamic) {
            sbuf.append(baseObjectType).append(' ');
            sbuf.append(library).append(':');
            sbuf.append(factoryFunction).append(' ');
        }

        sbuf.append('"');

        Iterator<String> itr = options.iterator();
        while (itr.hasNext()) {
            sbuf.append(itr.next());
            if (itr.hasNext()) {
                sbuf.append(' ');
            }
        }
        sbuf.append('"');

        return sbuf.toString();
    }

    public void writeTo(ArgumentWriter writer) {
        writer.write(ARGUMENT_NAME);
        writer.write(toString());
    }
}
