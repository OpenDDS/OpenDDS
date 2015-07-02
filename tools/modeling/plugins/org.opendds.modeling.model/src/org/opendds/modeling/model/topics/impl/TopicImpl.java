/**
 * <copyright>
 * </copyright>
 *
 */
package org.opendds.modeling.model.topics.impl;

import org.eclipse.emf.common.notify.Notification;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.InternalEObject;

import org.eclipse.emf.ecore.impl.ENotificationImpl;

import org.opendds.modeling.model.domain.impl.DomainEntityImpl;

import org.opendds.modeling.model.qos.DeadlineQosPolicy;
import org.opendds.modeling.model.qos.DestinationOrderQosPolicy;
import org.opendds.modeling.model.qos.DurabilityQosPolicy;
import org.opendds.modeling.model.qos.DurabilityServiceQosPolicy;
import org.opendds.modeling.model.qos.HistoryQosPolicy;
import org.opendds.modeling.model.qos.LatencyBudgetQosPolicy;
import org.opendds.modeling.model.qos.LifespanQosPolicy;
import org.opendds.modeling.model.qos.LivelinessQosPolicy;
import org.opendds.modeling.model.qos.OwnershipQosPolicy;
import org.opendds.modeling.model.qos.ReliabilityQosPolicy;
import org.opendds.modeling.model.qos.ResourceLimitsQosPolicy;
import org.opendds.modeling.model.qos.TopicDataQosPolicy;
import org.opendds.modeling.model.qos.TransportPriorityQosPolicy;

import org.opendds.modeling.model.topics.Topic;
import org.opendds.modeling.model.topics.TopicDescription;
import org.opendds.modeling.model.topics.TopicsPackage;

