/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.common.notify.Adapter;
import org.eclipse.emf.common.notify.Notifier;

import org.eclipse.emf.common.notify.impl.AdapterFactoryImpl;

import org.eclipse.emf.ecore.EObject;

/**
 * <!-- begin-user-doc -->
 * The <b>Adapter Factory</b> for the model.
 * It provides an adapter <code>createXXX</code> method for each class of the model.
 * <!-- end-user-doc -->
 * @see OpenDDS.ModelPackage
 * @generated
 */
public class ModelAdapterFactory extends AdapterFactoryImpl {
    /**
     * The cached model package.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected static ModelPackage modelPackage;

    /**
     * Creates an instance of the adapter factory.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ModelAdapterFactory() {
        if (modelPackage == null) {
            modelPackage = ModelPackage.eINSTANCE;
        }
    }

    /**
     * Returns whether this factory is applicable for the type of the object.
     * <!-- begin-user-doc -->
     * This implementation returns <code>true</code> if the object is either the model's package or is an instance object of the model.
     * <!-- end-user-doc -->
     * @return whether this factory is applicable for the type of the object.
     * @generated
     */
    @Override
    public boolean isFactoryForType(Object object) {
        if (object == modelPackage) {
            return true;
        }
        if (object instanceof EObject) {
            return ((EObject) object).eClass().getEPackage() == modelPackage;
        }
        return false;
    }

