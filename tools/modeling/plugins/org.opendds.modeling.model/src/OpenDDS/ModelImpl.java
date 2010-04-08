/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import java.util.Collection;

import org.eclipse.emf.common.notify.NotificationChain;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.InternalEObject;
import org.eclipse.emf.ecore.util.EObjectContainmentEList;
import org.eclipse.emf.ecore.util.InternalEList;

/**
 * <!-- begin-user-doc --> An implementation of the model object '
 * <em><b>Model</b></em>'. <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 * <li>{@link OpenDDS.ModelImpl#getApplications <em>Applications</em>}
 * </li>
 * <li>{@link OpenDDS.ModelImpl#getDomains <em>Domains</em>}</li>
 * <li>{@link OpenDDS.ModelImpl#getParticipants <em>Participants</em>}
 * </li>
 * <li>{@link OpenDDS.ModelImpl#getTopics <em>Topics</em>}</li>
 * <li>{@link OpenDDS.ModelImpl#getTopicTypes <em>Topic Types</em>}</li>
 * <li>{@link OpenDDS.ModelImpl#getQosPolicies <em>Qos Policies</em>}</li>
 * <li>{@link OpenDDS.ModelImpl#getTransports <em>Transports</em>}</li>
 * </ul>
 * </p>
 * 
 * @generated
 */
