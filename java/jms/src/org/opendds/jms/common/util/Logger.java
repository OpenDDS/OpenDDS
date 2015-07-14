/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.util;

import java.util.Arrays;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * @author  Steven Stallion
 */
public class Logger {

    public static Logger getLogger(String name) {
        return new Logger(LogFactory.getLog(name));
    }

    public static Logger getLogger(String name, Object discriminator) {
        return new Logger(LogFactory.getLog(name + "-" + discriminator));
    }

    public static Logger getLogger(Class clazz) {
        return new Logger(LogFactory.getLog(clazz));
    }

    private Log log;

    protected Logger(Log log) {
        this.log = log;
    }

    public boolean isDebugEnabled() {
        return log.isDebugEnabled();
    }

    public void debug(String message, Object... args) {
        if (isDebugEnabled()) {
            log.debug(String.format(message, args));
        }
    }

    public void debug(String message, String[] args) {
        if (isDebugEnabled()) {
            debug(message, Arrays.deepToString(args));
        }
    }

    public void debug(String message, Throwable cause) {
        if (isDebugEnabled()) {
            log.debug(message, cause);
        }
    }

    public boolean isInfoEnabled() {
        return log.isInfoEnabled();
    }

    public void info(String message, Object... args) {
        if (isInfoEnabled()) {
            log.info(String.format(message, args));
        }
    }

    public void info(String message, String[] args) {
        if (isInfoEnabled()) {
            info(message, Arrays.deepToString(args));
        }
    }

    public void info(String message, Throwable cause) {
        if (isInfoEnabled()) {
            log.info(message, cause);
        }
    }

    public boolean isErrorEnabled() {
        return log.isErrorEnabled();
    }

    public void error(String message, Object... args) {
        if (isErrorEnabled()) {
            log.error(String.format(message, args));
        }
    }

    public void error(String message, String[] args) {
        if (isErrorEnabled()) {
            error(message, Arrays.deepToString(args));
        }
    }

    public void error(String message, Throwable cause) {
        if (isErrorEnabled()) {
            log.error(message, cause);
        }
    }

    public boolean isWarnEnabled() {
        return log.isWarnEnabled();
    }

    public void warn(String message, Object... args) {
        if (isWarnEnabled()) {
            log.warn(String.format(message, args));
        }
    }

    public void warn(String message, String[] args) {
        if (isWarnEnabled()) {
            warn(message, Arrays.deepToString(args));
        }
    }

    public void warn(String message, Throwable cause) {
        if (isWarnEnabled()) {
            log.warn(message, cause);
        }
    }
}
