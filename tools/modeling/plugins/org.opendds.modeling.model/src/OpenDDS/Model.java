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
import org.eclipse.emf.ecore.EObject;

/**
 * <!-- begin-user-doc --> A representation of the model object '
 * <em><b>Model</b></em>'. <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.Model#getEntities <em>Entities</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.OpenDDSPackage#getModel()
 * @model
 * @generated
 */
public interface Model extends EObject {
    /**
     * Returns the value of the '<em><b>Entities</b></em>' containment reference list.
     * The list contents are of type {@link OpenDDS.ModelEntity}.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Entities</em>' containment reference
     * list isn't clear, there really should be more of a description
     * here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Entities</em>' containment reference list.
     * @see OpenDDS.OpenDDSPackage#getModel_Entities()
     * @model containment="true"
     * @generated
     */
    EList<ModelEntity> getEntities();

} // Model
