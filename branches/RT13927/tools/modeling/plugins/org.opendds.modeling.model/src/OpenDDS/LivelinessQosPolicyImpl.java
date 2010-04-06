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
import org.eclipse.emf.common.notify.NotificationChain;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.InternalEObject;
import org.eclipse.emf.ecore.impl.ENotificationImpl;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Liveliness Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.LivelinessQosPolicyImpl#getKind <em>Kind</em>}</li>
 *   <li>{@link OpenDDS.LivelinessQosPolicyImpl#getLease_duration <em>Lease duration</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class LivelinessQosPolicyImpl extends QosPolicyImpl implements LivelinessQosPolicy {
    /**
     * The default value of the '{@link #getKind() <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getKind()
     * @generated
     * @ordered
     */
    protected static final LivelinessQosPolicyKind KIND_EDEFAULT = LivelinessQosPolicyKind.AUTOMATIC;

    /**
     * The cached value of the '{@link #getKind() <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getKind()
     * @generated
     * @ordered
     */
    protected LivelinessQosPolicyKind kind = KIND_EDEFAULT;

    /**
     * The cached value of the '{@link #getLease_duration() <em>Lease duration</em>}' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getLease_duration()
     * @generated
     * @ordered
     */
    protected Period lease_duration;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected LivelinessQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.LIVELINESS_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LivelinessQosPolicyKind getKind() {
        return kind;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setKind(LivelinessQosPolicyKind newKind) {
        LivelinessQosPolicyKind oldKind = kind;
        kind = newKind == null ? KIND_EDEFAULT : newKind;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.LIVELINESS_QOS_POLICY__KIND, oldKind,
                    kind));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Period getLease_duration() {
        return lease_duration;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public NotificationChain basicSetLease_duration(Period newLease_duration, NotificationChain msgs) {
        Period oldLease_duration = lease_duration;
        lease_duration = newLease_duration;
        if (eNotificationRequired()) {
            ENotificationImpl notification = new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION, oldLease_duration, newLease_duration);
            if (msgs == null) {
                msgs = notification;
            } else {
                msgs.add(notification);
            }
        }
        return msgs;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setLease_duration(Period newLease_duration) {
        if (newLease_duration != lease_duration) {
            NotificationChain msgs = null;
            if (lease_duration != null) {
                msgs = ((InternalEObject) lease_duration).eInverseRemove(this, EOPPOSITE_FEATURE_BASE
                        - OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION, null, msgs);
            }
            if (newLease_duration != null) {
                msgs = ((InternalEObject) newLease_duration).eInverseAdd(this, EOPPOSITE_FEATURE_BASE
                        - OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION, null, msgs);
            }
            msgs = basicSetLease_duration(newLease_duration, msgs);
            if (msgs != null) {
                msgs.dispatch();
            }
        } else if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION,
                    newLease_duration, newLease_duration));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
        switch (featureID) {
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION:
                return basicSetLease_duration(null, msgs);
        }
        return super.eInverseRemove(otherEnd, featureID, msgs);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__KIND:
                return getKind();
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION:
                return getLease_duration();
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
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__KIND:
                setKind((LivelinessQosPolicyKind) newValue);
                return;
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION:
                setLease_duration((Period) newValue);
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
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__KIND:
                setKind(KIND_EDEFAULT);
                return;
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION:
                setLease_duration((Period) null);
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
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__KIND:
                return kind != KIND_EDEFAULT;
            case OpenDDSPackage.LIVELINESS_QOS_POLICY__LEASE_DURATION:
                return lease_duration != null;
        }
        return super.eIsSet(featureID);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public String toString() {
        if (eIsProxy()) {
            return super.toString();
        }

        StringBuffer result = new StringBuffer(super.toString());
        result.append(" (kind: ");
        result.append(kind);
        result.append(')');
        return result.toString();
    }

} //LivelinessQosPolicyImpl
