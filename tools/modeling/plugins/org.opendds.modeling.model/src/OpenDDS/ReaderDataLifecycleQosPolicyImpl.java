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
 * An implementation of the model object '<em><b>Reader Data Lifecycle Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.ReaderDataLifecycleQosPolicyImpl#getAutopurge_nowriter_samples_delay <em>Autopurge nowriter samples delay</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class ReaderDataLifecycleQosPolicyImpl extends QosPolicyImpl implements ReaderDataLifecycleQosPolicy {
    /**
     * The cached value of the '{@link #getAutopurge_nowriter_samples_delay() <em>Autopurge nowriter samples delay</em>}' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getAutopurge_nowriter_samples_delay()
     * @generated
     * @ordered
     */
    protected Period autopurge_nowriter_samples_delay;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected ReaderDataLifecycleQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return ModelPackage.Literals.READER_DATA_LIFECYCLE_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Period getAutopurge_nowriter_samples_delay() {
        return autopurge_nowriter_samples_delay;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public NotificationChain basicSetAutopurge_nowriter_samples_delay(Period newAutopurge_nowriter_samples_delay,
            NotificationChain msgs) {
        Period oldAutopurge_nowriter_samples_delay = autopurge_nowriter_samples_delay;
        autopurge_nowriter_samples_delay = newAutopurge_nowriter_samples_delay;
        if (eNotificationRequired()) {
            ENotificationImpl notification = new ENotificationImpl(this, Notification.SET,
                    ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY,
                    oldAutopurge_nowriter_samples_delay, newAutopurge_nowriter_samples_delay);
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
    public void setAutopurge_nowriter_samples_delay(Period newAutopurge_nowriter_samples_delay) {
        if (newAutopurge_nowriter_samples_delay != autopurge_nowriter_samples_delay) {
            NotificationChain msgs = null;
            if (autopurge_nowriter_samples_delay != null)
                msgs = ((InternalEObject) autopurge_nowriter_samples_delay).eInverseRemove(this, EOPPOSITE_FEATURE_BASE
                        - ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY, null, msgs);
            if (newAutopurge_nowriter_samples_delay != null)
                msgs = ((InternalEObject) newAutopurge_nowriter_samples_delay).eInverseAdd(this, EOPPOSITE_FEATURE_BASE
                        - ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY, null, msgs);
            msgs = basicSetAutopurge_nowriter_samples_delay(newAutopurge_nowriter_samples_delay, msgs);
            if (msgs != null)
                msgs.dispatch();
        } else if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET,
                    ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY,
                    newAutopurge_nowriter_samples_delay, newAutopurge_nowriter_samples_delay));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
        switch (featureID) {
            case ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY:
                return basicSetAutopurge_nowriter_samples_delay(null, msgs);
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
            case ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY:
                return getAutopurge_nowriter_samples_delay();
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
            case ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY:
                setAutopurge_nowriter_samples_delay((Period) newValue);
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
            case ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY:
                setAutopurge_nowriter_samples_delay((Period) null);
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
            case ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY:
                return autopurge_nowriter_samples_delay != null;
        }
        return super.eIsSet(featureID);
    }

} //ReaderDataLifecycleQosPolicyImpl
