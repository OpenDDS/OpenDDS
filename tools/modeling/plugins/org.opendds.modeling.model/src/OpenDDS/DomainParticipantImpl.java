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

import org.eclipse.emf.common.notify.Notification;
import org.eclipse.emf.common.notify.NotificationChain;
import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.InternalEObject;
import org.eclipse.emf.ecore.impl.ENotificationImpl;
import org.eclipse.emf.ecore.util.EObjectContainmentEList;
import org.eclipse.emf.ecore.util.InternalEList;

/**
 * <!-- begin-user-doc --> An implementation of the model object '
 * <em><b>Domain Participant</b></em>'. <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 * <li>{@link OpenDDS.DomainParticipantImpl#getSubscribers <em>
 * Subscribers</em>}</li>
 * <li>{@link OpenDDS.DomainParticipantImpl#getPublishers <em>
 * Publishers</em>}</li>
 * <li>{@link OpenDDS.DomainParticipantImpl#getEntity_factory <em>
 * Entity factory</em>}</li>
 * <li>{@link OpenDDS.DomainParticipantImpl#getUser_data <em>User data
 * </em>}</li>
 * <li>{@link OpenDDS.DomainParticipantImpl#getDomain <em>Domain</em>}
 * </li>
 * </ul>
 * </p>
 * 
 * @generated
 */
public class DomainParticipantImpl extends DomainEntityImpl implements DomainParticipant {
    /**
     * The cached value of the '{@link #getSubscribers()
     * <em>Subscribers</em>}' containment reference list. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getSubscribers()
     * @generated
     * @ordered
     */
    protected EList<Subscriber> subscribers;

    /**
     * The cached value of the '{@link #getPublishers()
     * <em>Publishers</em>}' containment reference list. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getPublishers()
     * @generated
     * @ordered
     */
    protected EList<Publisher> publishers;

    /**
     * The cached value of the '{@link #getEntity_factory()
     * <em>Entity factory</em>}' reference. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @see #getEntity_factory()
     * @generated
     * @ordered
     */
    protected EntityFactoryQosPolicy entity_factory;

    /**
     * The cached value of the '{@link #getUser_data()
     * <em>User data</em>}' reference. <!-- begin-user-doc --> <!--
     * end-user-doc -->
     * 
     * @see #getUser_data()
     * @generated
     * @ordered
     */
    protected UserDataQosPolicy user_data;

