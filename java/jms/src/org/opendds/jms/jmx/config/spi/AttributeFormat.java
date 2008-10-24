/*
 * $Id$
 */

package org.opendds.jms.jmx.config.spi;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import sun.misc.Service;

import org.opendds.jms.jmx.config.Attributes;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public abstract class AttributeFormat {
    private static List<AttributeFormat> instances =
        new ArrayList<AttributeFormat>();

    static {
        registerAll();
    }

    public static void registerAll() {
        Iterator itr = Service.providers(AttributeFormat.class);
        while (itr.hasNext()) {
            register((AttributeFormat) itr.next());
        }
    }

    public static void register(AttributeFormat instance) {
        instances.add(instance);
    }

    public static void unregister(AttributeFormat instance) {
        instances.remove(instance);
    }

    public static void unregisterAll() {
        instances.clear();
    }

    public static String[] format(Attributes attributes) {
        List<String> args = new ArrayList<String>();

        for (AttributeFormat format : instances) {
            format.format(attributes, args);
        }

        return args.toArray(new String[args.size()]);
    }

    //

    protected abstract void format(Attributes attributes, List<String> args);
}
