/*
 * $Id$
 */

package org.opendds.jms.management;

import org.opendds.jms.config.properties.DCPSProperties;
import org.opendds.jms.config.properties.InfoRepoProperties;
import org.opendds.jms.config.properties.ORBProperties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface DCPSInfoRepoServiceMBean
    extends ServiceMBean, InfoRepoProperties, DCPSProperties, ORBProperties {}
