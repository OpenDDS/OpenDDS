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
import org.eclipse.emf.ecore.impl.ENotificationImpl;
import org.eclipse.emf.ecore.impl.EObjectImpl;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Period</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.PeriodImpl#getSeconds <em>Seconds</em>}</li>
 *   <li>{@link OpenDDS.PeriodImpl#getNanoseconds <em>Nanoseconds</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class PeriodImpl extends EObjectImpl implements Period {
    /**
     * The default value of the '{@link #getSeconds() <em>Seconds</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getSeconds()
     * @generated
     * @ordered
     */
    protected static final long SECONDS_EDEFAULT = 0L;

    /**
     * The cached value of the '{@link #getSeconds() <em>Seconds</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getSeconds()
     * @generated
     * @ordered
     */
    protected long seconds = SECONDS_EDEFAULT;

    /**
     * The default value of the '{@link #getNanoseconds() <em>Nanoseconds</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getNanoseconds()
     * @generated
     * @ordered
     */
    protected static final long NANOSECONDS_EDEFAULT = 0L;

    /**
     * The cached value of the '{@link #getNanoseconds() <em>Nanoseconds</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getNanoseconds()
     * @generated
     * @ordered
     */
    protected long nanoseconds = NANOSECONDS_EDEFAULT;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected PeriodImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return ModelPackage.Literals.PERIOD;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public long getSeconds() {
        return seconds;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setSeconds(long newSeconds) {
        long oldSeconds = seconds;
        seconds = newSeconds;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.PERIOD__SECONDS, oldSeconds, seconds));
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public long getNanoseconds() {
        return nanoseconds;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setNanoseconds(long newNanoseconds) {
        long oldNanoseconds = nanoseconds;
        nanoseconds = newNanoseconds;
        if (eNotificationRequired()) {
            eNotify(new ENotificationImpl(this, Notification.SET, ModelPackage.PERIOD__NANOSECONDS, oldNanoseconds,
                    nanoseconds));
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
            case ModelPackage.PERIOD__SECONDS:
                return getSeconds();
            case ModelPackage.PERIOD__NANOSECONDS:
                return getNanoseconds();
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
            case ModelPackage.PERIOD__SECONDS:
                setSeconds((Long) newValue);
                return;
            case ModelPackage.PERIOD__NANOSECONDS:
                setNanoseconds((Long) newValue);
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
            case ModelPackage.PERIOD__SECONDS:
                setSeconds(SECONDS_EDEFAULT);
                return;
            case ModelPackage.PERIOD__NANOSECONDS:
                setNanoseconds(NANOSECONDS_EDEFAULT);
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
            case ModelPackage.PERIOD__SECONDS:
                return seconds != SECONDS_EDEFAULT;
            case ModelPackage.PERIOD__NANOSECONDS:
                return nanoseconds != NANOSECONDS_EDEFAULT;
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
        result.append(" (seconds: ");
        result.append(seconds);
        result.append(", nanoseconds: ");
        result.append(nanoseconds);
        result.append(')');
        return result.toString();
    }

} //PeriodImpl
