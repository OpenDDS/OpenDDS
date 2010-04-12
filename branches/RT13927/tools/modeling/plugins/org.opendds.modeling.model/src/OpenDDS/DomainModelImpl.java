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
 * <em><b>Domain Model</b></em>'. <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 * <li>{@link OpenDDS.DomainModelImpl#getDomains <em>Domains</em>}</li>
 * <li>{@link OpenDDS.DomainModelImpl#getParticipants <em>Participants
 * </em>}</li>
 * <li>{@link OpenDDS.DomainModelImpl#getTopics <em>Topics</em>}</li>
 * </ul>
 * </p>
 * 
 * @generated
 */
public class DomainModelImpl extends ModelImpl implements DomainModel {
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
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected DomainModelImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.DOMAIN_MODEL;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<Domain> getDomains() {
        if (domains == null) {
            domains = new EObjectContainmentEList<Domain>(Domain.class, this, OpenDDSPackage.DOMAIN_MODEL__DOMAINS);
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
                    OpenDDSPackage.DOMAIN_MODEL__PARTICIPANTS);
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
                    OpenDDSPackage.DOMAIN_MODEL__TOPICS);
        }
        return topics;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
        switch (featureID) {
            case OpenDDSPackage.DOMAIN_MODEL__DOMAINS:
                return ((InternalEList<?>) getDomains()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.DOMAIN_MODEL__PARTICIPANTS:
                return ((InternalEList<?>) getParticipants()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.DOMAIN_MODEL__TOPICS:
                return ((InternalEList<?>) getTopics()).basicRemove(otherEnd, msgs);
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
            case OpenDDSPackage.DOMAIN_MODEL__DOMAINS:
                return getDomains();
            case OpenDDSPackage.DOMAIN_MODEL__PARTICIPANTS:
                return getParticipants();
            case OpenDDSPackage.DOMAIN_MODEL__TOPICS:
                return getTopics();
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
            case OpenDDSPackage.DOMAIN_MODEL__DOMAINS:
                getDomains().clear();
                getDomains().addAll((Collection<? extends Domain>) newValue);
                return;
            case OpenDDSPackage.DOMAIN_MODEL__PARTICIPANTS:
                getParticipants().clear();
                getParticipants().addAll((Collection<? extends DomainParticipant>) newValue);
                return;
            case OpenDDSPackage.DOMAIN_MODEL__TOPICS:
                getTopics().clear();
                getTopics().addAll((Collection<? extends TopicDescription>) newValue);
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
            case OpenDDSPackage.DOMAIN_MODEL__DOMAINS:
                getDomains().clear();
                return;
            case OpenDDSPackage.DOMAIN_MODEL__PARTICIPANTS:
                getParticipants().clear();
                return;
            case OpenDDSPackage.DOMAIN_MODEL__TOPICS:
                getTopics().clear();
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
            case OpenDDSPackage.DOMAIN_MODEL__DOMAINS:
                return domains != null && !domains.isEmpty();
            case OpenDDSPackage.DOMAIN_MODEL__PARTICIPANTS:
                return participants != null && !participants.isEmpty();
            case OpenDDSPackage.DOMAIN_MODEL__TOPICS:
                return topics != null && !topics.isEmpty();
        }
        return super.eIsSet(featureID);
    }

} // DomainModelImpl
