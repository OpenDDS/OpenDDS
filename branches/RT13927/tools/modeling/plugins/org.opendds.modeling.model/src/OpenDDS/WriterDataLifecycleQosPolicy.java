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
 * <!-- begin-user-doc --> A representation of the model object '
 * <em><b>Writer Data Lifecycle Qos Policy</b></em>'. <!--
 * end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.WriterDataLifecycleQosPolicy#isAutodispose_unregistered_instances <em>Autodispose unregistered instances</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getWriterDataLifecycleQosPolicy()
 * @model
 * @generated
 */
public interface WriterDataLifecycleQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>Autodispose unregistered instances</b></em>' attribute.
     * The default value is <code>"true"</code>.
     * <!-- begin-user-doc
     * -->
     * <p>
     * If the meaning of the '
     * <em>Autodispose unregistered instances</em>' attribute isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Autodispose unregistered instances</em>' attribute.
     * @see #setAutodispose_unregistered_instances(boolean)
     * @see OpenDDS.OpenDDSPackage#getWriterDataLifecycleQosPolicy_Autodispose_unregistered_instances()
     * @model default="true"
     * @generated
     */
    boolean isAutodispose_unregistered_instances();

    /**
     * Sets the value of the '
     * {@link OpenDDS.WriterDataLifecycleQosPolicy#isAutodispose_unregistered_instances
     * <em>Autodispose unregistered instances</em>}' attribute. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     *
     * @param value
     *            the new value of the '
     *            <em>Autodispose unregistered instances</em>'
     *            attribute.
     * @see #isAutodispose_unregistered_instances()
     * @generated
     */
    void setAutodispose_unregistered_instances(boolean value);

} // WriterDataLifecycleQosPolicy
