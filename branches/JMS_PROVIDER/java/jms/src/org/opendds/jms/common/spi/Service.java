/*
 * $Id$
 */

package org.opendds.jms.common.spi;

import java.io.IOException;
import java.io.Reader;
import java.io.StreamTokenizer;
import java.util.Iterator;
import java.util.NoSuchElementException;

import org.opendds.jms.common.lang.ClassLoaders;
import org.opendds.jms.common.lang.Strings;

/**
 * @author Steven Stallion
 * @version $Revision$
 */
public class Service {

    protected static String getResource(Class clazz) {
        return "META-INF/services/" + clazz.getName();
    }

    public static <T> Iterator<T> providers(Class<T> clazz) {
        return providers(clazz, ClassLoaders.getContextLoader());
    }

    public static <T> Iterator<T> providers(Class<T> clazz, ClassLoader cl) {
        Reader reader = ClassLoaders.getResourceAsReader(getResource(clazz), cl);

        final StreamTokenizer st = new StreamTokenizer(reader);

        st.commentChar('#');
        st.slashSlashComments(true);
        st.slashStarComments(true);

        return new Iterator<T>() {
            {
                nextToken(); // pre-load first value
            }

            protected void nextToken() {
                try {
                    while (st.nextToken() != StreamTokenizer.TT_EOF) {
                        if (!Strings.isEmpty(st.sval)) {
                            break;
                        }
                    }

                } catch (IOException e) {
                    throw new IllegalStateException(e);
                }
            }

            public boolean hasNext() {
                return st.ttype != StreamTokenizer.TT_EOF;
            }

            @SuppressWarnings("unchecked")
            public T next() {
                if (!hasNext()) {
                    throw new NoSuchElementException();
                }

                try {
                    Class<T> clazz = (Class<T>) Class.forName(st.sval);
                    return clazz.newInstance();

                } catch (Exception e) {
                    throw new IllegalStateException(e);

                } finally {
                    nextToken();
                }
            }

            public void remove() {
                throw new UnsupportedOperationException();
            }
        };
    }

    //

    private Service() {}
}
