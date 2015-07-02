/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.spi;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.StreamTokenizer;
import java.util.Iterator;
import java.util.NoSuchElementException;

import org.opendds.jms.common.lang.ClassLoaders;
import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 */
public class Service {

    public static <T> Iterator<T> providers(Class<T> clazz) {
        return providers(clazz, ClassLoaders.getContextLoader());
    }

    public static <T> Iterator<T> providers(Class<T> clazz, ClassLoader cl) {
        return new ServiceIterator<T>(
            ClassLoaders.getResourceAsStream("META-INF/services/" + clazz.getName(), cl));
    }

    private static class ServiceIterator<T> implements Iterator<T> {
        private StreamTokenizer tokenizer;
        private boolean eof;
        private String next;

        public ServiceIterator(InputStream is) {
            tokenizer = new StreamTokenizer(new InputStreamReader(is));

            tokenizer.commentChar('#');
            tokenizer.slashSlashComments(false);
            tokenizer.slashStarComments(false);

            tokenizer.eolIsSignificant(true);
            tokenizer.ordinaryChars('0', '9');
        }

        public boolean hasNext() {
            if (eof) {
                return false;
            }

            if (next != null) {
                return true;
            }

            try { // prefetch next token
                StringBuilder sb = new StringBuilder();

                for (;;) {
                    switch(tokenizer.nextToken()) {
                        case StreamTokenizer.TT_WORD:
                            sb.append(tokenizer.sval);
                            break;

                        case StreamTokenizer.TT_EOL:
                            String s = sb.toString();
                            if (Strings.isEmpty(s)) {
                                sb.setLength(0);
                                break; // ignore empty line
                            }
                            next = s;
                            return true;

                        case StreamTokenizer.TT_EOF:
                            eof = true;
                            return false;
                    }
                }

            } catch (IOException e) {
                throw new IllegalStateException(e);
            }
        }

        public String takeNext() {
            try {
                return next;

            } finally {
                next = null;
            }
        }

        @SuppressWarnings("unchecked")
        public T next() {
            if (!hasNext()) {
                throw new NoSuchElementException();
            }

            try {
                Class clazz = Class.forName(takeNext());
                return (T) clazz.newInstance();

            } catch (Exception e) {
                throw new IllegalStateException(e);
            }
        }

        public void remove() {
            throw new UnsupportedOperationException();
        }
    }
}
