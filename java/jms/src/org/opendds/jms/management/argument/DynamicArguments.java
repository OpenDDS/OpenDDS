/*
 * $Id$
 */

package org.opendds.jms.management.argument;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.omg.CORBA.StringSeqHolder;

import org.opendds.jms.common.util.Logger;
import org.opendds.jms.management.DynamicMBeanSupport;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DynamicArguments {
    public static final String DELIMS = ";, \t\r\n\f";

    private static Logger logger = Logger.getLogger(DynamicArguments.class);

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

    public String[] toArgs() throws Exception {
        List<String> args = asList();
        return args.toArray(new String[args.size()]);
    }

    public StringSeqHolder toStringSeq() throws Exception {
        return new StringSeqHolder(toArgs());
    }

    @Override
    public String toString() {
        try {
            return Arrays.deepToString(toArgs());

        } catch (Exception e) {
            throw new IllegalStateException(e);
        }
    }
}
