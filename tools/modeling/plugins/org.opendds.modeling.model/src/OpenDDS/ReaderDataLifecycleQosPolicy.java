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
 * A representation of the model object '<em><b>Reader Data Lifecycle Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.ReaderDataLifecycleQosPolicy#getAutopurge_nowriter_samples_delay <em>Autopurge nowriter samples delay</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getReaderDataLifecycleQosPolicy()
 * @model
 * @generated
 */
public interface ReaderDataLifecycleQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Autopurge nowriter samples delay</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Autopurge nowriter samples delay</em>' containment reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Autopurge nowriter samples delay</em>' containment reference.
     * @see #setAutopurge_nowriter_samples_delay(Period)
     * @see OpenDDS.ModelPackage#getReaderDataLifecycleQosPolicy_Autopurge_nowriter_samples_delay()
     * @model containment="true"
     * @generated
     */
    Period getAutopurge_nowriter_samples_delay();

    /**
     * Sets the value of the '{@link OpenDDS.ReaderDataLifecycleQosPolicy#getAutopurge_nowriter_samples_delay <em>Autopurge nowriter samples delay</em>}' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Autopurge nowriter samples delay</em>' containment reference.
     * @see #getAutopurge_nowriter_samples_delay()
     * @generated
     */
    void setAutopurge_nowriter_samples_delay(Period value);

} // ReaderDataLifecycleQosPolicy
