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
 * An implementation of the model object '<em><b>Reliability Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.ReliabilityQosPolicyImpl#getKind <em>Kind</em>}</li>
 *   <li>{@link OpenDDS.ReliabilityQosPolicyImpl#getMax_blocking_time <em>Max blocking time</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class ReliabilityQosPolicyImpl extends QosPolicyImpl implements ReliabilityQosPolicy {
    /**
     * The default value of the '{@link #getKind() <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getKind()
     * @generated
     * @ordered
     */
    protected static final ReliabilityQosPolicyKind KIND_EDEFAULT = ReliabilityQosPolicyKind.BEST_EFFORT;

    /**
     * The cached value of the '{@link #getKind() <em>Kind</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getKind()
     * @generated
     * @ordered
     */
    protected ReliabilityQosPolicyKind kind = KIND_EDEFAULT;

    /**
     * The cached value of the '{@link #getMax_blocking_time() <em>Max blocking time</em>}' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getMax_blocking_time()
     * @generated
     * @ordered
     */
    protected Period max_blocking_time;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected ReliabilityQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.RELIABILITY_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ReliabilityQosPolicyKind getKind() {
        return kind;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setKind(ReliabilityQosPolicyKind newKind) {
        ReliabilityQosPolicyKind oldKind = kind;
        kind = newKind == null ? KIND_EDEFAULT : newKind;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.RELIABILITY_QOS_POLICY__KIND, oldKind,
                    kind));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Period getMax_blocking_time() {
        return max_blocking_time;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public NotificationChain basicSetMax_blocking_time(Period newMax_blocking_time, NotificationChain msgs) {
        Period oldMax_blocking_time = max_blocking_time;
        max_blocking_time = newMax_blocking_time;
        if (eNotificationRequired()) {
            ENotificationImpl notification = new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME, oldMax_blocking_time,
                    newMax_blocking_time);
            if (msgs == null)
                msgs = notification;
            else
                msgs.add(notification);
        }
        return msgs;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setMax_blocking_time(Period newMax_blocking_time) {
        if (newMax_blocking_time != max_blocking_time) {
            NotificationChain msgs = null;
            if (max_blocking_time != null)
                msgs = ((InternalEObject) max_blocking_time).eInverseRemove(this, EOPPOSITE_FEATURE_BASE
                        - OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME, null, msgs);
            if (newMax_blocking_time != null)
                msgs = ((InternalEObject) newMax_blocking_time).eInverseAdd(this, EOPPOSITE_FEATURE_BASE
                        - OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME, null, msgs);
            msgs = basicSetMax_blocking_time(newMax_blocking_time, msgs);
            if (msgs != null)
                msgs.dispatch();
        } else if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME, newMax_blocking_time,
                    newMax_blocking_time));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
        switch (featureID) {
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME:
                return basicSetMax_blocking_time(null, msgs);
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
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__KIND:
                return getKind();
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME:
                return getMax_blocking_time();
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
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__KIND:
                setKind((ReliabilityQosPolicyKind) newValue);
                return;
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME:
                setMax_blocking_time((Period) newValue);
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
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__KIND:
                setKind(KIND_EDEFAULT);
                return;
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME:
                setMax_blocking_time((Period) null);
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
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__KIND:
                return kind != KIND_EDEFAULT;
            case OpenDDSPackage.RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME:
                return max_blocking_time != null;
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
        if (eIsProxy())
            return super.toString();

        StringBuffer result = new StringBuffer(super.toString());
        result.append(" (kind: ");
        result.append(kind);
        result.append(')');
        return result.toString();
    }

} //ReliabilityQosPolicyImpl
