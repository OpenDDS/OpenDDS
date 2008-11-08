/*
 * $Id$
 */

package org.opendds.jms.loader;

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

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class LibraryIndex {
    public static final String DEFAULT_RESOURCE = "META-INF/INDEX.LIBS";

    public static final String VERSION_ATTRIBUTE = "Lib-Index-Version";
    public static final String CREATED_ATTRIBUTE = "Lib-Index-Created";

    public static final String ATTRIBUTE_SEPARATOR = ":";
    public static final String RESOURCE_SEPARATOR = "/";

    public static class Entry {
        private String name;
        private String resource;

        protected Entry(String resource) {
            this.resource = resource;

            // Determine library name (strip resource path)
            int index = resource.lastIndexOf(RESOURCE_SEPARATOR);
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
            return loader.getResourceAsStream(resource);
        }
    }

    private Map<String, String> attributes =
        new LinkedHashMap<String, String>();

    private List<Entry> entries =
        new ArrayList<Entry>();

    public LibraryIndex(InputStream in) throws IOException {
        parse(in);
    }

    public String getVersion() {
        return getAttribute(VERSION_ATTRIBUTE);
    }

    public long getCreated() {
        return Long.parseLong(getAttribute(CREATED_ATTRIBUTE));
    }

    public String getAttribute(String name) {
        return attributes.get(name);
    }

    public Collection<Entry> getEntries() {
        return Collections.unmodifiableCollection(entries);
    }

    protected void parse(InputStream in) throws IOException {
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        boolean parsedHeader = false;

        String line;
        while ((line = reader.readLine()) != null) {
            if (!parsedHeader) {
                if ("".equals(line)) {
                    parsedHeader = true;

                } else {
                    int index = line.indexOf(ATTRIBUTE_SEPARATOR);

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
