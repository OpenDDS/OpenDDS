/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.management.argument;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.omg.CORBA.StringSeqHolder;

import org.opendds.jms.management.DynamicMBeanSupport;

/**
 * @author  Steven Stallion
 */
public class DynamicArguments {
    public static final String DELIMS = ";, \t\r\n\f";

    private DynamicMBeanSupport instance;

    private List<DynamicArgumentProvider> providers =
        new ArrayList<DynamicArgumentProvider>();

    public DynamicArguments(DynamicMBeanSupport instance) {
        assert instance != null;

        this.instance = instance;
    }

    public void register(DynamicArgumentProvider provider) {
        assert provider != null;

        provider.setInstance(instance);
        provider.registerAttributes();

        providers.add(provider);
    }

    public List<String> asList() throws Exception {
        List<String> args = new ArrayList<String>();

        for (DynamicArgumentProvider provier : providers) {
            provier.addArgs(args);
        }

        return args;
    }

    public String[] toArray() throws Exception {
        List<String> args = asList();
        return args.toArray(new String[args.size()]);
    }

    public StringSeqHolder toStringSeq() throws Exception {
        return new StringSeqHolder(toArray());
    }

    @Override
    public String toString() {
        try {
            return Arrays.deepToString(toArray());

        } catch (Exception e) {
            throw new IllegalStateException(e);
        }
    }
}
