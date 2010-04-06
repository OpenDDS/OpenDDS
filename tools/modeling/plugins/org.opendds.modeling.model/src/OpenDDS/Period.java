/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.ecore.EObject;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Period</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.Period#getSeconds <em>Seconds</em>}</li>
 *   <li>{@link OpenDDS.Period#getNanoseconds <em>Nanoseconds</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getPeriod()
 * @model
 * @generated
 */
public interface Period extends EObject {
    /**
     * Returns the value of the '<em><b>Seconds</b></em>' attribute.
     * The default value is <code>"0"</code>.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Seconds</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Seconds</em>' attribute.
     * @see #setSeconds(long)
     * @see OpenDDS.OpenDDSPackage#getPeriod_Seconds()
     * @model default="0"
     * @generated
     */
    long getSeconds();

    /**
     * Sets the value of the '{@link OpenDDS.Period#getSeconds <em>Seconds</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Seconds</em>' attribute.
     * @see #getSeconds()
     * @generated
     */
    void setSeconds(long value);

    /**
     * Returns the value of the '<em><b>Nanoseconds</b></em>' attribute.
     * The default value is <code>"0"</code>.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Nanoseconds</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Nanoseconds</em>' attribute.
     * @see #setNanoseconds(long)
     * @see OpenDDS.OpenDDSPackage#getPeriod_Nanoseconds()
     * @model default="0"
     * @generated
     */
    long getNanoseconds();

    /**
     * Sets the value of the '{@link OpenDDS.Period#getNanoseconds <em>Nanoseconds</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Nanoseconds</em>' attribute.
     * @see #getNanoseconds()
     * @generated
     */
    void setNanoseconds(long value);

} // Period
