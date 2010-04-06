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
 * An implementation of the model object '<em><b>Deadline Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.DeadlineQosPolicyImpl#getPeriod <em>Period</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class DeadlineQosPolicyImpl extends QosPolicyImpl implements DeadlineQosPolicy {
    /**
     * The cached value of the '{@link #getPeriod() <em>Period</em>}' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getPeriod()
     * @generated
     * @ordered
     */
    protected Period period;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected DeadlineQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.DEADLINE_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Period getPeriod() {
        return period;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public NotificationChain basicSetPeriod(Period newPeriod, NotificationChain msgs) {
        Period oldPeriod = period;
        period = newPeriod;
        if (eNotificationRequired()) {
            ENotificationImpl notification = new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD, oldPeriod, newPeriod);
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
    public void setPeriod(Period newPeriod) {
        if (newPeriod != period) {
            NotificationChain msgs = null;
            if (period != null)
                msgs = ((InternalEObject) period).eInverseRemove(this, EOPPOSITE_FEATURE_BASE
                        - OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD, null, msgs);
            if (newPeriod != null)
                msgs = ((InternalEObject) newPeriod).eInverseAdd(this, EOPPOSITE_FEATURE_BASE
                        - OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD, null, msgs);
            msgs = basicSetPeriod(newPeriod, msgs);
            if (msgs != null)
                msgs.dispatch();
        } else if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET, OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD,
                    newPeriod, newPeriod));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
        switch (featureID) {
            case OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD:
                return basicSetPeriod(null, msgs);
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
            case OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD:
                return getPeriod();
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
            case OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD:
                setPeriod((Period) newValue);
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
            case OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD:
                setPeriod((Period) null);
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
            case OpenDDSPackage.DEADLINE_QOS_POLICY__PERIOD:
                return period != null;
        }
        return super.eIsSet(featureID);
    }

} //DeadlineQosPolicyImpl
