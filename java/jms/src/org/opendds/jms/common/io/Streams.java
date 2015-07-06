/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.io;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * @author  Steven Stallion
 */
public class Streams {
    public static final int BUFFER_CAPACITY = 16384;
    public static final int EOF = -1;

    public static byte[] createBuffer() {
        return new byte[BUFFER_CAPACITY];
    }

    public static void tie(InputStream in, OutputStream out) throws IOException {
        assert in != null;
        assert out != null;

        byte[] buffer = createBuffer();

        int read;
        do {
            read = in.read(buffer);
            if (read > 0) {
                out.write(buffer, 0, read);
            }

        } while (read != EOF);

        out.flush();
    }

    //

    private Streams() {}
}
