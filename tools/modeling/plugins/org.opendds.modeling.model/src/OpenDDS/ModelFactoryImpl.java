/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EDataType;
import org.eclipse.emf.ecore.EObject;
import org.eclipse.emf.ecore.EPackage;

import org.eclipse.emf.ecore.impl.EFactoryImpl;

import org.eclipse.emf.ecore.plugin.EcorePlugin;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model <b>Factory</b>.
 * <!-- end-user-doc -->
 * @generated
 */
public class ModelFactoryImpl extends EFactoryImpl implements ModelFactory {
    /**
     * Creates the default factory implementation.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public static ModelFactory init() {
        try {
            ModelFactory theModelFactory = (ModelFactory) EPackage.Registry.INSTANCE
                    .getEFactory("http://www.opendds.org/schemas/modeling/OpenDDS");
            if (theModelFactory != null) {
                return theModelFactory;
            }
        } catch (Exception exception) {
            EcorePlugin.INSTANCE.log(exception);
        }
        return new ModelFactoryImpl();
    }

    /**
     * Creates an instance of the factory.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ModelFactoryImpl() {
        super();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public EObject create(EClass eClass) {
        switch (eClass.getClassifierID()) {
            case ModelPackage.CONTENT_FILTERED_TOPIC:
                return createContentFilteredTopic();
            case ModelPackage.MULTI_TOPIC:
                return createMultiTopic();
            case ModelPackage.TOPIC:
                return createTopic();
            case ModelPackage.ARRAY:
                return createArray();
            case ModelPackage.OBOOLEAN:
                return createOBoolean();
            case ModelPackage.CASE:
                return createCase();
            case ModelPackage.OCHAR:
                return createOChar();
            case ModelPackage.ODOUBLE:
                return createODouble();
            case ModelPackage.ENUM:
                return createEnum();
            case ModelPackage.OFLOAT:
                return createOFloat();
            case ModelPackage.KEY:
                return createKey();
            case ModelPackage.KEY_FIELD:
                return createKeyField();
            case ModelPackage.OLONG:
                return createOLong();
            case ModelPackage.OLONG_LONG:
                return createOLongLong();
            case ModelPackage.OOCTET:
                return createOOctet();
            case ModelPackage.SEQUENCE:
                return createSequence();
            case ModelPackage.OSHORT:
                return createOShort();
            case ModelPackage.OSTRING:
                return createOString();
            case ModelPackage.TOPIC_STRUCT:
                return createTopicStruct();
            case ModelPackage.TYPEDEF:
                return createTypedef();
            case ModelPackage.OU_LONG:
                return createOULong();
            case ModelPackage.OU_LONG_LONG:
                return createOULongLong();
            case ModelPackage.UNION:
                return createUnion();
            case ModelPackage.OU_SHORT:
                return createOUShort();
            case ModelPackage.DATA_READER:
                return createDataReader();
            case ModelPackage.DATA_WRITER:
                return createDataWriter();
            case ModelPackage.DOMAIN:
                return createDomain();
            case ModelPackage.DOMAIN_PARTICIPANT:
                return createDomainParticipant();
            case ModelPackage.PUBLISHER:
                return createPublisher();
            case ModelPackage.SUBSCRIBER:
                return createSubscriber();
            case ModelPackage.DEADLINE_QOS_POLICY:
                return createDeadlineQosPolicy();
            case ModelPackage.DESTINATION_ORDER_QOS_POLICY:
                return createDestinationOrderQosPolicy();
            case ModelPackage.DURABILITY_QOS_POLICY:
                return createDurabilityQosPolicy();
            case ModelPackage.DURABILITY_SERVICE_QOS_POLICY:
                return createDurabilityServiceQosPolicy();
            case ModelPackage.ENTITY_FACTORY_QOS_POLICY:
                return createEntityFactoryQosPolicy();
            case ModelPackage.GROUP_DATA_QOS_POLICY:
                return createGroupDataQosPolicy();
            case ModelPackage.HISTORY_QOS_POLICY:
                return createHistoryQosPolicy();
            case ModelPackage.LATENCY_BUDGET_QOS_POLICY:
                return createLatencyBudgetQosPolicy();
            case ModelPackage.LIFESPAN_QOS_POLICY:
                return createLifespanQosPolicy();
            case ModelPackage.LIVELINESS_QOS_POLICY:
                return createLivelinessQosPolicy();
            case ModelPackage.OWNERSHIP_QOS_POLICY:
                return createOwnershipQosPolicy();
            case ModelPackage.OWNERSHIP_STRENGTH_QOS_POLICY:
                return createOwnershipStrengthQosPolicy();
            case ModelPackage.PARTITION_QOS_POLICY:
                return createPartitionQosPolicy();
            case ModelPackage.PRESENTATION_QOS_POLICY:
                return createPresentationQosPolicy();
            case ModelPackage.READER_DATA_LIFECYCLE_QOS_POLICY:
                return createReaderDataLifecycleQosPolicy();
            case ModelPackage.RELIABILITY_QOS_POLICY:
                return createReliabilityQosPolicy();
            case ModelPackage.RESOURCE_LIMITS_QOS_POLICY:
                return createResourceLimitsQosPolicy();
            case ModelPackage.TIME_BASED_FILTER_QOS_POLICY:
                return createTimeBasedFilterQosPolicy();
            case ModelPackage.TOPIC_DATA_QOS_POLICY:
                return createTopicDataQosPolicy();
            case ModelPackage.TRANSPORT_PRIORITY_QOS_POLICY:
                return createTransportPriorityQosPolicy();
            case ModelPackage.USER_DATA_QOS_POLICY:
                return createUserDataQosPolicy();
            case ModelPackage.PERIOD:
                return createPeriod();
            case ModelPackage.WRITER_DATA_LIFECYCLE_QOS_POLICY:
                return createWriterDataLifecycleQosPolicy();
            case ModelPackage.APPLICATION_TARGET:
                return createApplicationTarget();
            case ModelPackage.TRANSPORT:
                return createTransport();
            default:
                throw new IllegalArgumentException("The class '" + eClass.getName() + "' is not a valid classifier");
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public Object createFromString(EDataType eDataType, String initialValue) {
        switch (eDataType.getClassifierID()) {
            case ModelPackage.DESTINATION_ORDER_QOS_POLICY_KIND:
                return createDestinationOrderQosPolicyKindFromString(eDataType, initialValue);
            case ModelPackage.DURABILITY_QOS_POLICY_KIND:
                return createDurabilityQosPolicyKindFromString(eDataType, initialValue);
            case ModelPackage.HISTORY_QOS_POLICY_KIND:
                return createHistoryQosPolicyKindFromString(eDataType, initialValue);
            case ModelPackage.LIVELINESS_QOS_POLICY_KIND:
                return createLivelinessQosPolicyKindFromString(eDataType, initialValue);
            case ModelPackage.OWNERSHIP_QOS_POLICY_KIND:
                return createOwnershipQosPolicyKindFromString(eDataType, initialValue);
            case ModelPackage.PRESENTATION_QOS_POLICY_ACCESS_SCOPE_KIND:
                return createPresentationQosPolicyAccessScopeKindFromString(eDataType, initialValue);
            case ModelPackage.RELIABILITY_QOS_POLICY_KIND:
                return createReliabilityQosPolicyKindFromString(eDataType, initialValue);
            case ModelPackage.COMPONENT_TYPE:
                return createComponentTypeFromString(eDataType, initialValue);
            case ModelPackage.LANGUAGE_TYPE:
                return createLanguageTypeFromString(eDataType, initialValue);
            case ModelPackage.PLATFORM_TYPE:
                return createPlatformTypeFromString(eDataType, initialValue);
            default:
                throw new IllegalArgumentException("The datatype '" + eDataType.getName()
                        + "' is not a valid classifier");
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    @Override
    public String convertToString(EDataType eDataType, Object instanceValue) {
        switch (eDataType.getClassifierID()) {
            case ModelPackage.DESTINATION_ORDER_QOS_POLICY_KIND:
                return convertDestinationOrderQosPolicyKindToString(eDataType, instanceValue);
            case ModelPackage.DURABILITY_QOS_POLICY_KIND:
                return convertDurabilityQosPolicyKindToString(eDataType, instanceValue);
            case ModelPackage.HISTORY_QOS_POLICY_KIND:
                return convertHistoryQosPolicyKindToString(eDataType, instanceValue);
            case ModelPackage.LIVELINESS_QOS_POLICY_KIND:
                return convertLivelinessQosPolicyKindToString(eDataType, instanceValue);
            case ModelPackage.OWNERSHIP_QOS_POLICY_KIND:
                return convertOwnershipQosPolicyKindToString(eDataType, instanceValue);
            case ModelPackage.PRESENTATION_QOS_POLICY_ACCESS_SCOPE_KIND:
                return convertPresentationQosPolicyAccessScopeKindToString(eDataType, instanceValue);
            case ModelPackage.RELIABILITY_QOS_POLICY_KIND:
                return convertReliabilityQosPolicyKindToString(eDataType, instanceValue);
            case ModelPackage.COMPONENT_TYPE:
                return convertComponentTypeToString(eDataType, instanceValue);
            case ModelPackage.LANGUAGE_TYPE:
                return convertLanguageTypeToString(eDataType, instanceValue);
            case ModelPackage.PLATFORM_TYPE:
                return convertPlatformTypeToString(eDataType, instanceValue);
            default:
                throw new IllegalArgumentException("The datatype '" + eDataType.getName()
                        + "' is not a valid classifier");
        }
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ContentFilteredTopic createContentFilteredTopic() {
        ContentFilteredTopicImpl contentFilteredTopic = new ContentFilteredTopicImpl();
        return contentFilteredTopic;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public MultiTopic createMultiTopic() {
        MultiTopicImpl multiTopic = new MultiTopicImpl();
        return multiTopic;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Topic createTopic() {
        TopicImpl topic = new TopicImpl();
        return topic;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Array createArray() {
        ArrayImpl array = new ArrayImpl();
        return array;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OBoolean createOBoolean() {
        OBooleanImpl oBoolean = new OBooleanImpl();
        return oBoolean;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Case createCase() {
        CaseImpl case_ = new CaseImpl();
        return case_;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OChar createOChar() {
        OCharImpl oChar = new OCharImpl();
        return oChar;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ODouble createODouble() {
        ODoubleImpl oDouble = new ODoubleImpl();
        return oDouble;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Enum createEnum() {
        EnumImpl enum_ = new EnumImpl();
        return enum_;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OFloat createOFloat() {
        OFloatImpl oFloat = new OFloatImpl();
        return oFloat;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Key createKey() {
        KeyImpl key = new KeyImpl();
        return key;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public KeyField createKeyField() {
        KeyFieldImpl keyField = new KeyFieldImpl();
        return keyField;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OLong createOLong() {
        OLongImpl oLong = new OLongImpl();
        return oLong;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OLongLong createOLongLong() {
        OLongLongImpl oLongLong = new OLongLongImpl();
        return oLongLong;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OOctet createOOctet() {
        OOctetImpl oOctet = new OOctetImpl();
        return oOctet;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Sequence createSequence() {
        SequenceImpl sequence = new SequenceImpl();
        return sequence;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OShort createOShort() {
        OShortImpl oShort = new OShortImpl();
        return oShort;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OString createOString() {
        OStringImpl oString = new OStringImpl();
        return oString;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public TopicStruct createTopicStruct() {
        TopicStructImpl topicStruct = new TopicStructImpl();
        return topicStruct;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Typedef createTypedef() {
        TypedefImpl typedef = new TypedefImpl();
        return typedef;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OULong createOULong() {
        OULongImpl ouLong = new OULongImpl();
        return ouLong;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OULongLong createOULongLong() {
        OULongLongImpl ouLongLong = new OULongLongImpl();
        return ouLongLong;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Union createUnion() {
        UnionImpl union = new UnionImpl();
        return union;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OUShort createOUShort() {
        OUShortImpl ouShort = new OUShortImpl();
        return ouShort;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DataReader createDataReader() {
        DataReaderImpl dataReader = new DataReaderImpl();
        return dataReader;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DataWriter createDataWriter() {
        DataWriterImpl dataWriter = new DataWriterImpl();
        return dataWriter;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Domain createDomain() {
        DomainImpl domain = new DomainImpl();
        return domain;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DomainParticipant createDomainParticipant() {
        DomainParticipantImpl domainParticipant = new DomainParticipantImpl();
        return domainParticipant;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Publisher createPublisher() {
        PublisherImpl publisher = new PublisherImpl();
        return publisher;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Subscriber createSubscriber() {
        SubscriberImpl subscriber = new SubscriberImpl();
        return subscriber;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DeadlineQosPolicy createDeadlineQosPolicy() {
        DeadlineQosPolicyImpl deadlineQosPolicy = new DeadlineQosPolicyImpl();
        return deadlineQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DestinationOrderQosPolicy createDestinationOrderQosPolicy() {
        DestinationOrderQosPolicyImpl destinationOrderQosPolicy = new DestinationOrderQosPolicyImpl();
        return destinationOrderQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DurabilityQosPolicy createDurabilityQosPolicy() {
        DurabilityQosPolicyImpl durabilityQosPolicy = new DurabilityQosPolicyImpl();
        return durabilityQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DurabilityServiceQosPolicy createDurabilityServiceQosPolicy() {
        DurabilityServiceQosPolicyImpl durabilityServiceQosPolicy = new DurabilityServiceQosPolicyImpl();
        return durabilityServiceQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EntityFactoryQosPolicy createEntityFactoryQosPolicy() {
        EntityFactoryQosPolicyImpl entityFactoryQosPolicy = new EntityFactoryQosPolicyImpl();
        return entityFactoryQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public GroupDataQosPolicy createGroupDataQosPolicy() {
        GroupDataQosPolicyImpl groupDataQosPolicy = new GroupDataQosPolicyImpl();
        return groupDataQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public HistoryQosPolicy createHistoryQosPolicy() {
        HistoryQosPolicyImpl historyQosPolicy = new HistoryQosPolicyImpl();
        return historyQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LatencyBudgetQosPolicy createLatencyBudgetQosPolicy() {
        LatencyBudgetQosPolicyImpl latencyBudgetQosPolicy = new LatencyBudgetQosPolicyImpl();
        return latencyBudgetQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LifespanQosPolicy createLifespanQosPolicy() {
        LifespanQosPolicyImpl lifespanQosPolicy = new LifespanQosPolicyImpl();
        return lifespanQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LivelinessQosPolicy createLivelinessQosPolicy() {
        LivelinessQosPolicyImpl livelinessQosPolicy = new LivelinessQosPolicyImpl();
        return livelinessQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OwnershipQosPolicy createOwnershipQosPolicy() {
        OwnershipQosPolicyImpl ownershipQosPolicy = new OwnershipQosPolicyImpl();
        return ownershipQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OwnershipStrengthQosPolicy createOwnershipStrengthQosPolicy() {
        OwnershipStrengthQosPolicyImpl ownershipStrengthQosPolicy = new OwnershipStrengthQosPolicyImpl();
        return ownershipStrengthQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public PartitionQosPolicy createPartitionQosPolicy() {
        PartitionQosPolicyImpl partitionQosPolicy = new PartitionQosPolicyImpl();
        return partitionQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public PresentationQosPolicy createPresentationQosPolicy() {
        PresentationQosPolicyImpl presentationQosPolicy = new PresentationQosPolicyImpl();
        return presentationQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ReaderDataLifecycleQosPolicy createReaderDataLifecycleQosPolicy() {
        ReaderDataLifecycleQosPolicyImpl readerDataLifecycleQosPolicy = new ReaderDataLifecycleQosPolicyImpl();
        return readerDataLifecycleQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ReliabilityQosPolicy createReliabilityQosPolicy() {
        ReliabilityQosPolicyImpl reliabilityQosPolicy = new ReliabilityQosPolicyImpl();
        return reliabilityQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ResourceLimitsQosPolicy createResourceLimitsQosPolicy() {
        ResourceLimitsQosPolicyImpl resourceLimitsQosPolicy = new ResourceLimitsQosPolicyImpl();
        return resourceLimitsQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public TimeBasedFilterQosPolicy createTimeBasedFilterQosPolicy() {
        TimeBasedFilterQosPolicyImpl timeBasedFilterQosPolicy = new TimeBasedFilterQosPolicyImpl();
        return timeBasedFilterQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public TopicDataQosPolicy createTopicDataQosPolicy() {
        TopicDataQosPolicyImpl topicDataQosPolicy = new TopicDataQosPolicyImpl();
        return topicDataQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public TransportPriorityQosPolicy createTransportPriorityQosPolicy() {
        TransportPriorityQosPolicyImpl transportPriorityQosPolicy = new TransportPriorityQosPolicyImpl();
        return transportPriorityQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public UserDataQosPolicy createUserDataQosPolicy() {
        UserDataQosPolicyImpl userDataQosPolicy = new UserDataQosPolicyImpl();
        return userDataQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Period createPeriod() {
        PeriodImpl period = new PeriodImpl();
        return period;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public WriterDataLifecycleQosPolicy createWriterDataLifecycleQosPolicy() {
        WriterDataLifecycleQosPolicyImpl writerDataLifecycleQosPolicy = new WriterDataLifecycleQosPolicyImpl();
        return writerDataLifecycleQosPolicy;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ApplicationTarget createApplicationTarget() {
        ApplicationTargetImpl applicationTarget = new ApplicationTargetImpl();
        return applicationTarget;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public Transport createTransport() {
        TransportImpl transport = new TransportImpl();
        return transport;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DestinationOrderQosPolicyKind createDestinationOrderQosPolicyKindFromString(EDataType eDataType,
            String initialValue) {
        DestinationOrderQosPolicyKind result = DestinationOrderQosPolicyKind.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertDestinationOrderQosPolicyKindToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public DurabilityQosPolicyKind createDurabilityQosPolicyKindFromString(EDataType eDataType, String initialValue) {
        DurabilityQosPolicyKind result = DurabilityQosPolicyKind.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertDurabilityQosPolicyKindToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public HistoryQosPolicyKind createHistoryQosPolicyKindFromString(EDataType eDataType, String initialValue) {
        HistoryQosPolicyKind result = HistoryQosPolicyKind.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertHistoryQosPolicyKindToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LivelinessQosPolicyKind createLivelinessQosPolicyKindFromString(EDataType eDataType, String initialValue) {
        LivelinessQosPolicyKind result = LivelinessQosPolicyKind.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertLivelinessQosPolicyKindToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public OwnershipQosPolicyKind createOwnershipQosPolicyKindFromString(EDataType eDataType, String initialValue) {
        OwnershipQosPolicyKind result = OwnershipQosPolicyKind.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertOwnershipQosPolicyKindToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public PresentationQosPolicyAccessScopeKind createPresentationQosPolicyAccessScopeKindFromString(
            EDataType eDataType, String initialValue) {
        PresentationQosPolicyAccessScopeKind result = PresentationQosPolicyAccessScopeKind.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertPresentationQosPolicyAccessScopeKindToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ReliabilityQosPolicyKind createReliabilityQosPolicyKindFromString(EDataType eDataType, String initialValue) {
        ReliabilityQosPolicyKind result = ReliabilityQosPolicyKind.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertReliabilityQosPolicyKindToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ComponentType createComponentTypeFromString(EDataType eDataType, String initialValue) {
        ComponentType result = ComponentType.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertComponentTypeToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public LanguageType createLanguageTypeFromString(EDataType eDataType, String initialValue) {
        LanguageType result = LanguageType.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertLanguageTypeToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public PlatformType createPlatformTypeFromString(EDataType eDataType, String initialValue) {
        PlatformType result = PlatformType.get(initialValue);
        if (result == null)
            throw new IllegalArgumentException("The value '" + initialValue + "' is not a valid enumerator of '"
                    + eDataType.getName() + "'");
        return result;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public String convertPlatformTypeToString(EDataType eDataType, Object instanceValue) {
        return instanceValue == null ? null : instanceValue.toString();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ModelPackage getModelPackage() {
        return (ModelPackage) getEPackage();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @deprecated
     * @generated
     */
    @Deprecated
    public static ModelPackage getPackage() {
        return ModelPackage.eINSTANCE;
    }

} //ModelFactoryImpl
