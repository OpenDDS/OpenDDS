/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.util;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import org.opendds.jms.common.lang.Strings;

/**
 * @author  Steven Stallion
 */
public class LibIndex {
    public static final String DEFAULT_RESOURCE = "META-INF/INDEX.LIBS";

    public static class Entry {
        private String name;
        private String resource;

        protected Entry(String resource) {
            assert resource != null;

            this.resource = resource;

            // Determine library name (strip resource path)
            int index = resource.lastIndexOf("/");
            if (index != -1) {
                name = resource.substring(index + 1);
            } else {
                name = resource;
            }
        }

        public String getName() {
            return name;
        }

        public String getResource() {
            return resource;
        }

        public InputStream openStream(ClassLoader loader) throws IOException {
            assert loader != null;

            return loader.getResourceAsStream(resource);
        }
    }

    private Map<String, String> attributes =
        new LinkedHashMap<String, String>();

    private List<Entry> entries =
        new ArrayList<Entry>();

    public LibIndex(InputStream in) throws IOException {
        parse(in);
    }

    public String getVersion() {
        return getAttribute("LibIndex-Version");
    }

    public long getCreated() {
        return Long.parseLong(getAttribute("LibIndex-Created"));
    }

    public String getAttribute(String name) {
        return attributes.get(name);
    }

    public Collection<Entry> getEntries() {
        return Collections.unmodifiableCollection(entries);
    }

    protected void parse(InputStream in) throws IOException {
        assert in != null;

        boolean parsedHeader = false;

        BufferedReader reader =
            new BufferedReader(new InputStreamReader(in));

        String line;
        while ((line = reader.readLine()) != null) {
            if (!parsedHeader) {
                if (Strings.isEmpty(line)) {
                    parsedHeader = true;

                } else {
                    int index = line.indexOf(":");

                    if (index == -1) {
                        throw new IOException("Invalid attribute: " + line);
                    }

                    String name = line.substring(0, index).trim();
                    String value = line.substring(index + 1).trim();

                    attributes.put(name, value);
                }

            } else {
                entries.add(new Entry(line));
            }
        }
    }
}
