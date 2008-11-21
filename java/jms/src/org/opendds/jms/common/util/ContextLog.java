/*
 * $Id$
 */
 
package org.opendds.jms.common.util;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public class ContextLog {
    private Log log;

    private String context;
    private Object discriminator;

    public ContextLog(String context, Object discriminator) {
        this.context = context;
        this.discriminator = discriminator;
        log = LogFactory.getLog(getLogName());
    }

    public String getContext() {
        return context;
    }

    public Object getDiscriminator() {
        return discriminator;
    }

    public String getLogName() {
        return String.format("%s-%s", context, discriminator);
    }

    public void debug(String message, Object... args) {
        if (log.isDebugEnabled()) {
            log.debug(String.format(message, args));
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

    public void warn(String message, Throwable cause) {
        if (log.isWarnEnabled()) {
            log.warn(message, cause);
        }
    }
}
