/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

/**
 * @author  Weiqi Gao
 */
public class IdentifiersTest {
    @Test
    public void testIsValidIdentifier() {
        assertFalse(Identifiers.isValidIdentifier(null));
        assertFalse(Identifiers.isValidIdentifier(""));
        assertFalse(Identifiers.isValidIdentifier(" "));
        assertFalse(Identifiers.isValidIdentifier("1"));
        assertFalse(Identifiers.isValidIdentifier("non Identifier"));

        assertFalse(Identifiers.isValidIdentifier("null"));
        assertFalse(Identifiers.isValidIdentifier("Null"));
        assertFalse(Identifiers.isValidIdentifier("NULL"));

        assertFalse(Identifiers.isValidIdentifier("true"));
        assertFalse(Identifiers.isValidIdentifier("True"));
        assertFalse(Identifiers.isValidIdentifier("TRUE"));

        assertFalse(Identifiers.isValidIdentifier("false"));
        assertFalse(Identifiers.isValidIdentifier("False"));
        assertFalse(Identifiers.isValidIdentifier("FALSE"));

        assertFalse(Identifiers.isValidIdentifier("not"));
        assertFalse(Identifiers.isValidIdentifier("Not"));
        assertFalse(Identifiers.isValidIdentifier("NOT"));

        assertFalse(Identifiers.isValidIdentifier("and"));
        assertFalse(Identifiers.isValidIdentifier("And"));
        assertFalse(Identifiers.isValidIdentifier("AND"));

        assertFalse(Identifiers.isValidIdentifier("or"));
        assertFalse(Identifiers.isValidIdentifier("Or"));
        assertFalse(Identifiers.isValidIdentifier("OR"));

        assertFalse(Identifiers.isValidIdentifier("between"));
        assertFalse(Identifiers.isValidIdentifier("Between"));
        assertFalse(Identifiers.isValidIdentifier("BETWEEN"));

        assertFalse(Identifiers.isValidIdentifier("like"));
        assertFalse(Identifiers.isValidIdentifier("Like"));
        assertFalse(Identifiers.isValidIdentifier("LIKE"));

        assertFalse(Identifiers.isValidIdentifier("in"));
        assertFalse(Identifiers.isValidIdentifier("In"));
        assertFalse(Identifiers.isValidIdentifier("IN"));

        assertFalse(Identifiers.isValidIdentifier("is"));
        assertFalse(Identifiers.isValidIdentifier("Is"));
        assertFalse(Identifiers.isValidIdentifier("IS"));

        assertFalse(Identifiers.isValidIdentifier("escape"));
        assertFalse(Identifiers.isValidIdentifier("Escape"));
        assertFalse(Identifiers.isValidIdentifier("ESCAPE"));

        assertTrue(Identifiers.isValidIdentifier("a"));
        assertTrue(Identifiers.isValidIdentifier("_"));
        assertTrue(Identifiers.isValidIdentifier("$"));
        assertTrue(Identifiers.isValidIdentifier("normalIdentifier_$1"));
    }

    @Test
    public void testPropertyNameTypes() {
        assertTrue(Identifiers.isJMSDefinedPropertyName("JMSX_SpecDefine"));
        assertTrue(Identifiers.isProviderSpecificPropertyName("JMS_ProviderSpecific"));
        assertTrue(Identifiers.isApplicationSpecificPropertyName("ApplicationSpecific"));
    }
}
