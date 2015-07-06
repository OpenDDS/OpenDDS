/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package org.opendds.jms.common.lang;

import org.junit.Test;

/**
 * @author  Steven Stallion
 */
public class StringsTest {

    @Test(expected = AssertionError.class)
    public void hasLengthAssertions() {
        Strings.hasLength(null, -1);
    }

    @Test
    public void hasLengthOnNull() {
        assert !Strings.hasLength(null, 0);
    }

    @Test
    public void hasLengthOnEmpty() {
        assert Strings.hasLength("", 0);
    }

    @Test
    public void hasLengthOnNotEmpty() {
        assert Strings.hasLength("X", 1);
    }

    @Test
    public void hasLengthWithTrim() {
        assert !Strings.hasLength(" ", 1, true);
    }

    @Test
    public void hasLengthWithRange() {
        assert Strings.hasLength("XXX", 0);
        assert Strings.hasLength("XXX", 1);
        assert Strings.hasLength("XXX", 2);
        assert Strings.hasLength("XXX", 3);

        assert !Strings.hasLength("XXX", 4);
    }

    @Test
    public void isEmptyOnNull() {
        assert Strings.isEmpty(null);
    }

    @Test
    public void isEmptyOnEmpty() {
        assert Strings.isEmpty("");
    }

    @Test
    public void isEmptyOnEmptyTrim() {
        assert Strings.isEmpty("  ", true);
    }

    @Test
    public void isEmptyOnNotEmpty() {
        assert !Strings.isEmpty("X");
    }

    @Test
    public void isEmptyOnNotEmptyTrim() {
        assert !Strings.isEmpty(" X ", true);
    }
}