    /**
     * The switch that delegates to the <code>createXXX</code> methods.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    protected ModelSwitch<Adapter> modelSwitch = new ModelSwitch<Adapter>() {
        @Override
        public Adapter caseEntity(Entity object) {
            return createEntityAdapter();
        }

        @Override
        public Adapter caseNamedEntity(NamedEntity object) {
            return createNamedEntityAdapter();
        }

        @Override
        public Adapter caseSpecification(Specification object) {
            return createSpecificationAdapter();
        }

        @Override
        public Adapter caseDomainEntity(DomainEntity object) {
            return createDomainEntityAdapter();
        }

        @Override
        public Adapter caseContentFilteredTopic(ContentFilteredTopic object) {
            return createContentFilteredTopicAdapter();
        }

        @Override
        public Adapter caseMultiTopic(MultiTopic object) {
            return createMultiTopicAdapter();
        }

        @Override
        public Adapter caseTopic(Topic object) {
            return createTopicAdapter();
        }

        @Override
        public Adapter caseTopicDescription(TopicDescription object) {
            return createTopicDescriptionAdapter();
        }

        @Override
        public Adapter caseArray(Array object) {
            return createArrayAdapter();
        }

        @Override
        public Adapter caseOBoolean(OBoolean object) {
            return createOBooleanAdapter();
        }

        @Override
        public Adapter caseCase(Case object) {
            return createCaseAdapter();
        }

        @Override
        public Adapter caseOChar(OChar object) {
            return createOCharAdapter();
        }

        @Override
        public Adapter caseCollection(Collection object) {
            return createCollectionAdapter();
        }

        @Override
        public Adapter caseConstructedTopicType(ConstructedTopicType object) {
            return createConstructedTopicTypeAdapter();
        }

        @Override
        public Adapter caseODouble(ODouble object) {
            return createODoubleAdapter();
        }

        @Override
        public Adapter caseEnum(Enum object) {
            return createEnumAdapter();
        }

        @Override
        public Adapter caseOFloat(OFloat object) {
            return createOFloatAdapter();
        }

        @Override
        public Adapter caseKey(Key object) {
            return createKeyAdapter();
        }

        @Override
        public Adapter caseKeyField(KeyField object) {
            return createKeyFieldAdapter();
        }

        @Override
        public Adapter caseOLong(OLong object) {
            return createOLongAdapter();
        }

        @Override
        public Adapter caseOLongLong(OLongLong object) {
            return createOLongLongAdapter();
        }

        @Override
        public Adapter caseOOctet(OOctet object) {
            return createOOctetAdapter();
        }

        @Override
        public Adapter caseSequence(Sequence object) {
            return createSequenceAdapter();
        }

        @Override
        public Adapter caseOShort(OShort object) {
            return createOShortAdapter();
        }

        @Override
        public Adapter caseSimple(Simple object) {
            return createSimpleAdapter();
        }

        @Override
        public Adapter caseOString(OString object) {
            return createOStringAdapter();
        }

        @Override
        public Adapter caseTopicStruct(TopicStruct object) {
            return createTopicStructAdapter();
        }

        @Override
        public Adapter caseTopicField(TopicField object) {
            return createTopicFieldAdapter();
        }

        @Override
        public Adapter caseTypedef(Typedef object) {
            return createTypedefAdapter();
        }

        @Override
        public Adapter caseOULong(OULong object) {
            return createOULongAdapter();
        }

        @Override
        public Adapter caseOULongLong(OULongLong object) {
            return createOULongLongAdapter();
        }

        @Override
        public Adapter caseUnion(Union object) {
            return createUnionAdapter();
        }

        @Override
        public Adapter caseOUShort(OUShort object) {
            return createOUShortAdapter();
        }

        @Override
        public Adapter caseDataReader(DataReader object) {
            return createDataReaderAdapter();
        }

        @Override
        public Adapter caseDataReaderWriter(DataReaderWriter object) {
            return createDataReaderWriterAdapter();
        }

        @Override
        public Adapter caseDataWriter(DataWriter object) {
            return createDataWriterAdapter();
        }

        @Override
        public Adapter caseDomain(Domain object) {
            return createDomainAdapter();
        }

        @Override
        public Adapter caseDomainParticipant(DomainParticipant object) {
            return createDomainParticipantAdapter();
        }

        @Override
        public Adapter casePublisher(Publisher object) {
            return createPublisherAdapter();
        }

        @Override
        public Adapter casePublisherSubscriber(PublisherSubscriber object) {
            return createPublisherSubscriberAdapter();
        }

        @Override
        public Adapter caseSubscriber(Subscriber object) {
            return createSubscriberAdapter();
        }

        @Override
        public Adapter caseDeadlineQosPolicy(DeadlineQosPolicy object) {
            return createDeadlineQosPolicyAdapter();
        }

        @Override
        public Adapter caseDestinationOrderQosPolicy(DestinationOrderQosPolicy object) {
            return createDestinationOrderQosPolicyAdapter();
        }

        @Override
        public Adapter caseDurabilityQosPolicy(DurabilityQosPolicy object) {
            return createDurabilityQosPolicyAdapter();
        }

        @Override
        public Adapter caseDurabilityServiceQosPolicy(DurabilityServiceQosPolicy object) {
            return createDurabilityServiceQosPolicyAdapter();
        }

        @Override
        public Adapter caseEntityFactoryQosPolicy(EntityFactoryQosPolicy object) {
            return createEntityFactoryQosPolicyAdapter();
        }

        @Override
        public Adapter caseGroupDataQosPolicy(GroupDataQosPolicy object) {
            return createGroupDataQosPolicyAdapter();
        }

        @Override
        public Adapter caseHistoryQosPolicy(HistoryQosPolicy object) {
            return createHistoryQosPolicyAdapter();
        }

        @Override
        public Adapter caseLatencyBudgetQosPolicy(LatencyBudgetQosPolicy object) {
            return createLatencyBudgetQosPolicyAdapter();
        }

        @Override
        public Adapter caseLifespanQosPolicy(LifespanQosPolicy object) {
            return createLifespanQosPolicyAdapter();
        }

        @Override
        public Adapter caseLivelinessQosPolicy(LivelinessQosPolicy object) {
            return createLivelinessQosPolicyAdapter();
        }

        @Override
        public Adapter caseOwnershipQosPolicy(OwnershipQosPolicy object) {
            return createOwnershipQosPolicyAdapter();
        }

        @Override
        public Adapter caseOwnershipStrengthQosPolicy(OwnershipStrengthQosPolicy object) {
            return createOwnershipStrengthQosPolicyAdapter();
        }

        @Override
        public Adapter casePartitionQosPolicy(PartitionQosPolicy object) {
            return createPartitionQosPolicyAdapter();
        }

        @Override
        public Adapter casePresentationQosPolicy(PresentationQosPolicy object) {
            return createPresentationQosPolicyAdapter();
        }

        @Override
        public Adapter caseQosPolicy(QosPolicy object) {
            return createQosPolicyAdapter();
        }

        @Override
        public Adapter caseReaderDataLifecycleQosPolicy(ReaderDataLifecycleQosPolicy object) {
            return createReaderDataLifecycleQosPolicyAdapter();
        }

        @Override
        public Adapter caseReliabilityQosPolicy(ReliabilityQosPolicy object) {
            return createReliabilityQosPolicyAdapter();
        }

        @Override
        public Adapter caseResourceLimitsQosPolicy(ResourceLimitsQosPolicy object) {
            return createResourceLimitsQosPolicyAdapter();
        }

        @Override
        public Adapter caseTimeBasedFilterQosPolicy(TimeBasedFilterQosPolicy object) {
            return createTimeBasedFilterQosPolicyAdapter();
        }

        @Override
        public Adapter caseTopicDataQosPolicy(TopicDataQosPolicy object) {
            return createTopicDataQosPolicyAdapter();
        }

        @Override
        public Adapter caseTransportPriorityQosPolicy(TransportPriorityQosPolicy object) {
            return createTransportPriorityQosPolicyAdapter();
        }

        @Override
        public Adapter caseUserDataQosPolicy(UserDataQosPolicy object) {
            return createUserDataQosPolicyAdapter();
        }

        @Override
        public Adapter casePeriod(Period object) {
            return createPeriodAdapter();
        }

        @Override
        public Adapter caseWriterDataLifecycleQosPolicy(WriterDataLifecycleQosPolicy object) {
            return createWriterDataLifecycleQosPolicyAdapter();
        }

        @Override
        public Adapter caseApplicationTarget(ApplicationTarget object) {
            return createApplicationTargetAdapter();
        }

        @Override
        public Adapter caseTransport(Transport object) {
            return createTransportAdapter();
        }

        @Override
        public Adapter defaultCase(EObject object) {
            return createEObjectAdapter();
        }
    };

    /**
     * Creates an adapter for the <code>target</code>.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @param target the object to adapt.
     * @return the adapter for the <code>target</code>.
     * @generated
     */
    @Override
    public Adapter createAdapter(Notifier target) {
        return modelSwitch.doSwitch((EObject) target);
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Entity <em>Entity</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Entity
     * @generated
     */
    public Adapter createEntityAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.NamedEntity <em>Named Entity</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.NamedEntity
     * @generated
     */
    public Adapter createNamedEntityAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Specification <em>Specification</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Specification
     * @generated
     */
    public Adapter createSpecificationAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DomainEntity <em>Domain Entity</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DomainEntity
     * @generated
     */
    public Adapter createDomainEntityAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.ContentFilteredTopic <em>Content Filtered Topic</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.ContentFilteredTopic
     * @generated
     */
    public Adapter createContentFilteredTopicAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.MultiTopic <em>Multi Topic</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.MultiTopic
     * @generated
     */
    public Adapter createMultiTopicAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Topic <em>Topic</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Topic
     * @generated
     */
    public Adapter createTopicAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.TopicDescription <em>Topic Description</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.TopicDescription
     * @generated
     */
    public Adapter createTopicDescriptionAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Array <em>Array</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Array
     * @generated
     */
    public Adapter createArrayAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OBoolean <em>OBoolean</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OBoolean
     * @generated
     */
    public Adapter createOBooleanAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Case <em>Case</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Case
     * @generated
     */
    public Adapter createCaseAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OChar <em>OChar</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OChar
     * @generated
     */
    public Adapter createOCharAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Collection <em>Collection</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Collection
     * @generated
     */
    public Adapter createCollectionAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.ConstructedTopicType <em>Constructed Topic Type</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.ConstructedTopicType
     * @generated
     */
    public Adapter createConstructedTopicTypeAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.ODouble <em>ODouble</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.ODouble
     * @generated
     */
    public Adapter createODoubleAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Enum <em>Enum</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Enum
     * @generated
     */
    public Adapter createEnumAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OFloat <em>OFloat</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OFloat
     * @generated
     */
    public Adapter createOFloatAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Key <em>Key</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Key
     * @generated
     */
    public Adapter createKeyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.KeyField <em>Key Field</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.KeyField
     * @generated
     */
    public Adapter createKeyFieldAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OLong <em>OLong</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OLong
     * @generated
     */
    public Adapter createOLongAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OLongLong <em>OLong Long</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OLongLong
     * @generated
     */
    public Adapter createOLongLongAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OOctet <em>OOctet</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OOctet
     * @generated
     */
    public Adapter createOOctetAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Sequence <em>Sequence</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Sequence
     * @generated
     */
    public Adapter createSequenceAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OShort <em>OShort</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OShort
     * @generated
     */
    public Adapter createOShortAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Simple <em>Simple</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Simple
     * @generated
     */
    public Adapter createSimpleAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OString <em>OString</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OString
     * @generated
     */
    public Adapter createOStringAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.TopicStruct <em>Topic Struct</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.TopicStruct
     * @generated
     */
    public Adapter createTopicStructAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.TopicField <em>Topic Field</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.TopicField
     * @generated
     */
    public Adapter createTopicFieldAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Typedef <em>Typedef</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Typedef
     * @generated
     */
    public Adapter createTypedefAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OULong <em>OU Long</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OULong
     * @generated
     */
    public Adapter createOULongAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OULongLong <em>OU Long Long</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OULongLong
     * @generated
     */
    public Adapter createOULongLongAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Union <em>Union</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Union
     * @generated
     */
    public Adapter createUnionAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OUShort <em>OU Short</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OUShort
     * @generated
     */
    public Adapter createOUShortAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DataReader <em>Data Reader</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DataReader
     * @generated
     */
    public Adapter createDataReaderAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DataReaderWriter <em>Data Reader Writer</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DataReaderWriter
     * @generated
     */
    public Adapter createDataReaderWriterAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DataWriter <em>Data Writer</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DataWriter
     * @generated
     */
    public Adapter createDataWriterAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Domain <em>Domain</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Domain
     * @generated
     */
    public Adapter createDomainAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DomainParticipant <em>Domain Participant</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DomainParticipant
     * @generated
     */
    public Adapter createDomainParticipantAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Publisher <em>Publisher</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Publisher
     * @generated
     */
    public Adapter createPublisherAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.PublisherSubscriber <em>Publisher Subscriber</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.PublisherSubscriber
     * @generated
     */
    public Adapter createPublisherSubscriberAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Subscriber <em>Subscriber</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Subscriber
     * @generated
     */
    public Adapter createSubscriberAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DeadlineQosPolicy <em>Deadline Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DeadlineQosPolicy
     * @generated
     */
    public Adapter createDeadlineQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DestinationOrderQosPolicy <em>Destination Order Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DestinationOrderQosPolicy
     * @generated
     */
    public Adapter createDestinationOrderQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DurabilityQosPolicy <em>Durability Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DurabilityQosPolicy
     * @generated
     */
    public Adapter createDurabilityQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.DurabilityServiceQosPolicy <em>Durability Service Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.DurabilityServiceQosPolicy
     * @generated
     */
    public Adapter createDurabilityServiceQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.EntityFactoryQosPolicy <em>Entity Factory Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.EntityFactoryQosPolicy
     * @generated
     */
    public Adapter createEntityFactoryQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.GroupDataQosPolicy <em>Group Data Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.GroupDataQosPolicy
     * @generated
     */
    public Adapter createGroupDataQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.HistoryQosPolicy <em>History Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.HistoryQosPolicy
     * @generated
     */
    public Adapter createHistoryQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.LatencyBudgetQosPolicy <em>Latency Budget Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.LatencyBudgetQosPolicy
     * @generated
     */
    public Adapter createLatencyBudgetQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.LifespanQosPolicy <em>Lifespan Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.LifespanQosPolicy
     * @generated
     */
    public Adapter createLifespanQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.LivelinessQosPolicy <em>Liveliness Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.LivelinessQosPolicy
     * @generated
     */
    public Adapter createLivelinessQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OwnershipQosPolicy <em>Ownership Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OwnershipQosPolicy
     * @generated
     */
    public Adapter createOwnershipQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.OwnershipStrengthQosPolicy <em>Ownership Strength Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.OwnershipStrengthQosPolicy
     * @generated
     */
    public Adapter createOwnershipStrengthQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.PartitionQosPolicy <em>Partition Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.PartitionQosPolicy
     * @generated
     */
    public Adapter createPartitionQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.PresentationQosPolicy <em>Presentation Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.PresentationQosPolicy
     * @generated
     */
    public Adapter createPresentationQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.QosPolicy <em>Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.QosPolicy
     * @generated
     */
    public Adapter createQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.ReaderDataLifecycleQosPolicy <em>Reader Data Lifecycle Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.ReaderDataLifecycleQosPolicy
     * @generated
     */
    public Adapter createReaderDataLifecycleQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.ReliabilityQosPolicy <em>Reliability Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.ReliabilityQosPolicy
     * @generated
     */
    public Adapter createReliabilityQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.ResourceLimitsQosPolicy <em>Resource Limits Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.ResourceLimitsQosPolicy
     * @generated
     */
    public Adapter createResourceLimitsQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.TimeBasedFilterQosPolicy <em>Time Based Filter Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.TimeBasedFilterQosPolicy
     * @generated
     */
    public Adapter createTimeBasedFilterQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.TopicDataQosPolicy <em>Topic Data Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.TopicDataQosPolicy
     * @generated
     */
    public Adapter createTopicDataQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.TransportPriorityQosPolicy <em>Transport Priority Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.TransportPriorityQosPolicy
     * @generated
     */
    public Adapter createTransportPriorityQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.UserDataQosPolicy <em>User Data Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.UserDataQosPolicy
     * @generated
     */
    public Adapter createUserDataQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Period <em>Period</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Period
     * @generated
     */
    public Adapter createPeriodAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.WriterDataLifecycleQosPolicy <em>Writer Data Lifecycle Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.WriterDataLifecycleQosPolicy
     * @generated
     */
    public Adapter createWriterDataLifecycleQosPolicyAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.ApplicationTarget <em>Application Target</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.ApplicationTarget
     * @generated
     */
    public Adapter createApplicationTargetAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for an object of class '{@link OpenDDS.Transport <em>Transport</em>}'.
     * <!-- begin-user-doc -->
     * This default implementation returns null so that we can easily ignore cases;
     * it's useful to ignore a case when inheritance will catch all the cases anyway.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @see OpenDDS.Transport
     * @generated
     */
    public Adapter createTransportAdapter() {
        return null;
    }

    /**
     * Creates a new adapter for the default case.
     * <!-- begin-user-doc -->
     * This default implementation returns null.
     * <!-- end-user-doc -->
     * @return the new adapter.
     * @generated
     */
    public Adapter createEObjectAdapter() {
        return null;
    }

} //ModelAdapterFactory
