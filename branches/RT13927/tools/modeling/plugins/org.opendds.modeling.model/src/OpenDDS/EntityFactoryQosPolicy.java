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
 * A representation of the model object '<em><b>Entity Factory Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.EntityFactoryQosPolicy#isAutoenable_created_entities <em>Autoenable created entities</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getEntityFactoryQosPolicy()
 * @model
 * @generated
 */
public interface EntityFactoryQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Autoenable created entities</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Autoenable created entities</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Autoenable created entities</em>' attribute.
     * @see #setAutoenable_created_entities(boolean)
     * @see OpenDDS.ModelPackage#getEntityFactoryQosPolicy_Autoenable_created_entities()
     * @model
     * @generated
     */
    boolean isAutoenable_created_entities();

    /**
     * Sets the value of the '{@link OpenDDS.EntityFactoryQosPolicy#isAutoenable_created_entities <em>Autoenable created entities</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Autoenable created entities</em>' attribute.
     * @see #isAutoenable_created_entities()
     * @generated
     */
    void setAutoenable_created_entities(boolean value);

} // EntityFactoryQosPolicy
