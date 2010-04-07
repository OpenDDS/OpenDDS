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
 * <em><b>Data Reader</b></em>'. <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.DataReader#getTopic <em>Topic</em>}</li>
 *   <li>{@link OpenDDS.DataReader#getReader_data_lifecycle <em>Reader data lifecycle</em>}</li>
 *   <li>{@link OpenDDS.DataReader#getTransport_priority <em>Transport priority</em>}</li>
 *   <li>{@link OpenDDS.DataReader#getDurability_service <em>Durability service</em>}</li>
 *   <li>{@link OpenDDS.DataReader#getOwnership_strength <em>Ownership strength</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getDataReader()
 * @model
 * @generated
 */
public interface DataReader extends DataReaderWriter {
    /**
     * Returns the value of the '<em><b>Topic</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Topic</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Topic</em>' reference.
     * @see #setTopic(TopicDescription)
     * @see OpenDDS.OpenDDSPackage#getDataReader_Topic()
     * @model required="true"
     * @generated
     */
    TopicDescription getTopic();

    /**
     * Sets the value of the '{@link OpenDDS.DataReader#getTopic <em>Topic</em>}' reference.
     * <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * @param value the new value of the '<em>Topic</em>' reference.
     * @see #getTopic()
     * @generated
     */
    void setTopic(TopicDescription value);

    /**
     * Returns the value of the '<em><b>Reader data lifecycle</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Reader data lifecycle</em>'
     * reference isn't clear, there really should be more of a
     * description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Reader data lifecycle</em>' reference.
     * @see #setReader_data_lifecycle(ReaderDataLifecycleQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getDataReader_Reader_data_lifecycle()
     * @model
     * @generated
     */
    ReaderDataLifecycleQosPolicy getReader_data_lifecycle();

    /**
     * Sets the value of the '{@link OpenDDS.DataReader#getReader_data_lifecycle <em>Reader data lifecycle</em>}' reference.
     * <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * @param value the new value of the '<em>Reader data lifecycle</em>' reference.
     * @see #getReader_data_lifecycle()
     * @generated
     */
    void setReader_data_lifecycle(ReaderDataLifecycleQosPolicy value);

    /**
     * Returns the value of the '<em><b>Transport priority</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Transport priority</em>' reference
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Transport priority</em>' reference.
     * @see #setTransport_priority(TransportPriorityQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getDataReader_Transport_priority()
     * @model
     * @generated
     */
    TransportPriorityQosPolicy getTransport_priority();

    /**
     * Sets the value of the '{@link OpenDDS.DataReader#getTransport_priority <em>Transport priority</em>}' reference.
     * <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * @param value the new value of the '<em>Transport priority</em>' reference.
     * @see #getTransport_priority()
     * @generated
     */
    void setTransport_priority(TransportPriorityQosPolicy value);

    /**
     * Returns the value of the '<em><b>Durability service</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Durability service</em>' reference
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Durability service</em>' reference.
     * @see #setDurability_service(DurabilityServiceQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getDataReader_Durability_service()
     * @model
     * @generated
     */
    DurabilityServiceQosPolicy getDurability_service();

    /**
     * Sets the value of the '{@link OpenDDS.DataReader#getDurability_service <em>Durability service</em>}' reference.
     * <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * @param value the new value of the '<em>Durability service</em>' reference.
     * @see #getDurability_service()
     * @generated
     */
    void setDurability_service(DurabilityServiceQosPolicy value);

    /**
     * Returns the value of the '<em><b>Ownership strength</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Ownership strength</em>' reference
     * isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Ownership strength</em>' reference.
     * @see #setOwnership_strength(OwnershipStrengthQosPolicy)
     * @see OpenDDS.OpenDDSPackage#getDataReader_Ownership_strength()
     * @model
     * @generated
     */
    OwnershipStrengthQosPolicy getOwnership_strength();

    /**
     * Sets the value of the '{@link OpenDDS.DataReader#getOwnership_strength <em>Ownership strength</em>}' reference.
     * <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * @param value the new value of the '<em>Ownership strength</em>' reference.
     * @see #getOwnership_strength()
     * @generated
     */
    void setOwnership_strength(OwnershipStrengthQosPolicy value);

} // DataReader
