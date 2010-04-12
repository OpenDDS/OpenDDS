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
 * <em><b>Type Model</b></em>'. <!-- end-user-doc -->
 * 
 * <p>
 * The following features are supported:
 * <ul>
 * <li>{@link OpenDDS.TypeModel#getTypes <em>Types</em>}</li>
 * </ul>
 * </p>
 * 
 * @see OpenDDS.OpenDDSPackage#getTypeModel()
 * @model
 * @generated
 */
public interface TypeModel extends Model {
    /**
     * Returns the value of the '<em><b>Types</b></em>' containment
     * reference list. The list contents are of type
     * {@link OpenDDS.TopicStruct}. <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Types</em>' containment reference
     * list isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * 
     * @return the value of the '<em>Types</em>' containment reference
     *         list.
     * @see OpenDDS.OpenDDSPackage#getTypeModel_Types()
     * @model containment="true"
     * @generated
     */
    EList<TopicStruct> getTypes();

} // TypeModel
