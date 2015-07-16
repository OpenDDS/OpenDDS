/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common;

import java.util.Arrays;
import java.util.List;

import org.opendds.jms.common.lang.Strings;

/**
 * @author  Weiqi Gao
 */
public class Identifiers {
    protected static List<String> forbidenNames = Arrays.asList("NULL", "TRUE", "FALSE",
        "NOT", "AND", "OR", "BETWEEN", "LIKE", "IN", "IS", "ESCAPE");

    /**
     * See JMS 1.1, 3.8.1.1 Message Selector Syntax section.
     * @param s A potential identifier
     * @return True if it is a valid identifier, false otherwise
     */
    public static boolean isValidIdentifier(String s) {
        if (Strings.isEmpty(s)) return false;
        if (!Character.isJavaIdentifierStart(s.charAt(0))) return false;
        for (int i = 0; i < s.length(); i++) {
            if (!Character.isJavaIdentifierPart(s.charAt(i))) return false;
        }
        for (String name : forbidenNames) {
            if (s.equalsIgnoreCase(name)) return false;
        }
        return true;
    }

    public static boolean isJMSDefinedPropertyName(String s) {
        if (!isValidIdentifier(s)) return false;
        return s.startsWith("JMSX");
    }

    public static boolean isProviderSpecificPropertyName(String s) {
        if (!isValidIdentifier(s)) return false;
        return s.startsWith("JMS_");
    }

    public static boolean isApplicationSpecificPropertyName(String s) {
        if (!isValidIdentifier(s)) return false;
        return !s.startsWith("JMS");
    }
}
