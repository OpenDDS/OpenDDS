/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.util;

import java.util.Enumeration;

import javax.naming.CompositeName;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;

/**
 * @author  Steven Stallion
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

    @SuppressWarnings("unchecked")
    public <T> T lookup(String name) throws NamingException {
        Context context = getContext();
        return (T) context.lookup(name);
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
        assert name != null;

        Context context = getContext();

        Enumeration<String> en = new CompositeName(name).getAll();
        while (en.hasMoreElements()) {
            String comp = en.nextElement();

            if (!en.hasMoreElements()) {
                break; // ignore last component; not a context
            }

            try {
                Object obj = context.lookup(comp);
                if (!(obj instanceof Context)) {
                    break; // not a context; abort
                }
                context = (Context) obj; // context exists

            } catch (NamingException e) {
                context = context.createSubcontext(comp);
            }
        }

        return name;
    }
}