    /**
     * The cached value of the '{@link #getDomain() <em>Domain</em>}'
     * reference. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @see #getDomain()
     * @generated
     * @ordered
     */
    protected Domain domain;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected DomainParticipantImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.DOMAIN_PARTICIPANT;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<Subscriber> getSubscribers() {
        if (subscribers == null) {
            subscribers = new EObjectContainmentEList<Subscriber>(Subscriber.class, this,
                    OpenDDSPackage.DOMAIN_PARTICIPANT__SUBSCRIBERS);
        }
        return subscribers;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EList<Publisher> getPublishers() {
        if (publishers == null) {
            publishers = new EObjectContainmentEList<Publisher>(Publisher.class, this,
                    OpenDDSPackage.DOMAIN_PARTICIPANT__PUBLISHERS);
        }
        return publishers;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EntityFactoryQosPolicy getEntity_factory() {
        if (entity_factory != null && entity_factory.eIsProxy()) {
            InternalEObject oldEntity_factory = (InternalEObject) entity_factory;
            entity_factory = (EntityFactoryQosPolicy) eResolveProxy(oldEntity_factory);
            if (entity_factory != oldEntity_factory) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.DOMAIN_PARTICIPANT__ENTITY_FACTORY, oldEntity_factory, entity_factory));
                }
            }
        }
        return entity_factory;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public EntityFactoryQosPolicy basicGetEntity_factory() {
        return entity_factory;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setEntity_factory(EntityFactoryQosPolicy newEntity_factory) {
        EntityFactoryQosPolicy oldEntity_factory = entity_factory;
        entity_factory = newEntity_factory;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DOMAIN_PARTICIPANT__ENTITY_FACTORY,
                    oldEntity_factory, entity_factory));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public UserDataQosPolicy getUser_data() {
        if (user_data != null && user_data.eIsProxy()) {
            InternalEObject oldUser_data = (InternalEObject) user_data;
            user_data = (UserDataQosPolicy) eResolveProxy(oldUser_data);
            if (user_data != oldUser_data) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.DOMAIN_PARTICIPANT__USER_DATA, oldUser_data, user_data));
                }
            }
        }
        return user_data;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public UserDataQosPolicy basicGetUser_data() {
        return user_data;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setUser_data(UserDataQosPolicy newUser_data) {
        UserDataQosPolicy oldUser_data = user_data;
        user_data = newUser_data;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DOMAIN_PARTICIPANT__USER_DATA,
                    oldUser_data, user_data));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public Domain getDomain() {
        if (domain != null && domain.eIsProxy()) {
            InternalEObject oldDomain = (InternalEObject) domain;
            domain = (Domain) eResolveProxy(oldDomain);
            if (domain != oldDomain) {
                if (eNotificationRequired()) {
                    eNotify(new ENotificationImpl(this, Notification.RESOLVE,
                            OpenDDSPackage.DOMAIN_PARTICIPANT__DOMAIN, oldDomain, domain));
                }
            }
        }
        return domain;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public Domain basicGetDomain() {
        return domain;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setDomain(Domain newDomain) {
        Domain oldDomain = domain;
        domain = newDomain;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DOMAIN_PARTICIPANT__DOMAIN, oldDomain,
                    domain));
        }
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
        switch (featureID) {
            case OpenDDSPackage.DOMAIN_PARTICIPANT__SUBSCRIBERS:
                return ((InternalEList<?>) getSubscribers()).basicRemove(otherEnd, msgs);
            case OpenDDSPackage.DOMAIN_PARTICIPANT__PUBLISHERS:
                return ((InternalEList<?>) getPublishers()).basicRemove(otherEnd, msgs);
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
            case OpenDDSPackage.DOMAIN_PARTICIPANT__SUBSCRIBERS:
                return getSubscribers();
            case OpenDDSPackage.DOMAIN_PARTICIPANT__PUBLISHERS:
                return getPublishers();
            case OpenDDSPackage.DOMAIN_PARTICIPANT__ENTITY_FACTORY:
                if (resolve) {
                    return getEntity_factory();
                }
                return basicGetEntity_factory();
            case OpenDDSPackage.DOMAIN_PARTICIPANT__USER_DATA:
                if (resolve) {
                    return getUser_data();
                }
                return basicGetUser_data();
            case OpenDDSPackage.DOMAIN_PARTICIPANT__DOMAIN:
                if (resolve) {
                    return getDomain();
                }
                return basicGetDomain();
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
            case OpenDDSPackage.DOMAIN_PARTICIPANT__SUBSCRIBERS:
                getSubscribers().clear();
                getSubscribers().addAll((Collection<? extends Subscriber>) newValue);
                return;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__PUBLISHERS:
                getPublishers().clear();
                getPublishers().addAll((Collection<? extends Publisher>) newValue);
                return;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__ENTITY_FACTORY:
                setEntity_factory((EntityFactoryQosPolicy) newValue);
                return;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__USER_DATA:
                setUser_data((UserDataQosPolicy) newValue);
                return;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__DOMAIN:
                setDomain((Domain) newValue);
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
            case OpenDDSPackage.DOMAIN_PARTICIPANT__SUBSCRIBERS:
                getSubscribers().clear();
                return;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__PUBLISHERS:
                getPublishers().clear();
                return;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__ENTITY_FACTORY:
                setEntity_factory((EntityFactoryQosPolicy) null);
                return;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__USER_DATA:
                setUser_data((UserDataQosPolicy) null);
                return;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__DOMAIN:
                setDomain((Domain) null);
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
            case OpenDDSPackage.DOMAIN_PARTICIPANT__SUBSCRIBERS:
                return subscribers != null && !subscribers.isEmpty();
            case OpenDDSPackage.DOMAIN_PARTICIPANT__PUBLISHERS:
                return publishers != null && !publishers.isEmpty();
            case OpenDDSPackage.DOMAIN_PARTICIPANT__ENTITY_FACTORY:
                return entity_factory != null;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__USER_DATA:
                return user_data != null;
            case OpenDDSPackage.DOMAIN_PARTICIPANT__DOMAIN:
                return domain != null;
        }
        return super.eIsSet(featureID);
    }

} // DomainParticipantImpl
