/*
 * $Id$
 */

package org.opendds.jms.management.argument;

import java.util.ArrayList;
import java.util.Iterator;
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
        StringBuilder sb = new StringBuilder();
        try {
            Iterator<String> itr = asList().iterator();
            while (itr.hasNext()) {
                sb.append(itr.next());
                if (itr.hasNext()) {
                    sb.append(' ');
                }
            }

        } catch (Exception e) {
            logger.error("Unexpected problem rendering toString(): %s", e.getMessage(), e);
        }
        return sb.toString();
    }
}