import org.opendds.modeling.model.types.Struct;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Topic</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getDeadline <em>Deadline</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getDestination_order <em>Destination order</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getDurability <em>Durability</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getDurability_service <em>Durability service</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getHistory <em>History</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getLatency_budget <em>Latency budget</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getLiveliness <em>Liveliness</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getOwnership <em>Ownership</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getReliability <em>Reliability</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getResource_limits <em>Resource limits</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getTopic_data <em>Topic data</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getTransport_priority <em>Transport priority</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getLifespan <em>Lifespan</em>}</li>
 *   <li>{@link org.opendds.modeling.model.topics.impl.TopicImpl#getDatatype <em>Datatype</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class TopicImpl extends DomainEntityImpl implements Topic {
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
	 * The cached value of the '{@link #getDestination_order() <em>Destination order</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getDestination_order()
	 * @generated
	 * @ordered
	 */
	protected DestinationOrderQosPolicy destination_order;

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
	 * The cached value of the '{@link #getDurability_service() <em>Durability service</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getDurability_service()
	 * @generated
	 * @ordered
	 */
	protected DurabilityServiceQosPolicy durability_service;

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
	 * The cached value of the '{@link #getLatency_budget() <em>Latency budget</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getLatency_budget()
	 * @generated
	 * @ordered
	 */
	protected LatencyBudgetQosPolicy latency_budget;

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
	 * The cached value of the '{@link #getOwnership() <em>Ownership</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getOwnership()
	 * @generated
	 * @ordered
	 */
	protected OwnershipQosPolicy ownership;

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
	 * The cached value of the '{@link #getResource_limits() <em>Resource limits</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getResource_limits()
	 * @generated
	 * @ordered
	 */
	protected ResourceLimitsQosPolicy resource_limits;

	/**
	 * The cached value of the '{@link #getTopic_data() <em>Topic data</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getTopic_data()
	 * @generated
	 * @ordered
	 */
	protected TopicDataQosPolicy topic_data;

	/**
	 * The cached value of the '{@link #getTransport_priority() <em>Transport priority</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getTransport_priority()
	 * @generated
	 * @ordered
	 */
	protected TransportPriorityQosPolicy transport_priority;

	/**
	 * The cached value of the '{@link #getLifespan() <em>Lifespan</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getLifespan()
	 * @generated
	 * @ordered
	 */
	protected LifespanQosPolicy lifespan;

	/**
	 * The cached value of the '{@link #getDatatype() <em>Datatype</em>}' reference.
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @see #getDatatype()
	 * @generated
	 * @ordered
	 */
	protected Struct datatype;

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	protected TopicImpl() {
		super();
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	protected EClass eStaticClass() {
		return TopicsPackage.Literals.TOPIC;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public Struct getDatatype() {
		if (datatype != null && datatype.eIsProxy()) {
			InternalEObject oldDatatype = (InternalEObject) datatype;
			datatype = (Struct) eResolveProxy(oldDatatype);
			if (datatype != oldDatatype) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__DATATYPE, oldDatatype,
							datatype));
			}
		}
		return datatype;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public Struct basicGetDatatype() {
		return datatype;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setDatatype(Struct newDatatype) {
		Struct oldDatatype = datatype;
		datatype = newDatatype;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__DATATYPE, oldDatatype, datatype));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__DEADLINE, oldDeadline,
							deadline));
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
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__DEADLINE, oldDeadline, deadline));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__DESTINATION_ORDER,
							oldDestination_order, destination_order));
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
	public void setDestination_order(
			DestinationOrderQosPolicy newDestination_order) {
		DestinationOrderQosPolicy oldDestination_order = destination_order;
		destination_order = newDestination_order;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__DESTINATION_ORDER,
					oldDestination_order, destination_order));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__DURABILITY, oldDurability,
							durability));
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
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__DURABILITY, oldDurability, durability));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public DurabilityServiceQosPolicy getDurability_service() {
		if (durability_service != null && durability_service.eIsProxy()) {
			InternalEObject oldDurability_service = (InternalEObject) durability_service;
			durability_service = (DurabilityServiceQosPolicy) eResolveProxy(oldDurability_service);
			if (durability_service != oldDurability_service) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__DURABILITY_SERVICE,
							oldDurability_service, durability_service));
			}
		}
		return durability_service;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public DurabilityServiceQosPolicy basicGetDurability_service() {
		return durability_service;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setDurability_service(
			DurabilityServiceQosPolicy newDurability_service) {
		DurabilityServiceQosPolicy oldDurability_service = durability_service;
		durability_service = newDurability_service;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__DURABILITY_SERVICE,
					oldDurability_service, durability_service));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__HISTORY, oldHistory, history));
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
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__HISTORY, oldHistory, history));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__LATENCY_BUDGET,
							oldLatency_budget, latency_budget));
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
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__LATENCY_BUDGET, oldLatency_budget,
					latency_budget));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__LIVELINESS, oldLiveliness,
							liveliness));
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
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__LIVELINESS, oldLiveliness, liveliness));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__OWNERSHIP, oldOwnership,
							ownership));
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
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__OWNERSHIP, oldOwnership, ownership));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__RELIABILITY, oldReliability,
							reliability));
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
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__RELIABILITY, oldReliability,
					reliability));
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
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__RESOURCE_LIMITS,
							oldResource_limits, resource_limits));
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
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__RESOURCE_LIMITS, oldResource_limits,
					resource_limits));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public TopicDataQosPolicy getTopic_data() {
		if (topic_data != null && topic_data.eIsProxy()) {
			InternalEObject oldTopic_data = (InternalEObject) topic_data;
			topic_data = (TopicDataQosPolicy) eResolveProxy(oldTopic_data);
			if (topic_data != oldTopic_data) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__TOPIC_DATA, oldTopic_data,
							topic_data));
			}
		}
		return topic_data;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public TopicDataQosPolicy basicGetTopic_data() {
		return topic_data;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setTopic_data(TopicDataQosPolicy newTopic_data) {
		TopicDataQosPolicy oldTopic_data = topic_data;
		topic_data = newTopic_data;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__TOPIC_DATA, oldTopic_data, topic_data));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public TransportPriorityQosPolicy getTransport_priority() {
		if (transport_priority != null && transport_priority.eIsProxy()) {
			InternalEObject oldTransport_priority = (InternalEObject) transport_priority;
			transport_priority = (TransportPriorityQosPolicy) eResolveProxy(oldTransport_priority);
			if (transport_priority != oldTransport_priority) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__TRANSPORT_PRIORITY,
							oldTransport_priority, transport_priority));
			}
		}
		return transport_priority;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public TransportPriorityQosPolicy basicGetTransport_priority() {
		return transport_priority;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setTransport_priority(
			TransportPriorityQosPolicy newTransport_priority) {
		TransportPriorityQosPolicy oldTransport_priority = transport_priority;
		transport_priority = newTransport_priority;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__TRANSPORT_PRIORITY,
					oldTransport_priority, transport_priority));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public LifespanQosPolicy getLifespan() {
		if (lifespan != null && lifespan.eIsProxy()) {
			InternalEObject oldLifespan = (InternalEObject) lifespan;
			lifespan = (LifespanQosPolicy) eResolveProxy(oldLifespan);
			if (lifespan != oldLifespan) {
				if (eNotificationRequired())
					eNotify(new ENotificationImpl(this, Notification.RESOLVE,
							TopicsPackage.TOPIC__LIFESPAN, oldLifespan,
							lifespan));
			}
		}
		return lifespan;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public LifespanQosPolicy basicGetLifespan() {
		return lifespan;
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	public void setLifespan(LifespanQosPolicy newLifespan) {
		LifespanQosPolicy oldLifespan = lifespan;
		lifespan = newLifespan;
		if (eNotificationRequired())
			eNotify(new ENotificationImpl(this, Notification.SET,
					TopicsPackage.TOPIC__LIFESPAN, oldLifespan, lifespan));
	}

	/**
	 * <!-- begin-user-doc -->
	 * <!-- end-user-doc -->
	 * @generated
	 */
	@Override
	public Object eGet(int featureID, boolean resolve, boolean coreType) {
		switch (featureID) {
		case TopicsPackage.TOPIC__DEADLINE:
			if (resolve)
				return getDeadline();
			return basicGetDeadline();
		case TopicsPackage.TOPIC__DESTINATION_ORDER:
			if (resolve)
				return getDestination_order();
			return basicGetDestination_order();
		case TopicsPackage.TOPIC__DURABILITY:
			if (resolve)
				return getDurability();
			return basicGetDurability();
		case TopicsPackage.TOPIC__DURABILITY_SERVICE:
			if (resolve)
				return getDurability_service();
			return basicGetDurability_service();
		case TopicsPackage.TOPIC__HISTORY:
			if (resolve)
				return getHistory();
			return basicGetHistory();
		case TopicsPackage.TOPIC__LATENCY_BUDGET:
			if (resolve)
				return getLatency_budget();
			return basicGetLatency_budget();
		case TopicsPackage.TOPIC__LIVELINESS:
			if (resolve)
				return getLiveliness();
			return basicGetLiveliness();
		case TopicsPackage.TOPIC__OWNERSHIP:
			if (resolve)
				return getOwnership();
			return basicGetOwnership();
		case TopicsPackage.TOPIC__RELIABILITY:
			if (resolve)
				return getReliability();
			return basicGetReliability();
		case TopicsPackage.TOPIC__RESOURCE_LIMITS:
			if (resolve)
				return getResource_limits();
			return basicGetResource_limits();
		case TopicsPackage.TOPIC__TOPIC_DATA:
			if (resolve)
				return getTopic_data();
			return basicGetTopic_data();
		case TopicsPackage.TOPIC__TRANSPORT_PRIORITY:
			if (resolve)
				return getTransport_priority();
			return basicGetTransport_priority();
		case TopicsPackage.TOPIC__LIFESPAN:
			if (resolve)
				return getLifespan();
			return basicGetLifespan();
		case TopicsPackage.TOPIC__DATATYPE:
			if (resolve)
				return getDatatype();
			return basicGetDatatype();
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
		case TopicsPackage.TOPIC__DEADLINE:
			setDeadline((DeadlineQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__DESTINATION_ORDER:
			setDestination_order((DestinationOrderQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__DURABILITY:
			setDurability((DurabilityQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__DURABILITY_SERVICE:
			setDurability_service((DurabilityServiceQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__HISTORY:
			setHistory((HistoryQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__LATENCY_BUDGET:
			setLatency_budget((LatencyBudgetQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__LIVELINESS:
			setLiveliness((LivelinessQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__OWNERSHIP:
			setOwnership((OwnershipQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__RELIABILITY:
			setReliability((ReliabilityQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__RESOURCE_LIMITS:
			setResource_limits((ResourceLimitsQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__TOPIC_DATA:
			setTopic_data((TopicDataQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__TRANSPORT_PRIORITY:
			setTransport_priority((TransportPriorityQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__LIFESPAN:
			setLifespan((LifespanQosPolicy) newValue);
			return;
		case TopicsPackage.TOPIC__DATATYPE:
			setDatatype((Struct) newValue);
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
		case TopicsPackage.TOPIC__DEADLINE:
			setDeadline((DeadlineQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__DESTINATION_ORDER:
			setDestination_order((DestinationOrderQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__DURABILITY:
			setDurability((DurabilityQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__DURABILITY_SERVICE:
			setDurability_service((DurabilityServiceQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__HISTORY:
			setHistory((HistoryQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__LATENCY_BUDGET:
			setLatency_budget((LatencyBudgetQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__LIVELINESS:
			setLiveliness((LivelinessQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__OWNERSHIP:
			setOwnership((OwnershipQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__RELIABILITY:
			setReliability((ReliabilityQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__RESOURCE_LIMITS:
			setResource_limits((ResourceLimitsQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__TOPIC_DATA:
			setTopic_data((TopicDataQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__TRANSPORT_PRIORITY:
			setTransport_priority((TransportPriorityQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__LIFESPAN:
			setLifespan((LifespanQosPolicy) null);
			return;
		case TopicsPackage.TOPIC__DATATYPE:
			setDatatype((Struct) null);
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
		case TopicsPackage.TOPIC__DEADLINE:
			return deadline != null;
		case TopicsPackage.TOPIC__DESTINATION_ORDER:
			return destination_order != null;
		case TopicsPackage.TOPIC__DURABILITY:
			return durability != null;
		case TopicsPackage.TOPIC__DURABILITY_SERVICE:
			return durability_service != null;
		case TopicsPackage.TOPIC__HISTORY:
			return history != null;
		case TopicsPackage.TOPIC__LATENCY_BUDGET:
			return latency_budget != null;
		case TopicsPackage.TOPIC__LIVELINESS:
			return liveliness != null;
		case TopicsPackage.TOPIC__OWNERSHIP:
			return ownership != null;
		case TopicsPackage.TOPIC__RELIABILITY:
			return reliability != null;
		case TopicsPackage.TOPIC__RESOURCE_LIMITS:
			return resource_limits != null;
		case TopicsPackage.TOPIC__TOPIC_DATA:
			return topic_data != null;
		case TopicsPackage.TOPIC__TRANSPORT_PRIORITY:
			return transport_priority != null;
		case TopicsPackage.TOPIC__LIFESPAN:
			return lifespan != null;
		case TopicsPackage.TOPIC__DATATYPE:
			return datatype != null;
		}
		return super.eIsSet(featureID);
	}

	/**
	 * @generated NOT
	 */
	@Override
	public org.eclipse.emf.common.util.EList<org.opendds.modeling.model.types.Type> getTypes() {
		return com.ociweb.emf.util.ReferencesFinder.findInstancesOf(
				org.opendds.modeling.model.types.Type.class, this);
	}

} //TopicImpl
