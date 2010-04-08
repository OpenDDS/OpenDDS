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
 * <em><b>Durability Service Qos Policy</b></em>'. <!-- end-user-doc
 * -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.DurabilityServiceQosPolicy#getHistory_depth <em>
 * History depth</em>}</li>
 * <li>{@link OpenDDS.DurabilityServiceQosPolicy#getHistory_kind <em>
 * History kind</em>}</li>
 * <li>{@link OpenDDS.DurabilityServiceQosPolicy#getMax_instances <em>
 * Max instances</em>}</li>
 * <li>{@link OpenDDS.DurabilityServiceQosPolicy#getMax_samples <em>
 * Max samples</em>}</li>
 * <li>
 * {@link OpenDDS.DurabilityServiceQosPolicy#getMax_samples_per_instance
 * <em>Max samples per instance</em>}</li>
 * <li>
 * {@link OpenDDS.DurabilityServiceQosPolicy#getService_cleanup_delay
 * <em>Service cleanup delay</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getDurabilityServiceQosPolicy()
 * @model
 * @generated
 */
public interface DurabilityServiceQosPolicy extends QosPolicy {
    /**
     * Returns the value of the '<em><b>History depth</b></em>'
     * attribute. The default value is <code>"1"</code>. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of the '<em>History depth</em>' attribute isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>History depth</em>' attribute.
     * @see #setHistory_depth(long)
     * @see OpenDDS.OpenDDSPackage#getDurabilityServiceQosPolicy_History_depth()
     * @model default="1"
     * @generated
     */
    long getHistory_depth();

    /**
     * Sets the value of the '
     * {@link OpenDDS.DurabilityServiceQosPolicy#getHistory_depth
     * <em>History depth</em>}' attribute. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>History depth</em>'
     *            attribute.
     * @see #getHistory_depth()
     * @generated
     */
    void setHistory_depth(long value);

    /**
     * Returns the value of the '<em><b>History kind</b></em>'
     * attribute. The default value is <code>"KEEP_LAST"</code>. The
     * literals are from the enumeration
     * {@link OpenDDS.HistoryQosPolicyKind}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>History kind</em>' attribute isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>History kind</em>' attribute.
     * @see OpenDDS.HistoryQosPolicyKind
     * @see #setHistory_kind(HistoryQosPolicyKind)
     * @see OpenDDS.OpenDDSPackage#getDurabilityServiceQosPolicy_History_kind()
     * @model default="KEEP_LAST"
     * @generated
     */
    HistoryQosPolicyKind getHistory_kind();

    /**
     * Sets the value of the '
     * {@link OpenDDS.DurabilityServiceQosPolicy#getHistory_kind
     * <em>History kind</em>}' attribute. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>History kind</em>'
     *            attribute.
     * @see OpenDDS.HistoryQosPolicyKind
     * @see #getHistory_kind()
     * @generated
     */
    void setHistory_kind(HistoryQosPolicyKind value);

    /**
     * Returns the value of the '<em><b>Max instances</b></em>'
     * attribute. The default value is <code>"-1"</code>. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Max instances</em>' attribute isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Max instances</em>' attribute.
     * @see #setMax_instances(long)
     * @see OpenDDS.OpenDDSPackage#getDurabilityServiceQosPolicy_Max_instances()
     * @model default="-1"
     * @generated
     */
    long getMax_instances();

    /**
     * Sets the value of the '
     * {@link OpenDDS.DurabilityServiceQosPolicy#getMax_instances
     * <em>Max instances</em>}' attribute. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>Max instances</em>'
     *            attribute.
     * @see #getMax_instances()
     * @generated
     */
    void setMax_instances(long value);

    /**
     * Returns the value of the '<em><b>Max samples</b></em>'
     * attribute. The default value is <code>"-1"</code>. <!--
     * begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Max samples</em>' attribute isn't
     * clear, there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Max samples</em>' attribute.
     * @see #setMax_samples(long)
     * @see OpenDDS.OpenDDSPackage#getDurabilityServiceQosPolicy_Max_samples()
     * @model default="-1"
     * @generated
     */
    long getMax_samples();

    /**
     * Sets the value of the '
     * {@link OpenDDS.DurabilityServiceQosPolicy#getMax_samples
     * <em>Max samples</em>}' attribute. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>Max samples</em>'
     *            attribute.
     * @see #getMax_samples()
     * @generated
     */
    void setMax_samples(long value);

    /**
     * Returns the value of the '
     * <em><b>Max samples per instance</b></em>' attribute. The
     * default value is <code>"-1"</code>. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Max samples per instance</em>'
     * attribute isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Max samples per instance</em>'
     *         attribute.
     * @see #setMax_samples_per_instance(long)
     * @see OpenDDS.OpenDDSPackage#getDurabilityServiceQosPolicy_Max_samples_per_instance()
     * @model default="-1"
     * @generated
     */
    long getMax_samples_per_instance();

    /**
     * Sets the value of the '
     * {@link OpenDDS.DurabilityServiceQosPolicy#getMax_samples_per_instance
     * <em>Max samples per instance</em>}' attribute. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '
     *            <em>Max samples per instance</em>' attribute.
     * @see #getMax_samples_per_instance()
     * @generated
     */
    void setMax_samples_per_instance(long value);

    /**
     * Returns the value of the '<em><b>Service cleanup delay</b></em>
     * ' containment reference. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Service cleanup delay</em>'
     * containment reference isn't clear, there really should be more
     * of a description here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Service cleanup delay</em>'
     *         containment reference.
     * @see #setService_cleanup_delay(Period)
     * @see OpenDDS.OpenDDSPackage#getDurabilityServiceQosPolicy_Service_cleanup_delay()
     * @model containment="true"
     * @generated
     */
    Period getService_cleanup_delay();

    /**
     * Sets the value of the '
     * {@link OpenDDS.DurabilityServiceQosPolicy#getService_cleanup_delay
     * <em>Service cleanup delay</em>}' containment reference. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @param value
     *            the new value of the '<em>Service cleanup delay</em>
     *            ' containment reference.
     * @see #getService_cleanup_delay()
     * @generated
     */
    void setService_cleanup_delay(Period value);

} // DurabilityServiceQosPolicy
