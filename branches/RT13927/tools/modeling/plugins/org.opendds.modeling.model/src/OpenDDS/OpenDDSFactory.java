/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.ecore.EFactory;

/**
 * <!-- begin-user-doc --> The <b>Factory</b> for the model. It
 * provides a create method for each non-abstract class of the model.
 * <!-- end-user-doc -->
 * 
 * @see OpenDDS.OpenDDSPackage
 * @generated
 */
public interface OpenDDSFactory extends EFactory {
    /**
     * The singleton instance of the factory. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @generated
     */
    OpenDDSFactory eINSTANCE = OpenDDS.OpenDDSFactoryImpl.init();

    /**
     * Returns a new object of class '<em>Content Filtered Topic</em>
     * '. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Content Filtered Topic</em>
     *         '.
     * @generated
     */
    ContentFilteredTopic createContentFilteredTopic();

    /**
     * Returns a new object of class '<em>Multi Topic</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Multi Topic</em>'.
     * @generated
     */
    MultiTopic createMultiTopic();

    /**
     * Returns a new object of class '<em>Topic</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Topic</em>'.
     * @generated
     */
    Topic createTopic();

    /**
     * Returns a new object of class '<em>Array</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Array</em>'.
     * @generated
     */
    Array createArray();

    /**
     * Returns a new object of class '<em>OBoolean</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OBoolean</em>'.
     * @generated
     */
    OBoolean createOBoolean();

    /**
     * Returns a new object of class '<em>Case</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Case</em>'.
     * @generated
     */
    Case createCase();

    /**
     * Returns a new object of class '<em>OChar</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OChar</em>'.
     * @generated
     */
    OChar createOChar();

    /**
     * Returns a new object of class '<em>ODouble</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>ODouble</em>'.
     * @generated
     */
    ODouble createODouble();

    /**
     * Returns a new object of class '<em>Enum</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Enum</em>'.
     * @generated
     */
    Enum createEnum();

    /**
     * Returns a new object of class '<em>OFloat</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OFloat</em>'.
     * @generated
     */
    OFloat createOFloat();

    /**
     * Returns a new object of class '<em>Key</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Key</em>'.
     * @generated
     */
    Key createKey();

    /**
     * Returns a new object of class '<em>Key Field</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Key Field</em>'.
     * @generated
     */
    KeyField createKeyField();

    /**
     * Returns a new object of class '<em>OLong</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OLong</em>'.
     * @generated
     */
    OLong createOLong();

    /**
     * Returns a new object of class '<em>OLong Long</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OLong Long</em>'.
     * @generated
     */
    OLongLong createOLongLong();

    /**
     * Returns a new object of class '<em>OOctet</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OOctet</em>'.
     * @generated
     */
    OOctet createOOctet();

    /**
     * Returns a new object of class '<em>Sequence</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Sequence</em>'.
     * @generated
     */
    Sequence createSequence();

    /**
     * Returns a new object of class '<em>OShort</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OShort</em>'.
     * @generated
     */
    OShort createOShort();

    /**
     * Returns a new object of class '<em>OString</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OString</em>'.
     * @generated
     */
    OString createOString();

    /**
     * Returns a new object of class '<em>Topic Struct</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Topic Struct</em>'.
     * @generated
     */
    TopicStruct createTopicStruct();

    /**
     * Returns a new object of class '<em>OU Long</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OU Long</em>'.
     * @generated
     */
    OULong createOULong();

    /**
     * Returns a new object of class '<em>OU Long Long</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OU Long Long</em>'.
     * @generated
     */
    OULongLong createOULongLong();

    /**
     * Returns a new object of class '<em>Union</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Union</em>'.
     * @generated
     */
    Union createUnion();

    /**
     * Returns a new object of class '<em>OU Short</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>OU Short</em>'.
     * @generated
     */
    OUShort createOUShort();

    /**
     * Returns a new object of class '<em>Data Reader</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Data Reader</em>'.
     * @generated
     */
    DataReader createDataReader();

    /**
     * Returns a new object of class '<em>Data Writer</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Data Writer</em>'.
     * @generated
     */
    DataWriter createDataWriter();

    /**
     * Returns a new object of class '<em>Domain</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Domain</em>'.
     * @generated
     */
    Domain createDomain();

    /**
     * Returns a new object of class '<em>Domain Participant</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Domain Participant</em>'.
     * @generated
     */
    DomainParticipant createDomainParticipant();

    /**
     * Returns a new object of class '<em>Publisher</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Publisher</em>'.
     * @generated
     */
    Publisher createPublisher();

    /**
     * Returns a new object of class '<em>Subscriber</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Subscriber</em>'.
     * @generated
     */
    Subscriber createSubscriber();

    /**
     * Returns a new object of class '<em>Deadline Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Deadline Qos Policy</em>'.
     * @generated
     */
    DeadlineQosPolicy createDeadlineQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Destination Order Qos Policy</em>'. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Destination Order Qos Policy</em>'.
     * @generated
     */
    DestinationOrderQosPolicy createDestinationOrderQosPolicy();

