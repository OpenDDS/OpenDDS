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
 * <!-- begin-user-doc --> An implementation of the model object '
 * <em><b>Latency Budget Qos Policy</b></em>'. <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 * <li>{@link OpenDDS.LatencyBudgetQosPolicyImpl#getDuration <em>
 * Duration</em>}</li>
 * </ul>
 * </p>
 * 
 * @generated
 */
public class LatencyBudgetQosPolicyImpl extends QosPolicyImpl implements LatencyBudgetQosPolicy {
    /**
     * The cached value of the '{@link #getDuration()
     * <em>Duration</em>}' containment reference. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @see #getDuration()
     * @generated
     * @ordered
     */
    protected Period duration;

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    protected LatencyBudgetQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.LATENCY_BUDGET_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public Period getDuration() {
        return duration;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public NotificationChain basicSetDuration(Period newDuration, NotificationChain msgs) {
        Period oldDuration = duration;
        duration = newDuration;
        if (eNotificationRequired()) {
            ENotificationImpl notification = new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION, oldDuration, newDuration);
            if (msgs == null) {
                msgs = notification;
            } else {
                msgs.add(notification);
            }
        }
        return msgs;
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    public void setDuration(Period newDuration) {
        if (newDuration != duration) {
            NotificationChain msgs = null;
            if (duration != null) {
                msgs = ((InternalEObject) duration).eInverseRemove(this, EOPPOSITE_FEATURE_BASE
                        - OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION, null, msgs);
            }
            if (newDuration != null) {
                msgs = ((InternalEObject) newDuration).eInverseAdd(this, EOPPOSITE_FEATURE_BASE
                        - OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION, null, msgs);
            }
            msgs = basicSetDuration(newDuration, msgs);
            if (msgs != null) {
                msgs.dispatch();
            }
        } else if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION,
                    newDuration, newDuration));
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
            case OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION:
                return basicSetDuration(null, msgs);
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
            case OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION:
                return getDuration();
        }
        return super.eGet(featureID, resolve, coreType);
    }

    /**
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @generated
     */
    @Override
    public void eSet(int featureID, Object newValue) {
        switch (featureID) {
            case OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION:
                setDuration((Period) newValue);
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
            case OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION:
                setDuration((Period) null);
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
            case OpenDDSPackage.LATENCY_BUDGET_QOS_POLICY__DURATION:
                return duration != null;
        }
        return super.eIsSet(featureID);
    }

} // LatencyBudgetQosPolicyImpl
