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

/**
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Writer Data Lifecycle Qos Policy</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.WriterDataLifecycleQosPolicyImpl#isAutodispose_unregistered_instances <em>Autodispose unregistered instances</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class WriterDataLifecycleQosPolicyImpl extends QosPolicyImpl implements WriterDataLifecycleQosPolicy {
    /**
     * The default value of the '{@link #isAutodispose_unregistered_instances() <em>Autodispose unregistered instances</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #isAutodispose_unregistered_instances()
     * @generated
     * @ordered
     */
    protected static final boolean AUTODISPOSE_UNREGISTERED_INSTANCES_EDEFAULT = false;

    /**
     * The cached value of the '{@link #isAutodispose_unregistered_instances() <em>Autodispose unregistered instances</em>}' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #isAutodispose_unregistered_instances()
     * @generated
     * @ordered
     */
    protected boolean autodispose_unregistered_instances = AUTODISPOSE_UNREGISTERED_INSTANCES_EDEFAULT;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected WriterDataLifecycleQosPolicyImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return OpenDDSPackage.Literals.WRITER_DATA_LIFECYCLE_QOS_POLICY;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public boolean isAutodispose_unregistered_instances() {
        return autodispose_unregistered_instances;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void setAutodispose_unregistered_instances(boolean newAutodispose_unregistered_instances) {
        boolean oldAutodispose_unregistered_instances = autodispose_unregistered_instances;
        autodispose_unregistered_instances = newAutodispose_unregistered_instances;
        if (eNotificationRequired())
            eNotify(new ENotificationImpl(this, Notification.SET,
                    OpenDDSPackage.WRITER_DATA_LIFECYCLE_QOS_POLICY__AUTODISPOSE_UNREGISTERED_INSTANCES,
                    oldAutodispose_unregistered_instances, autodispose_unregistered_instances));
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object eGet(int featureID, boolean resolve, boolean coreType) {
        switch (featureID) {
            case OpenDDSPackage.WRITER_DATA_LIFECYCLE_QOS_POLICY__AUTODISPOSE_UNREGISTERED_INSTANCES:
                return isAutodispose_unregistered_instances();
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
            case OpenDDSPackage.WRITER_DATA_LIFECYCLE_QOS_POLICY__AUTODISPOSE_UNREGISTERED_INSTANCES:
                setAutodispose_unregistered_instances((Boolean) newValue);
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
            case OpenDDSPackage.WRITER_DATA_LIFECYCLE_QOS_POLICY__AUTODISPOSE_UNREGISTERED_INSTANCES:
                setAutodispose_unregistered_instances(AUTODISPOSE_UNREGISTERED_INSTANCES_EDEFAULT);
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
            case OpenDDSPackage.WRITER_DATA_LIFECYCLE_QOS_POLICY__AUTODISPOSE_UNREGISTERED_INSTANCES:
                return autodispose_unregistered_instances != AUTODISPOSE_UNREGISTERED_INSTANCES_EDEFAULT;
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
        result.append(" (autodispose_unregistered_instances: ");
        result.append(autodispose_unregistered_instances);
        result.append(')');
        return result.toString();
    }

} //WriterDataLifecycleQosPolicyImpl