    /**
     * Returns a new object of class '<em>Durability Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Durability Qos Policy</em>'.
     * @generated
     */
    DurabilityQosPolicy createDurabilityQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Durability Service Qos Policy</em>'. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Durability Service Qos Policy</em>'.
     * @generated
     */
    DurabilityServiceQosPolicy createDurabilityServiceQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Entity Factory Qos Policy</em>'. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Entity Factory Qos Policy</em>'.
     * @generated
     */
    EntityFactoryQosPolicy createEntityFactoryQosPolicy();

    /**
     * Returns a new object of class '<em>Group Data Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Group Data Qos Policy</em>'.
     * @generated
     */
    GroupDataQosPolicy createGroupDataQosPolicy();

    /**
     * Returns a new object of class '<em>History Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>History Qos Policy</em>'.
     * @generated
     */
    HistoryQosPolicy createHistoryQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Latency Budget Qos Policy</em>'. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Latency Budget Qos Policy</em>'.
     * @generated
     */
    LatencyBudgetQosPolicy createLatencyBudgetQosPolicy();

    /**
     * Returns a new object of class '<em>Lifespan Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Lifespan Qos Policy</em>'.
     * @generated
     */
    LifespanQosPolicy createLifespanQosPolicy();

    /**
     * Returns a new object of class '<em>Liveliness Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Liveliness Qos Policy</em>'.
     * @generated
     */
    LivelinessQosPolicy createLivelinessQosPolicy();

    /**
     * Returns a new object of class '<em>Ownership Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Ownership Qos Policy</em>'.
     * @generated
     */
    OwnershipQosPolicy createOwnershipQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Ownership Strength Qos Policy</em>'. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Ownership Strength Qos Policy</em>'.
     * @generated
     */
    OwnershipStrengthQosPolicy createOwnershipStrengthQosPolicy();

    /**
     * Returns a new object of class '<em>Partition Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Partition Qos Policy</em>'.
     * @generated
     */
    PartitionQosPolicy createPartitionQosPolicy();

    /**
     * Returns a new object of class '<em>Presentation Qos Policy</em>
     * '. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Presentation Qos Policy</em>
     *         '.
     * @generated
     */
    PresentationQosPolicy createPresentationQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Reader Data Lifecycle Qos Policy</em>'. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Reader Data Lifecycle Qos Policy</em>'.
     * @generated
     */
    ReaderDataLifecycleQosPolicy createReaderDataLifecycleQosPolicy();

    /**
     * Returns a new object of class '<em>Reliability Qos Policy</em>
     * '. <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Reliability Qos Policy</em>
     *         '.
     * @generated
     */
    ReliabilityQosPolicy createReliabilityQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Resource Limits Qos Policy</em>'. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Resource Limits Qos Policy</em>'.
     * @generated
     */
    ResourceLimitsQosPolicy createResourceLimitsQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Time Based Filter Qos Policy</em>'. <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Time Based Filter Qos Policy</em>'.
     * @generated
     */
    TimeBasedFilterQosPolicy createTimeBasedFilterQosPolicy();

    /**
     * Returns a new object of class '<em>Topic Data Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Topic Data Qos Policy</em>'.
     * @generated
     */
    TopicDataQosPolicy createTopicDataQosPolicy();

    /**
     * Returns a new object of class '
     * <em>Transport Priority Qos Policy</em>'. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Transport Priority Qos Policy</em>'.
     * @generated
     */
    TransportPriorityQosPolicy createTransportPriorityQosPolicy();

    /**
     * Returns a new object of class '<em>User Data Qos Policy</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>User Data Qos Policy</em>'.
     * @generated
     */
    UserDataQosPolicy createUserDataQosPolicy();

    /**
     * Returns a new object of class '<em>Period</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Period</em>'.
     * @generated
     */
    Period createPeriod();

    /**
     * Returns a new object of class '
     * <em>Writer Data Lifecycle Qos Policy</em>'. <!-- begin-user-doc
     * --> <!-- end-user-doc -->
     * 
     * @return a new object of class '
     *         <em>Writer Data Lifecycle Qos Policy</em>'.
     * @generated
     */
    WriterDataLifecycleQosPolicy createWriterDataLifecycleQosPolicy();

    /**
     * Returns a new object of class '<em>Application Target</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Application Target</em>'.
     * @generated
     */
    ApplicationTarget createApplicationTarget();

    /**
     * Returns a new object of class '<em>Transport</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Transport</em>'.
     * @generated
     */
    Transport createTransport();

    /**
     * Returns a new object of class '<em>Application Model</em>'.
     * <!-- begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Application Model</em>'.
     * @generated
     */
    ApplicationModel createApplicationModel();

    /**
     * Returns a new object of class '<em>Domain Model</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Domain Model</em>'.
     * @generated
     */
    DomainModel createDomainModel();

    /**
     * Returns a new object of class '<em>Type Model</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Type Model</em>'.
     * @generated
     */
    TypeModel createTypeModel();

    /**
     * Returns a new object of class '<em>Qos Model</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Qos Model</em>'.
     * @generated
     */
    QosModel createQosModel();

    /**
     * Returns a new object of class '<em>Transport Model</em>'. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return a new object of class '<em>Transport Model</em>'.
     * @generated
     */
    TransportModel createTransportModel();

    /**
     * Returns the package supported by this factory. <!--
     * begin-user-doc --> <!-- end-user-doc -->
     * 
     * @return the package supported by this factory.
     * @generated
     */
    OpenDDSPackage getOpenDDSPackage();

} // OpenDDSFactory
