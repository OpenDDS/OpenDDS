/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.InternalEObject;
import org.eclipse.emf.ecore.impl.ENotificationImpl;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Data Reader Writer</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getDurability <em>Durability</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getDestination_order <em>Destination order</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getDeadline <em>Deadline</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getHistory <em>History</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getUser_data <em>User data</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getResource_limits <em>Resource limits</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getOwnership <em>Ownership</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getLiveliness <em>Liveliness</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getLatency_budget <em>Latency budget</em>}</li>
 *   <li>{@link OpenDDS.DataReaderWriterImpl#getReliability <em>Reliability</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public abstract class DataReaderWriterImpl extends DomainEntityImpl implements DataReaderWriter {
    /**
     * The cached value of the '{@link #getDurability() <em>Durability</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getDurability()
     * @generated
     * @ordered
     */
    protected DurabilityQosPolicy durability;

    /**
     * The cached value of the '{@link #getDestination_order() <em>Destination order</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getDestination_order()
     * @generated
     * @ordered
     */
    protected DestinationOrderQosPolicy destination_order;

    /**
     * The cached value of the '{@link #getDeadline() <em>Deadline</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getDeadline()
     * @generated
     * @ordered
     */
    protected DeadlineQosPolicy deadline;

    /**
     * The cached value of the '{@link #getHistory() <em>History</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getHistory()
     * @generated
     * @ordered
     */
    protected HistoryQosPolicy history;

    /**
     * The cached value of the '{@link #getUser_data() <em>User data</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getUser_data()
     * @generated
     * @ordered
     */
    protected UserDataQosPolicy user_data;

    /**
     * The cached value of the '{@link #getResource_limits() <em>Resource limits</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getResource_limits()
     * @generated
     * @ordered
     */
    protected ResourceLimitsQosPolicy resource_limits;

    /**
     * The cached value of the '{@link #getOwnership() <em>Ownership</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getOwnership()
     * @generated
     * @ordered
     */
    protected OwnershipQosPolicy ownership;

    /**
     * The cached value of the '{@link #getLiveliness() <em>Liveliness</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getLiveliness()
     * @generated
     * @ordered
     */
    protected LivelinessQosPolicy liveliness;

    /**
     * The cached value of the '{@link #getLatency_budget() <em>Latency budget</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getLatency_budget()
     * @generated
     * @ordered
     */
    protected LatencyBudgetQosPolicy latency_budget;

    /**
     * The cached value of the '{@link #getReliability() <em>Reliability</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getReliability()
     * @generated
     * @ordered
     */
    protected ReliabilityQosPolicy reliability;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected DataReaderWriterImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return ModelPackage.Literals.DATA_READER_WRITER;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DurabilityQosPolicy getDurability() {
        if (durability != null && durability.eIsProxy()) {
            InternalEObject oldDurability = (InternalEObject) durability;
            durability = (DurabilityQosPolicy) eResolveProxy(oldDurability);
            if (durability != oldDurability) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__DURABILITY, oldDurability, durability));
                }
            }
        }
        return durability;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DurabilityQosPolicy basicGetDurability() {
        return durability;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setDurability(DurabilityQosPolicy newDurability) {
        DurabilityQosPolicy oldDurability = durability;
        durability = newDurability;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__DURABILITY,
                    oldDurability, durability));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DestinationOrderQosPolicy getDestination_order() {
        if (destination_order != null && destination_order.eIsProxy()) {
            InternalEObject oldDestination_order = (InternalEObject) destination_order;
            destination_order = (DestinationOrderQosPolicy) eResolveProxy(oldDestination_order);
            if (destination_order != oldDestination_order) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__DESTINATION_ORDER, oldDestination_order, destination_order));
                }
            }
        }
        return destination_order;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DestinationOrderQosPolicy basicGetDestination_order() {
        return destination_order;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setDestination_order(DestinationOrderQosPolicy newDestination_order) {
        DestinationOrderQosPolicy oldDestination_order = destination_order;
        destination_order = newDestination_order;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__DESTINATION_ORDER,
                    oldDestination_order, destination_order));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DeadlineQosPolicy getDeadline() {
        if (deadline != null && deadline.eIsProxy()) {
            InternalEObject oldDeadline = (InternalEObject) deadline;
            deadline = (DeadlineQosPolicy) eResolveProxy(oldDeadline);
            if (deadline != oldDeadline) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__DEADLINE, oldDeadline, deadline));
                }
            }
        }
        return deadline;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DeadlineQosPolicy basicGetDeadline() {
        return deadline;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setDeadline(DeadlineQosPolicy newDeadline) {
        DeadlineQosPolicy oldDeadline = deadline;
        deadline = newDeadline;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__DEADLINE,
                    oldDeadline, deadline));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public HistoryQosPolicy getHistory() {
        if (history != null && history.eIsProxy()) {
            InternalEObject oldHistory = (InternalEObject) history;
            history = (HistoryQosPolicy) eResolveProxy(oldHistory);
            if (history != oldHistory) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE, ModelPackage.DATA_READER_WRITER__HISTORY,
                            oldHistory, history));
                }
            }
        }
        return history;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public HistoryQosPolicy basicGetHistory() {
        return history;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setHistory(HistoryQosPolicy newHistory) {
        HistoryQosPolicy oldHistory = history;
        history = newHistory;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__HISTORY, oldHistory,
                    history));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public UserDataQosPolicy getUser_data() {
        if (user_data != null && user_data.eIsProxy()) {
            InternalEObject oldUser_data = (InternalEObject) user_data;
            user_data = (UserDataQosPolicy) eResolveProxy(oldUser_data);
            if (user_data != oldUser_data) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__USER_DATA, oldUser_data, user_data));
                }
            }
        }
        return user_data;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public UserDataQosPolicy basicGetUser_data() {
        return user_data;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setUser_data(UserDataQosPolicy newUser_data) {
        UserDataQosPolicy oldUser_data = user_data;
        user_data = newUser_data;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__USER_DATA,
                    oldUser_data, user_data));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ResourceLimitsQosPolicy getResource_limits() {
        if (resource_limits != null && resource_limits.eIsProxy()) {
            InternalEObject oldResource_limits = (InternalEObject) resource_limits;
            resource_limits = (ResourceLimitsQosPolicy) eResolveProxy(oldResource_limits);
            if (resource_limits != oldResource_limits) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__RESOURCE_LIMITS, oldResource_limits, resource_limits));
                }
            }
        }
        return resource_limits;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ResourceLimitsQosPolicy basicGetResource_limits() {
        return resource_limits;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setResource_limits(ResourceLimitsQosPolicy newResource_limits) {
        ResourceLimitsQosPolicy oldResource_limits = resource_limits;
        resource_limits = newResource_limits;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__RESOURCE_LIMITS,
                    oldResource_limits, resource_limits));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OwnershipQosPolicy getOwnership() {
        if (ownership != null && ownership.eIsProxy()) {
            InternalEObject oldOwnership = (InternalEObject) ownership;
            ownership = (OwnershipQosPolicy) eResolveProxy(oldOwnership);
            if (ownership != oldOwnership) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__OWNERSHIP, oldOwnership, ownership));
                }
            }
        }
        return ownership;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OwnershipQosPolicy basicGetOwnership() {
        return ownership;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setOwnership(OwnershipQosPolicy newOwnership) {
        OwnershipQosPolicy oldOwnership = ownership;
        ownership = newOwnership;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__OWNERSHIP,
                    oldOwnership, ownership));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LivelinessQosPolicy getLiveliness() {
        if (liveliness != null && liveliness.eIsProxy()) {
            InternalEObject oldLiveliness = (InternalEObject) liveliness;
            liveliness = (LivelinessQosPolicy) eResolveProxy(oldLiveliness);
            if (liveliness != oldLiveliness) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__LIVELINESS, oldLiveliness, liveliness));
                }
            }
        }
        return liveliness;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LivelinessQosPolicy basicGetLiveliness() {
        return liveliness;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setLiveliness(LivelinessQosPolicy newLiveliness) {
        LivelinessQosPolicy oldLiveliness = liveliness;
        liveliness = newLiveliness;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__LIVELINESS,
                    oldLiveliness, liveliness));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LatencyBudgetQosPolicy getLatency_budget() {
        if (latency_budget != null && latency_budget.eIsProxy()) {
            InternalEObject oldLatency_budget = (InternalEObject) latency_budget;
            latency_budget = (LatencyBudgetQosPolicy) eResolveProxy(oldLatency_budget);
            if (latency_budget != oldLatency_budget) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__LATENCY_BUDGET, oldLatency_budget, latency_budget));
                }
            }
        }
        return latency_budget;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LatencyBudgetQosPolicy basicGetLatency_budget() {
        return latency_budget;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setLatency_budget(LatencyBudgetQosPolicy newLatency_budget) {
        LatencyBudgetQosPolicy oldLatency_budget = latency_budget;
        latency_budget = newLatency_budget;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__LATENCY_BUDGET,
                    oldLatency_budget, latency_budget));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ReliabilityQosPolicy getReliability() {
        if (reliability != null && reliability.eIsProxy()) {
            InternalEObject oldReliability = (InternalEObject) reliability;
            reliability = (ReliabilityQosPolicy) eResolveProxy(oldReliability);
            if (reliability != oldReliability) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            ModelPackage.DATA_READER_WRITER__RELIABILITY, oldReliability, reliability));
                }
            }
        }
        return reliability;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ReliabilityQosPolicy basicGetReliability() {
        return reliability;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setReliability(ReliabilityQosPolicy newReliability) {
        ReliabilityQosPolicy oldReliability = reliability;
        reliability = newReliability;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.DATA_READER_WRITER__RELIABILITY,
                    oldReliability, reliability));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case ModelPackage.DATA_READER_WRITER__DURABILITY:
                if (resolve) {
                    return getDurability();
                }
                return basicGetDurability();
            case ModelPackage.DATA_READER_WRITER__DESTINATION_ORDER:
                if (resolve) {
                    return getDestination_order();
                }
                return basicGetDestination_order();
            case ModelPackage.DATA_READER_WRITER__DEADLINE:
                if (resolve) {
                    return getDeadline();
                }
                return basicGetDeadline();
            case ModelPackage.DATA_READER_WRITER__HISTORY:
                if (resolve) {
                    return getHistory();
                }
                return basicGetHistory();
            case ModelPackage.DATA_READER_WRITER__USER_DATA:
                if (resolve) {
                    return getUser_data();
                }
                return basicGetUser_data();
            case ModelPackage.DATA_READER_WRITER__RESOURCE_LIMITS:
                if (resolve) {
                    return getResource_limits();
                }
                return basicGetResource_limits();
            case ModelPackage.DATA_READER_WRITER__OWNERSHIP:
                if (resolve) {
                    return getOwnership();
                }
                return basicGetOwnership();
            case ModelPackage.DATA_READER_WRITER__LIVELINESS:
                if (resolve) {
                    return getLiveliness();
                }
                return basicGetLiveliness();
            case ModelPackage.DATA_READER_WRITER__LATENCY_BUDGET:
                if (resolve) {
                    return getLatency_budget();
                }
                return basicGetLatency_budget();
            case ModelPackage.DATA_READER_WRITER__RELIABILITY:
                if (resolve) {
                    return getReliability();
                }
                return basicGetReliability();
        }
        return super.eGet(featureID, resolve, coreType);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public void eSet(int featureID, Object newValue) {
        switch (featureID) {
            case ModelPackage.DATA_READER_WRITER__DURABILITY:
                setDurability((DurabilityQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__DESTINATION_ORDER:
                setDestination_order((DestinationOrderQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__DEADLINE:
                setDeadline((DeadlineQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__HISTORY:
                setHistory((HistoryQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__USER_DATA:
                setUser_data((UserDataQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__RESOURCE_LIMITS:
                setResource_limits((ResourceLimitsQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__OWNERSHIP:
                setOwnership((OwnershipQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__LIVELINESS:
                setLiveliness((LivelinessQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__LATENCY_BUDGET:
                setLatency_budget((LatencyBudgetQosPolicy) newValue);
                return;
            case ModelPackage.DATA_READER_WRITER__RELIABILITY:
                setReliability((ReliabilityQosPolicy) newValue);
                return;
        }
        super.eSet(featureID, newValue);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public void eUnset(int featureID) {
        switch (featureID) {
            case ModelPackage.DATA_READER_WRITER__DURABILITY:
                setDurability((DurabilityQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__DESTINATION_ORDER:
                setDestination_order((DestinationOrderQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__DEADLINE:
                setDeadline((DeadlineQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__HISTORY:
                setHistory((HistoryQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__USER_DATA:
                setUser_data((UserDataQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__RESOURCE_LIMITS:
                setResource_limits((ResourceLimitsQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__OWNERSHIP:
                setOwnership((OwnershipQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__LIVELINESS:
                setLiveliness((LivelinessQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__LATENCY_BUDGET:
                setLatency_budget((LatencyBudgetQosPolicy) null);
                return;
            case ModelPackage.DATA_READER_WRITER__RELIABILITY:
                setReliability((ReliabilityQosPolicy) null);
                return;
        }
        super.eUnset(featureID);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public boolean eIsSet(int featureID) {
        switch (featureID) {
            case ModelPackage.DATA_READER_WRITER__DURABILITY:
                return durability != null;
            case ModelPackage.DATA_READER_WRITER__DESTINATION_ORDER:
                return destination_order != null;
            case ModelPackage.DATA_READER_WRITER__DEADLINE:
                return deadline != null;
            case ModelPackage.DATA_READER_WRITER__HISTORY:
                return history != null;
            case ModelPackage.DATA_READER_WRITER__USER_DATA:
                return user_data != null;
            case ModelPackage.DATA_READER_WRITER__RESOURCE_LIMITS:
                return resource_limits != null;
            case ModelPackage.DATA_READER_WRITER__OWNERSHIP:
                return ownership != null;
            case ModelPackage.DATA_READER_WRITER__LIVELINESS:
                return liveliness != null;
            case ModelPackage.DATA_READER_WRITER__LATENCY_BUDGET:
                return latency_budget != null;
            case ModelPackage.DATA_READER_WRITER__RELIABILITY:
                return reliability != null;
        }
        return super.eIsSet(featureID);
    }

} //DataReaderWriterImpl
