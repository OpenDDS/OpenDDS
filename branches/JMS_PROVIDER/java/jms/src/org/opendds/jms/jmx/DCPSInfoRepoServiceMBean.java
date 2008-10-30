/*
 * $Id$
 */

package org.opendds.jms.jmx;

import org.opendds.jms.config.props.DCPSProperties;
import org.opendds.jms.config.props.InfoRepoProperties;
import org.opendds.jms.config.props.ORBProperties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface DCPSInfoRepoServiceMBean
    extends ServiceMBean, InfoRepoProperties, DCPSProperties, ORBProperties {}
