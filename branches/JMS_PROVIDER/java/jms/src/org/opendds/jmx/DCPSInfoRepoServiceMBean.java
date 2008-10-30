/*
 * $Id$
 */

package org.opendds.jmx;

import org.opendds.config.props.DCPSProperties;
import org.opendds.config.props.InfoRepoProperties;
import org.opendds.config.props.ORBProperties;

/**
 * @author  Steven Stallion
 * @version $Revision$
 */
public interface DCPSInfoRepoServiceMBean
    extends ServiceMBean, InfoRepoProperties, DCPSProperties, ORBProperties {}
