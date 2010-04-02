/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Presentation Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.PresentationQosPolicy#getAccess_scope <em>Access scope</em>}</li>
 *   <li>{@link OpenDDS.PresentationQosPolicy#isCoherent_access <em>Coherent access</em>}</li>
 *   <li>{@link OpenDDS.PresentationQosPolicy#isOrdered_access <em>Ordered access</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getPresentationQosPolicy()
 * @model
 * @generated
 */
public interface PresentationQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Access scope</b></em>' attribute.
     * The literals are from the enumeration {@link OpenDDS.PresentationQosPolicyAccessScopeKind}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Access scope</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Access scope</em>' attribute.
     * @see OpenDDS.PresentationQosPolicyAccessScopeKind
     * @see #setAccess_scope(PresentationQosPolicyAccessScopeKind)
     * @see OpenDDS.ModelPackage#getPresentationQosPolicy_Access_scope()
     * @model
     * @generated
     */
    PresentationQosPolicyAccessScopeKind getAccess_scope();

    /**
     * Sets the value of the '{@link OpenDDS.PresentationQosPolicy#getAccess_scope <em>Access scope</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Access scope</em>' attribute.
     * @see OpenDDS.PresentationQosPolicyAccessScopeKind
     * @see #getAccess_scope()
     * @generated
     */
    void setAccess_scope(PresentationQosPolicyAccessScopeKind value);

    /**
     * Returns the value of the '<em><b>Coherent access</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Coherent access</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Coherent access</em>' attribute.
     * @see #setCoherent_access(boolean)
     * @see OpenDDS.ModelPackage#getPresentationQosPolicy_Coherent_access()
     * @model
     * @generated
     */
    boolean isCoherent_access();

    /**
     * Sets the value of the '{@link OpenDDS.PresentationQosPolicy#isCoherent_access <em>Coherent access</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Coherent access</em>' attribute.
     * @see #isCoherent_access()
     * @generated
     */
    void setCoherent_access(boolean value);

    /**
     * Returns the value of the '<em><b>Ordered access</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Ordered access</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Ordered access</em>' attribute.
     * @see #setOrdered_access(boolean)
     * @see OpenDDS.ModelPackage#getPresentationQosPolicy_Ordered_access()
     * @model
     * @generated
     */
    boolean isOrdered_access();

    /**
     * Sets the value of the '{@link OpenDDS.PresentationQosPolicy#isOrdered_access <em>Ordered access</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Ordered access</em>' attribute.
     * @see #isOrdered_access()
     * @generated
     */
    void setOrdered_access(boolean value);

} // PresentationQosPolicy
