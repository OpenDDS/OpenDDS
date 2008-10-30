/*
 * $Id$
 */

package org.opendds.config.spi;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import sun.misc.Service;

import org.opendds.config.Configuration;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public abstract class PropertyFormat {
    private static List<PropertyFormat> instances =
        new ArrayList<PropertyFormat>();

    static {
        registerAll();
    }

    public static void registerAll() {
        Iterator itr = Service.providers(PropertyFormat.class);
        while (itr.hasNext()) {
            register((PropertyFormat) itr.next());
        }
    }

    public static void register(PropertyFormat instance) {
        instances.add(instance);
    }

    public static void unregister(PropertyFormat instance) {
        instances.remove(instance);
    }

    public static void unregisterAll() {
        instances.clear();
    }

    public static String[] format(Configuration attributes) {
        List<String> args = new ArrayList<String>();

        for (PropertyFormat format : instances) {
            format.format(attributes, args);
        }

        return args.toArray(new String[args.size()]);
    }

    //

    protected abstract void format(Configuration config, List<String> args);
}
