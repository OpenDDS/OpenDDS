/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms;

import OpenDDS.JMS.MessagePayloadDataReader;

/**
 * An alternative representation of a MessagePayload that is used by SessionImpl during
 * the acknowledgement phase of the Message consumption
 *
 * @author  Weiqi Gao
 * @version $Revision$
 */
public class DataReaderHandlePair {
    private MessagePayloadDataReader dataReader;
    private int instanceHandle;

    public DataReaderHandlePair(MessagePayloadDataReader dataReader, int instanceHandle) {
        this.dataReader = dataReader;
        this.instanceHandle = instanceHandle;
    }

    public MessagePayloadDataReader getDataReader() {
        return dataReader;
    }

    public int getInstanceHandle() {
        return instanceHandle;
    }
}
