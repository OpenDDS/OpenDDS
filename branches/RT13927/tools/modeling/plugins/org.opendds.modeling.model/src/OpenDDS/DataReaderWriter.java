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
 * A representation of the model object '<em><b>Data Reader Writer</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.DataReaderWriter#getDurability <em>Durability</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getDestination_order <em>Destination order</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getDeadline <em>Deadline</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getHistory <em>History</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getUser_data <em>User data</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getResource_limits <em>Resource limits</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getOwnership <em>Ownership</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getLiveliness <em>Liveliness</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getLatency_budget <em>Latency budget</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriter#getReliability <em>Reliability</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getDataReaderWriter()
 * @model abstract="true"
 * @generated
 */
public interface DataReaderWriter extends DomainEntity {
    /**
     * Returns the value of the '<em><b>Durability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Durability</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Durability</em>' reference.
     * @see #setDurability(DurabilityQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_Durability()
     * @model
     * @generated
     */
    DurabilityQosPolicy getDurability();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getDurability <em>Durability</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Durability</em>' reference.
     * @see #getDurability()
     * @generated
     */
    void setDurability(DurabilityQosPolicy value);

    /**
     * Returns the value of the '<em><b>Destination order</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Destination order</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Destination order</em>' reference.
     * @see #setDestination_order(DestinationOrderQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_Destination_order()
     * @model
     * @generated
     */
    DestinationOrderQosPolicy getDestination_order();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getDestination_order <em>Destination order</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Destination order</em>' reference.
     * @see #getDestination_order()
     * @generated
     */
    void setDestination_order(DestinationOrderQosPolicy value);

    /**
     * Returns the value of the '<em><b>Deadline</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Deadline</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Deadline</em>' reference.
     * @see #setDeadline(DeadlineQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_Deadline()
     * @model
     * @generated
     */
    DeadlineQosPolicy getDeadline();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getDeadline <em>Deadline</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Deadline</em>' reference.
     * @see #getDeadline()
     * @generated
     */
    void setDeadline(DeadlineQosPolicy value);

    /**
     * Returns the value of the '<em><b>History</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>History</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>History</em>' reference.
     * @see #setHistory(HistoryQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_History()
     * @model
     * @generated
     */
    HistoryQosPolicy getHistory();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getHistory <em>History</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>History</em>' reference.
     * @see #getHistory()
     * @generated
     */
    void setHistory(HistoryQosPolicy value);

    /**
     * Returns the value of the '<em><b>User data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>User data</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>User data</em>' reference.
     * @see #setUser_data(UserDataQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_User_data()
     * @model
     * @generated
     */
    UserDataQosPolicy getUser_data();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getUser_data <em>User data</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>User data</em>' reference.
     * @see #getUser_data()
     * @generated
     */
    void setUser_data(UserDataQosPolicy value);

    /**
     * Returns the value of the '<em><b>Resource limits</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Resource limits</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Resource limits</em>' reference.
     * @see #setResource_limits(ResourceLimitsQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_Resource_limits()
     * @model
     * @generated
     */
    ResourceLimitsQosPolicy getResource_limits();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getResource_limits <em>Resource limits</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Resource limits</em>' reference.
     * @see #getResource_limits()
     * @generated
     */
    void setResource_limits(ResourceLimitsQosPolicy value);

    /**
     * Returns the value of the '<em><b>Ownership</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Ownership</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Ownership</em>' reference.
     * @see #setOwnership(OwnershipQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_Ownership()
     * @model
     * @generated
     */
    OwnershipQosPolicy getOwnership();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getOwnership <em>Ownership</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Ownership</em>' reference.
     * @see #getOwnership()
     * @generated
     */
    void setOwnership(OwnershipQosPolicy value);

    /**
     * Returns the value of the '<em><b>Liveliness</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Liveliness</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Liveliness</em>' reference.
     * @see #setLiveliness(LivelinessQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_Liveliness()
     * @model
     * @generated
     */
    LivelinessQosPolicy getLiveliness();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getLiveliness <em>Liveliness</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Liveliness</em>' reference.
     * @see #getLiveliness()
     * @generated
     */
    void setLiveliness(LivelinessQosPolicy value);

    /**
     * Returns the value of the '<em><b>Latency budget</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Latency budget</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Latency budget</em>' reference.
     * @see #setLatency_budget(LatencyBudgetQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_Latency_budget()
     * @model
     * @generated
     */
    LatencyBudgetQosPolicy getLatency_budget();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getLatency_budget <em>Latency budget</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Latency budget</em>' reference.
     * @see #getLatency_budget()
     * @generated
     */
    void setLatency_budget(LatencyBudgetQosPolicy value);

    /**
     * Returns the value of the '<em><b>Reliability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Reliability</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Reliability</em>' reference.
     * @see #setReliability(ReliabilityQosPolicy)
     * @see OpenDDS.ModelPackage#getDataReaderWriter_Reliability()
     * @model
     * @generated
     */
    ReliabilityQosPolicy getReliability();

    /**
     * Sets the value of the '{@link OpenDDS.DataReaderWriter#getReliability <em>Reliability</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Reliability</em>' reference.
     * @see #getReliability()
     * @generated
     */
    void setReliability(ReliabilityQosPolicy value);

} // DataReaderWriter