public class ModelImpl extends EntityImpl implements Model {
    /**
     * The cached value of the '{@link #getApplications()
     * <em>Applications</em>}' containment reference list. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getApplications()
     * @generated
     * @ordered
     */
    protected EList<ApplicationTarget> applications;
    /**
     * The cached value of the '{@link #getDomains() <em>Domains</em>}
     * ' containment reference list. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @see #getDomains()
     * @generated
     * @ordered
     */
    protected EList<Domain> domains;
    /**
     * The cached value of the '{@link #getParticipants()
     * <em>Participants</em>}' containment reference list. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getParticipants()
     * @generated
     * @ordered
     */
    protected EList<DomainParticipant> participants;
    /**
     * The cached value of the '{@link #getTopics() <em>Topics</em>}'
     * containment reference list. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @see #getTopics()
     * @generated
     * @ordered
     */
    protected EList<TopicDescription> topics;
    /**
     * The cached value of the '{@link #getTopicTypes()
     * <em>Topic Types</em>}' containment reference list. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getTopicTypes()
     * @generated
     * @ordered
     */
    protected EList<TopicStruct> topicTypes;
    /**
     * The cached value of the '{@link #getQosPolicies()
     * <em>Qos Policies</em>}' containment reference list. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getQosPolicies()
     * @generated
     * @ordered
     */
    protected EList<QosPolicy> qosPolicies;
    /**
     * The cached value of the '{@link #getTransports()
     * <em>Transports</em>}' containment reference list. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getTransports()
     * @generated
     * @ordered
     */
    protected EList<Transport> transports;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected ModelImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.MODEL;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<ApplicationTarget> getApplications() {
        if (applications == null) {
            applications = new EObjectContainmentEList<ApplicationTarget>(ApplicationTarget.class, this,
                    OpenDDSPackage.MODEL__APPLICATIONS);
        }
        return applications;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<Domain> getDomains() {
        if (domains == null) {
            domains = new EObjectContainmentEList<Domain>(Domain.class, this, OpenDDSPackage.MODEL__DOMAINS);
        }
        return domains;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<DomainParticipant> getParticipants() {
        if (participants == null) {
            participants = new EObjectContainmentEList<DomainParticipant>(DomainParticipant.class, this,
                    OpenDDSPackage.MODEL__PARTICIPANTS);
        }
        return participants;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<TopicDescription> getTopics() {
        if (topics == null) {
            topics = new EObjectContainmentEList<TopicDescription>(TopicDescription.class, this,
                    OpenDDSPackage.MODEL__TOPICS);
        }
        return topics;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<TopicStruct> getTopicTypes() {
        if (topicTypes == null) {
            topicTypes = new EObjectContainmentEList<TopicStruct>(TopicStruct.class, this,
                    OpenDDSPackage.MODEL__TOPIC_TYPES);
        }
        return topicTypes;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<QosPolicy> getQosPolicies() {
        if (qosPolicies == null) {
            qosPolicies = new EObjectContainmentEList<QosPolicy>(QosPolicy.class, this,
                    OpenDDSPackage.MODEL__QOS_POLICIES);
        }
        return qosPolicies;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<Transport> getTransports() {
        if (transports == null) {
            transports = new EObjectContainmentEList<Transport>(Transport.class, this, OpenDDSPackage.MODEL__TRANSPORTS);
        }
        return transports;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
        switch (featureID) {
            case OpenDDSPackage.MODEL__APPLICATIONS:
                return ((InternalEList<?>) getApplications()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.MODEL__DOMAINS:
                return ((InternalEList<?>) getDomains()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.MODEL__PARTICIPANTS:
                return ((InternalEList<?>) getParticipants()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.MODEL__TOPICS:
                return ((InternalEList<?>) getTopics()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.MODEL__TOPIC_TYPES:
                return ((InternalEList<?>) getTopicTypes()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.MODEL__QOS_POLICIES:
                return ((InternalEList<?>) getQosPolicies()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.MODEL__TRANSPORTS:
                return ((InternalEList<?>) getTransports()).basicRemove(otherEnd, msgs);
        }
        return super.eInverseRemove(otherEnd, featureID, msgs);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.MODEL__APPLICATIONS:
                return getApplications();
            case OpenDDSPackage.MODEL__DOMAINS:
                return getDomains();
            case OpenDDSPackage.MODEL__PARTICIPANTS:
                return getParticipants();
            case OpenDDSPackage.MODEL__TOPICS:
                return getTopics();
            case OpenDDSPackage.MODEL__TOPIC_TYPES:
                return getTopicTypes();
            case OpenDDSPackage.MODEL__QOS_POLICIES:
                return getQosPolicies();
            case OpenDDSPackage.MODEL__TRANSPORTS:
                return getTransports();
        }
        return super.eGet(featureID, resolve, coreType);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @SuppressWarnings("unchecked")
    @Override
    public void eSet(int featureID, Object newValue) {
        switch (featureID) {
            case OpenDDSPackage.MODEL__APPLICATIONS:
                getApplications().clear();
                getApplications().addAll((Collection<? extends ApplicationTarget>) newValue);
                return;
            case OpenDDSPackage.MODEL__DOMAINS:
                getDomains().clear();
                getDomains().addAll((Collection<? extends Domain>) newValue);
                return;
            case OpenDDSPackage.MODEL__PARTICIPANTS:
                getParticipants().clear();
                getParticipants().addAll((Collection<? extends DomainParticipant>) newValue);
                return;
            case OpenDDSPackage.MODEL__TOPICS:
                getTopics().clear();
                getTopics().addAll((Collection<? extends TopicDescription>) newValue);
                return;
            case OpenDDSPackage.MODEL__TOPIC_TYPES:
                getTopicTypes().clear();
                getTopicTypes().addAll((Collection<? extends TopicStruct>) newValue);
                return;
            case OpenDDSPackage.MODEL__QOS_POLICIES:
                getQosPolicies().clear();
                getQosPolicies().addAll((Collection<? extends QosPolicy>) newValue);
                return;
            case OpenDDSPackage.MODEL__TRANSPORTS:
                getTransports().clear();
                getTransports().addAll((Collection<? extends Transport>) newValue);
                return;
        }
        super.eSet(featureID, newValue);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public void eUnset(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.MODEL__APPLICATIONS:
                getApplications().clear();
                return;
            case OpenDDSPackage.MODEL__DOMAINS:
                getDomains().clear();
                return;
            case OpenDDSPackage.MODEL__PARTICIPANTS:
                getParticipants().clear();
                return;
            case OpenDDSPackage.MODEL__TOPICS:
                getTopics().clear();
                return;
            case OpenDDSPackage.MODEL__TOPIC_TYPES:
                getTopicTypes().clear();
                return;
            case OpenDDSPackage.MODEL__QOS_POLICIES:
                getQosPolicies().clear();
                return;
            case OpenDDSPackage.MODEL__TRANSPORTS:
                getTransports().clear();
                return;
        }
        super.eUnset(featureID);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public boolean eIsSet(int featureID) {
        switch (featureID) {
            case OpenDDSPackage.MODEL__APPLICATIONS:
                return applications != null && !applications.isEmpty();
            case OpenDDSPackage.MODEL__DOMAINS:
                return domains != null && !domains.isEmpty();
            case OpenDDSPackage.MODEL__PARTICIPANTS:
                return participants != null && !participants.isEmpty();
            case OpenDDSPackage.MODEL__TOPICS:
                return topics != null && !topics.isEmpty();
            case OpenDDSPackage.MODEL__TOPIC_TYPES:
                return topicTypes != null && !topicTypes.isEmpty();
            case OpenDDSPackage.MODEL__QOS_POLICIES:
                return qosPolicies != null && !qosPolicies.isEmpty();
            case OpenDDSPackage.MODEL__TRANSPORTS:
                return transports != null && !transports.isEmpty();
        }
        return super.eIsSet(featureID);
    }

} // ModelImpl
