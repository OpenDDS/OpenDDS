/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.common.util.EList;

/**
 * <!-- begin-user-doc --> A representation of the model object '
 * <em><b>Topic Struct</b></em>'. <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.TopicStruct#getMembers <em>Members</em>}</li>
 *   <li>{@link OpenDDS.TopicStruct#getKeys <em>Keys</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getTopicStruct()
 * @model
 * @generated
 */
public interface TopicStruct extends ConstructedTopicType, ModelEntity {
    /**
     * Returns the value of the '<em><b>Members</b></em>' containment reference list.
     * The list contents are of type {@link OpenDDS.TopicField}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Members</em>' containment reference
     * list isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Members</em>' containment reference list.
     * @see OpenDDS.OpenDDSPackage#getTopicStruct_Members()
     * @model containment="true" required="true"
     * @generated
     */
    EList<TopicField> getMembers();

    /**
     * Returns the value of the '<em><b>Keys</b></em>' containment reference list.
     * The list contents are of type {@link OpenDDS.Key}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Keys</em>' containment reference
     * list isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Keys</em>' containment reference list.
     * @see OpenDDS.OpenDDSPackage#getTopicStruct_Keys()
     * @model containment="true"
     * @generated
     */
    EList<Key> getKeys();

} // TopicStruct
