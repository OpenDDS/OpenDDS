/*
 * $Id$
 */
 
package org.opendds.jms.common.util;

import java.util.Arrays;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * @author  Steven Stallion
 * @version $Revision$
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

    public void debug(String message, Object... args) {
        if (log.isDebugEnabled()) {
            log.debug(String.format(message, args));
        }
    }

    public void debug(String message, String[] args) {
        if (log.isDebugEnabled()) {
            debug(message, Arrays.deepToString(args));
        }
    }

    public void debug(String message, Throwable cause) {
        if (log.isDebugEnabled()) {
            log.debug(message, cause);
        }
    }

    public void info(String message, Object... args) {
        if (log.isInfoEnabled()) {
            log.info(String.format(message, args));
        }
    }

    public void info(String message, String[] args) {
        if (log.isInfoEnabled()) {
            info(message, Arrays.deepToString(args));
        }
    }

    public void info(String message, Throwable cause) {
        if (log.isInfoEnabled()) {
            log.info(message, cause);
        }
    }

    public void error(String message, Object... args) {
        if (log.isErrorEnabled()) {
            log.error(String.format(message, args));
        }
    }

    public void error(String message, String[] args) {
        if (log.isErrorEnabled()) {
            error(message, Arrays.deepToString(args));
        }
    }

    public void error(String message, Throwable cause) {
        if (log.isErrorEnabled()) {
            log.error(message, cause);
        }
    }

    public void warn(String message, Object... args) {
        if (log.isWarnEnabled()) {
            log.warn(String.format(message, args));
        }
    }

    public void warn(String message, String[] args) {
        if (log.isWarnEnabled()) {
            warn(message, Arrays.deepToString(args));
        }
    }

    public void warn(String message, Throwable cause) {
        if (log.isWarnEnabled()) {
            log.warn(message, cause);
        }
    }
}
