/*
 * $Id$
 */

package org.opendds.jms.util;

import java.util.Enumeration;

import javax.naming.CompositeName;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class JndiHelper {
    private Context context;

    public JndiHelper() {
        this(null);
    }

    public JndiHelper(Context context) {
        this.context = context;
    }

    public Context getContext() throws NamingException {
        if (context == null) {
            context = new InitialContext();
        }
        return context;
    }

    public Object lookup(String name) throws NamingException {
        Context context = getContext();
        return context.lookup(name);
    }

    public void bind(String name, Object value) throws NamingException {
        Context context = getContext();
        context.rebind(verify(name), value);
    }

    public void unbind(String name) throws NamingException {
        Context context = getContext();
        context.unbind(name);
    }

    public String verify(String name) throws NamingException {
        Context context = getContext();

        Enumeration<String> en = new CompositeName(name).getAll();
        while (en.hasMoreElements()) {
            String comp = en.nextElement();

            // Ignore last component (not a context)
            if (!en.hasMoreElements()) {
                break;
            }

            try {
                Object obj = context.lookup(comp);
                if (obj instanceof Context) {
                    context = (Context) obj;

                } else {
                    break; // not a context
                }
                continue;

            } catch (NamingException e) {}  

            context = context.createSubcontext(comp);
        }

        return name;
    }
}
