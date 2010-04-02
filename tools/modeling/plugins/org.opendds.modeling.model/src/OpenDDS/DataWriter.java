/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

/**
 * <!-- begin-user-doc -->
 * A representation of the model object '<em><b>Data Writer</b></em>'.
 * <!-- end-user-doc -->
 *
 * <p>
 * The following features are supported:
 * <ul>
 *   <li>{@link OpenDDS.DataWriter#getTopic <em>Topic</em>}</li>
 *   <li>{@link OpenDDS.DataWriter#getWriter_data_lifecycle <em>Writer data lifecycle</em>}</li>
 * </ul>
 * </p>
 *
 * @see OpenDDS.ModelPackage#getDataWriter()
 * @model
 * @generated
 */
public interface DataWriter extends DataReaderWriter {
    /**
     * Returns the value of the '<em><b>Topic</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Topic</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Topic</em>' reference.
     * @see #setTopic(Topic)
     * @see OpenDDS.ModelPackage#getDataWriter_Topic()
     * @model required="true"
     * @generated
     */
    Topic getTopic();

    /**
     * Sets the value of the '{@link OpenDDS.DataWriter#getTopic <em>Topic</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Topic</em>' reference.
     * @see #getTopic()
     * @generated
     */
    void setTopic(Topic value);

    /**
     * Returns the value of the '<em><b>Writer data lifecycle</b></em>' reference.
     * <!-- begin-user-doc -->
     * <p>
     * If the meaning of the '<em>Writer data lifecycle</em>' reference isn't clear,
     * there really should be more of a description here...
     * </p>
     * <!-- end-user-doc -->
     * @return the value of the '<em>Writer data lifecycle</em>' reference.
     * @see #setWriter_data_lifecycle(WriterDataLifecycleQosPolicy)
     * @see OpenDDS.ModelPackage#getDataWriter_Writer_data_lifecycle()
     * @model
     * @generated
     */
    WriterDataLifecycleQosPolicy getWriter_data_lifecycle();

    /**
     * Sets the value of the '{@link OpenDDS.DataWriter#getWriter_data_lifecycle <em>Writer data lifecycle</em>}' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param value the new value of the '<em>Writer data lifecycle</em>' reference.
     * @see #getWriter_data_lifecycle()
     * @generated
     */
    void setWriter_data_lifecycle(WriterDataLifecycleQosPolicy value);

} // DataWriter
