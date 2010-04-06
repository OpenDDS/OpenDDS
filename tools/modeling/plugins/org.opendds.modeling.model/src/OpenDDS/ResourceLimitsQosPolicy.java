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
 * A representation of the model object '<em><b>Resource Limits Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.ResourceLimitsQosPolicy#getMax_instances <em>Max instances</em>}</li>
 *   <li>{@link OpenDDS.ResourceLimitsQosPolicy#getMax_samples <em>Max samples</em>}</li>
 *   <li>{@link OpenDDS.ResourceLimitsQosPolicy#getMax_samples_per_instance <em>Max samples per instance</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getResourceLimitsQosPolicy()
 * @model
 * @generated
 */
public interface ResourceLimitsQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Max instances</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Max instances</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Max instances</em>' attribute.
     * @see #setMax_instances(long)
     * @see OpenDDS.OpenDDSPackage#getResourceLimitsQosPolicy_Max_instances()
     * @model
     * @generated
     */
    long getMax_instances();

    /**
     * Sets the value of the '{@link OpenDDS.ResourceLimitsQosPolicy#getMax_instances <em>Max instances</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Max instances</em>' attribute.
     * @see #getMax_instances()
     * @generated
     */
    void setMax_instances(long value);

    /**
     * Returns the value of the '<em><b>Max samples</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Max samples</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Max samples</em>' attribute.
     * @see #setMax_samples(long)
     * @see OpenDDS.OpenDDSPackage#getResourceLimitsQosPolicy_Max_samples()
     * @model
     * @generated
     */
    long getMax_samples();

    /**
     * Sets the value of the '{@link OpenDDS.ResourceLimitsQosPolicy#getMax_samples <em>Max samples</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Max samples</em>' attribute.
     * @see #getMax_samples()
     * @generated
     */
    void setMax_samples(long value);

    /**
     * Returns the value of the '<em><b>Max samples per instance</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Max samples per instance</em>' attribute isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Max samples per instance</em>' attribute.
     * @see #setMax_samples_per_instance(long)
     * @see OpenDDS.OpenDDSPackage#getResourceLimitsQosPolicy_Max_samples_per_instance()
     * @model
     * @generated
     */
    long getMax_samples_per_instance();

    /**
     * Sets the value of the '{@link OpenDDS.ResourceLimitsQosPolicy#getMax_samples_per_instance <em>Max samples per instance</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Max samples per instance</em>' attribute.
     * @see #getMax_samples_per_instance()
     * @generated
     */
    void setMax_samples_per_instance(long value);

} // ResourceLimitsQosPolicy
