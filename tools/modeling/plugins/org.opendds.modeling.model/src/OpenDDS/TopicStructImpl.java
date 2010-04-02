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
 * <!-- begin-user-doc -->
 * An implementation of the model object '<em><b>Topic Struct</b></em>'.
 * <!-- end-user-doc -->
 * <p>
 * The following features are implemented:
 * <ul>
 *   <li>{@link OpenDDS.TopicStructImpl#getMembers <em>Members</em>}</li>
 *   <li>{@link OpenDDS.TopicStructImpl#getKeys <em>Keys</em>}</li>
 * </ul>
 * </p>
 *
 * @generated
 */
public class TopicStructImpl extends ConstructedTopicTypeImpl implements TopicStruct {
    /**
     * The cached value of the '{@link #getMembers() <em>Members</em>}' containment reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getMembers()
     * @generated
     * @ordered
     */
    protected EList<TopicField> members;

    /**
     * The cached value of the '{@link #getKeys() <em>Keys</em>}' containment reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #getKeys()
     * @generated
     * @ordered
     */
    protected EList<Key> keys;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected TopicStructImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    protected EClass eStaticClass() {
        return ModelPackage.Literals.TOPIC_STRUCT;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EList<TopicField> getMembers() {
        if (members == null) {
            members = new EObjectContainmentEList<TopicField>(TopicField.class, this,
                    ModelPackage.TOPIC_STRUCT__MEMBERS);
        }
        return members;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EList<Key> getKeys() {
        if (keys == null) {
            keys = new EObjectContainmentEList<Key>(Key.class, this, ModelPackage.TOPIC_STRUCT__KEYS);
        }
        return keys;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public NotificationChain eInverseRemove(InternalEObject otherEnd, int featureID, NotificationChain msgs) {
        switch (featureID) {
            case ModelPackage.TOPIC_STRUCT__MEMBERS:
                return ((InternalEList<?>) getMembers()).basicRemove(otherEnd, msgs);
            case ModelPackage.TOPIC_STRUCT__KEYS:
                return ((InternalEList<?>) getKeys()).basicRemove(otherEnd, msgs);
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
            case ModelPackage.TOPIC_STRUCT__MEMBERS:
                return getMembers();
            case ModelPackage.TOPIC_STRUCT__KEYS:
                return getKeys();
        }
        return super.eGet(featureID, resolve, coreType);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @SuppressWarnings("unchecked")
    @Override
    public void eSet(int featureID, Object newValue) {
        switch (featureID) {
            case ModelPackage.TOPIC_STRUCT__MEMBERS:
                getMembers().clear();
                getMembers().addAll((Collection<? extends TopicField>) newValue);
                return;
            case ModelPackage.TOPIC_STRUCT__KEYS:
                getKeys().clear();
                getKeys().addAll((Collection<? extends Key>) newValue);
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
            case ModelPackage.TOPIC_STRUCT__MEMBERS:
                getMembers().clear();
                return;
            case ModelPackage.TOPIC_STRUCT__KEYS:
                getKeys().clear();
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
            case ModelPackage.TOPIC_STRUCT__MEMBERS:
                return members != null && !members.isEmpty();
            case ModelPackage.TOPIC_STRUCT__KEYS:
                return keys != null && !keys.isEmpty();
        }
        return super.eIsSet(featureID);
    }

} //TopicStructImpl
