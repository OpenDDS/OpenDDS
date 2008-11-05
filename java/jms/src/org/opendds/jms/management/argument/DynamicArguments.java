/*
 * $Id$
 */

package org.opendds.jms.management.argument;

import java.util.List;
import java.util.ArrayList;

import org.opendds.jms.management.DynamicMBeanSupport;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class DynamicArguments {
    public static final String DELIMS = ";, \t\r\n\f";

    private DynamicMBeanSupport instance;

    private List<DynamicArgumentProvider> providers =
        new ArrayList<DynamicArgumentProvider>();

    public DynamicArguments(DynamicMBeanSupport instance) {
        this.instance = instance;
    }

    public void register(DynamicArgumentProvider provider) {
        provider.setInstance(instance);
        provider.registerAttributes();

        providers.add(provider);
    }

    public String[] toArgs() throws Exception {
        List<String> args = new ArrayList<String>();

        for (DynamicArgumentProvider provier : providers) {
            provier.addArgs(args);
        }

        return args.toArray(new String[args.size()]);
    }
}
