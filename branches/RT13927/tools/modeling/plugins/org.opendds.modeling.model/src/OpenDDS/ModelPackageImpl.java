/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

package OpenDDS;

import org.eclipse.emf.ecore.EAttribute;
import org.eclipse.emf.ecore.EClass;
import org.eclipse.emf.ecore.EEnum;
import org.eclipse.emf.ecore.EPackage;
import org.eclipse.emf.ecore.EReference;
import org.eclipse.emf.ecore.impl.EPackageImpl;

/**
 * <!-- begin-user-doc -->
 * An implementation of the model <b>Package</b>.
 * <!-- end-user-doc -->
 * @generated
 */
public class ModelPackageImpl extends EPackageImpl implements ModelPackage {
    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass entityEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass namedEntityEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass specificationEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass domainEntityEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass contentFilteredTopicEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass multiTopicEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass topicEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass topicDescriptionEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass arrayEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oBooleanEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass caseEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oCharEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass collectionEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass constructedTopicTypeEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oDoubleEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass enumEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oFloatEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass keyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass keyFieldEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oLongEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oLongLongEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oOctetEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass sequenceEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oShortEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass simpleEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass oStringEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass topicStructEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass topicFieldEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass typedefEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass ouLongEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass ouLongLongEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass unionEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass ouShortEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass dataReaderEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass dataReaderWriterEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass dataWriterEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass domainEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass domainParticipantEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass publisherEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass publisherSubscriberEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass subscriberEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass deadlineQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass destinationOrderQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass durabilityQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass durabilityServiceQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass entityFactoryQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass groupDataQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass historyQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass latencyBudgetQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass lifespanQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass livelinessQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass ownershipQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass ownershipStrengthQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass partitionQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass presentationQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass qosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass readerDataLifecycleQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass reliabilityQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass resourceLimitsQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass timeBasedFilterQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass topicDataQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass transportPriorityQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass userDataQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass periodEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass writerDataLifecycleQosPolicyEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass applicationTargetEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EClass transportEClass = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum destinationOrderQosPolicyKindEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum durabilityQosPolicyKindEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum historyQosPolicyKindEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum livelinessQosPolicyKindEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum ownershipQosPolicyKindEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum presentationQosPolicyAccessScopeKindEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum reliabilityQosPolicyKindEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum componentTypeEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum languageTypeEEnum = null;

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private EEnum platformTypeEEnum = null;

    /**
     * Creates an instance of the model <b>Package</b>, registered with
     * {@link org.eclipse.emf.ecore.EPackage.Registry EPackage.Registry} by the package
     * package URI value.
     * <p>Note: the correct way to create the package is via the static
     * factory method {@link #init init()}, which also performs
     * initialization of the package, or returns the registered package,
     * if one already exists.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see org.eclipse.emf.ecore.EPackage.Registry
     * @see OpenDDS.ModelPackage#eNS_URI
     * @see #init()
     * @generated
     */
    private ModelPackageImpl() {
        super(eNS_URI, ModelFactory.eINSTANCE);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private static boolean isInited = false;

    /**
     * Creates, registers, and initializes the <b>Package</b> for this model, and for any others upon which it depends.
     *
     * <p>This method is used to initialize {@link ModelPackage#eINSTANCE} when that field is accessed.
     * Clients should not invoke it directly. Instead, they should simply access that field to obtain the package.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see #eNS_URI
     * @see #createPackageContents()
     * @see #initializePackageContents()
     * @generated
     */
    public static ModelPackage init() {
        if (isInited) {
            return (ModelPackage) EPackage.Registry.INSTANCE.getEPackage(ModelPackage.eNS_URI);
        }

        // Obtain or create and register package
        ModelPackageImpl theModelPackage = (ModelPackageImpl) (EPackage.Registry.INSTANCE.get(eNS_URI) instanceof ModelPackageImpl ? EPackage.Registry.INSTANCE
                .get(eNS_URI)
                : new ModelPackageImpl());

        isInited = true;

        // Create package meta-data objects
        theModelPackage.createPackageContents();

        // Initialize created meta-data
        theModelPackage.initializePackageContents();

        // Mark meta-data to indicate it can't be changed
        theModelPackage.freeze();

        // Update the registry and return the package
        EPackage.Registry.INSTANCE.put(ModelPackage.eNS_URI, theModelPackage);
        return theModelPackage;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getEntity() {
        return entityEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getNamedEntity() {
        return namedEntityEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getNamedEntity_Name() {
        return (EAttribute) namedEntityEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getSpecification() {
        return specificationEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDomainEntity() {
        return domainEntityEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getContentFilteredTopic() {
        return contentFilteredTopicEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getContentFilteredTopic_Filter_expression() {
        return (EAttribute) contentFilteredTopicEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getMultiTopic() {
        return multiTopicEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getMultiTopic_Subscription_expression() {
        return (EAttribute) multiTopicEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTopic() {
        return topicEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Durability_service() {
        return (EReference) topicEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Transport_priority() {
        return (EReference) topicEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Topic_data() {
        return (EReference) topicEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Resource_limits() {
        return (EReference) topicEClass.getEStructuralFeatures().get(3);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Reliability() {
        return (EReference) topicEClass.getEStructuralFeatures().get(4);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Ownership() {
        return (EReference) topicEClass.getEStructuralFeatures().get(5);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Liveliness() {
        return (EReference) topicEClass.getEStructuralFeatures().get(6);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_History() {
        return (EReference) topicEClass.getEStructuralFeatures().get(7);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Durability() {
        return (EReference) topicEClass.getEStructuralFeatures().get(8);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Destination_order() {
        return (EReference) topicEClass.getEStructuralFeatures().get(9);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Deadline() {
        return (EReference) topicEClass.getEStructuralFeatures().get(10);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopic_Latency_budget() {
        return (EReference) topicEClass.getEStructuralFeatures().get(11);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTopicDescription() {
        return topicDescriptionEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopicDescription_Type() {
        return (EReference) topicDescriptionEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getArray() {
        return arrayEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getArray_Length() {
        return (EAttribute) arrayEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOBoolean() {
        return oBooleanEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getCase() {
        return caseEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getCase_Labels() {
        return (EAttribute) caseEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getCase_Type() {
        return (EReference) caseEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOChar() {
        return oCharEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getCollection() {
        return collectionEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getCollection_Type() {
        return (EReference) collectionEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getConstructedTopicType() {
        return constructedTopicTypeEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getODouble() {
        return oDoubleEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getEnum() {
        return enumEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getEnum_Labels() {
        return (EAttribute) enumEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOFloat() {
        return oFloatEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getKey() {
        return keyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getKey_Member() {
        return (EReference) keyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getKeyField() {
        return keyFieldEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOLong() {
        return oLongEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOLongLong() {
        return oLongLongEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOOctet() {
        return oOctetEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getSequence() {
        return sequenceEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOShort() {
        return oShortEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getSimple() {
        return simpleEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOString() {
        return oStringEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTopicStruct() {
        return topicStructEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopicStruct_Members() {
        return (EReference) topicStructEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTopicStruct_Keys() {
        return (EReference) topicStructEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTopicField() {
        return topicFieldEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTypedef() {
        return typedefEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTypedef_Type() {
        return (EReference) typedefEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOULong() {
        return ouLongEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOULongLong() {
        return ouLongLongEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getUnion() {
        return unionEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getUnion_Switch() {
        return (EReference) unionEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getUnion_Cases() {
        return (EReference) unionEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOUShort() {
        return ouShortEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDataReader() {
        return dataReaderEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReader_Topic() {
        return (EReference) dataReaderEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReader_Reader_data_lifecycle() {
        return (EReference) dataReaderEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReader_Transport_priority() {
        return (EReference) dataReaderEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReader_Durability_service() {
        return (EReference) dataReaderEClass.getEStructuralFeatures().get(3);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReader_Ownership_strength() {
        return (EReference) dataReaderEClass.getEStructuralFeatures().get(4);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDataReaderWriter() {
        return dataReaderWriterEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_Durability() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_Destination_order() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_Deadline() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_History() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(3);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_User_data() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(4);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_Resource_limits() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(5);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_Ownership() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(6);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_Liveliness() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(7);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_Latency_budget() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(8);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataReaderWriter_Reliability() {
        return (EReference) dataReaderWriterEClass.getEStructuralFeatures().get(9);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDataWriter() {
        return dataWriterEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataWriter_Topic() {
        return (EReference) dataWriterEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDataWriter_Writer_data_lifecycle() {
        return (EReference) dataWriterEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDomain() {
        return domainEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDomainParticipant() {
        return domainParticipantEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDomainParticipant_Subscribers() {
        return (EReference) domainParticipantEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDomainParticipant_Publishers() {
        return (EReference) domainParticipantEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDomainParticipant_Entity_factory() {
        return (EReference) domainParticipantEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDomainParticipant_User_data() {
        return (EReference) domainParticipantEClass.getEStructuralFeatures().get(3);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDomainParticipant_Domain() {
        return (EReference) domainParticipantEClass.getEStructuralFeatures().get(4);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getPublisher() {
        return publisherEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getPublisher_Writers() {
        return (EReference) publisherEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getPublisherSubscriber() {
        return publisherSubscriberEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getPublisherSubscriber_Entity_factory() {
        return (EReference) publisherSubscriberEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getPublisherSubscriber_Presentation() {
        return (EReference) publisherSubscriberEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getPublisherSubscriber_Group_data() {
        return (EReference) publisherSubscriberEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getPublisherSubscriber_Partition() {
        return (EReference) publisherSubscriberEClass.getEStructuralFeatures().get(3);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getPublisherSubscriber_Transport() {
        return (EReference) publisherSubscriberEClass.getEStructuralFeatures().get(4);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getSubscriber() {
        return subscriberEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getSubscriber_Readers() {
        return (EReference) subscriberEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDeadlineQosPolicy() {
        return deadlineQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDeadlineQosPolicy_Period() {
        return (EReference) deadlineQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDestinationOrderQosPolicy() {
        return destinationOrderQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getDestinationOrderQosPolicy_Kind() {
        return (EAttribute) destinationOrderQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDurabilityQosPolicy() {
        return durabilityQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getDurabilityQosPolicy_Kind() {
        return (EAttribute) durabilityQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getDurabilityServiceQosPolicy() {
        return durabilityServiceQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getDurabilityServiceQosPolicy_History_depth() {
        return (EAttribute) durabilityServiceQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getDurabilityServiceQosPolicy_History_kind() {
        return (EAttribute) durabilityServiceQosPolicyEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getDurabilityServiceQosPolicy_Max_instances() {
        return (EAttribute) durabilityServiceQosPolicyEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getDurabilityServiceQosPolicy_Max_samples() {
        return (EAttribute) durabilityServiceQosPolicyEClass.getEStructuralFeatures().get(3);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getDurabilityServiceQosPolicy_Max_samples_per_instance() {
        return (EAttribute) durabilityServiceQosPolicyEClass.getEStructuralFeatures().get(4);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getDurabilityServiceQosPolicy_Service_cleanup_delay() {
        return (EReference) durabilityServiceQosPolicyEClass.getEStructuralFeatures().get(5);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getEntityFactoryQosPolicy() {
        return entityFactoryQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getEntityFactoryQosPolicy_Autoenable_created_entities() {
        return (EAttribute) entityFactoryQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getGroupDataQosPolicy() {
        return groupDataQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getGroupDataQosPolicy_Value() {
        return (EAttribute) groupDataQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getHistoryQosPolicy() {
        return historyQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getHistoryQosPolicy_Depth() {
        return (EAttribute) historyQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getHistoryQosPolicy_Kind() {
        return (EAttribute) historyQosPolicyEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getLatencyBudgetQosPolicy() {
        return latencyBudgetQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getLatencyBudgetQosPolicy_Duration() {
        return (EReference) latencyBudgetQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getLifespanQosPolicy() {
        return lifespanQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getLifespanQosPolicy_Duration() {
        return (EReference) lifespanQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getLivelinessQosPolicy() {
        return livelinessQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getLivelinessQosPolicy_Kind() {
        return (EAttribute) livelinessQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getLivelinessQosPolicy_Lease_duration() {
        return (EReference) livelinessQosPolicyEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOwnershipQosPolicy() {
        return ownershipQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getOwnershipQosPolicy_Kind() {
        return (EAttribute) ownershipQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getOwnershipStrengthQosPolicy() {
        return ownershipStrengthQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getOwnershipStrengthQosPolicy_Value() {
        return (EAttribute) ownershipStrengthQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getPartitionQosPolicy() {
        return partitionQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getPartitionQosPolicy_Name() {
        return (EAttribute) partitionQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getPresentationQosPolicy() {
        return presentationQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getPresentationQosPolicy_Access_scope() {
        return (EAttribute) presentationQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getPresentationQosPolicy_Coherent_access() {
        return (EAttribute) presentationQosPolicyEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getPresentationQosPolicy_Ordered_access() {
        return (EAttribute) presentationQosPolicyEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getQosPolicy() {
        return qosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getReaderDataLifecycleQosPolicy() {
        return readerDataLifecycleQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getReaderDataLifecycleQosPolicy_Autopurge_nowriter_samples_delay() {
        return (EReference) readerDataLifecycleQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getReliabilityQosPolicy() {
        return reliabilityQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getReliabilityQosPolicy_Kind() {
        return (EAttribute) reliabilityQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getReliabilityQosPolicy_Max_blocking_time() {
        return (EReference) reliabilityQosPolicyEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getResourceLimitsQosPolicy() {
        return resourceLimitsQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getResourceLimitsQosPolicy_Max_instances() {
        return (EAttribute) resourceLimitsQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getResourceLimitsQosPolicy_Max_samples() {
        return (EAttribute) resourceLimitsQosPolicyEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getResourceLimitsQosPolicy_Max_samples_per_instance() {
        return (EAttribute) resourceLimitsQosPolicyEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTimeBasedFilterQosPolicy() {
        return timeBasedFilterQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getTimeBasedFilterQosPolicy_Minimum_separation() {
        return (EReference) timeBasedFilterQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTopicDataQosPolicy() {
        return topicDataQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getTopicDataQosPolicy_Value() {
        return (EAttribute) topicDataQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTransportPriorityQosPolicy() {
        return transportPriorityQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getTransportPriorityQosPolicy_Value() {
        return (EAttribute) transportPriorityQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getUserDataQosPolicy() {
        return userDataQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getUserDataQosPolicy_Value() {
        return (EAttribute) userDataQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getPeriod() {
        return periodEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getPeriod_Seconds() {
        return (EAttribute) periodEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getPeriod_Nanoseconds() {
        return (EAttribute) periodEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getWriterDataLifecycleQosPolicy() {
        return writerDataLifecycleQosPolicyEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getWriterDataLifecycleQosPolicy_Autodispose_unregistered_instances() {
        return (EAttribute) writerDataLifecycleQosPolicyEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getApplicationTarget() {
        return applicationTargetEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getApplicationTarget_Component_type() {
        return (EAttribute) applicationTargetEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getApplicationTarget_Language() {
        return (EAttribute) applicationTargetEClass.getEStructuralFeatures().get(1);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getApplicationTarget_Platform() {
        return (EAttribute) applicationTargetEClass.getEStructuralFeatures().get(2);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EReference getApplicationTarget_Participants() {
        return (EReference) applicationTargetEClass.getEStructuralFeatures().get(3);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getApplicationTarget_Service_arguments() {
        return (EAttribute) applicationTargetEClass.getEStructuralFeatures().get(4);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EClass getTransport() {
        return transportEClass;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EAttribute getTransport_Transport_id() {
        return (EAttribute) transportEClass.getEStructuralFeatures().get(0);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getDestinationOrderQosPolicyKind() {
        return destinationOrderQosPolicyKindEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getDurabilityQosPolicyKind() {
        return durabilityQosPolicyKindEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getHistoryQosPolicyKind() {
        return historyQosPolicyKindEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getLivelinessQosPolicyKind() {
        return livelinessQosPolicyKindEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getOwnershipQosPolicyKind() {
        return ownershipQosPolicyKindEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getPresentationQosPolicyAccessScopeKind() {
        return presentationQosPolicyAccessScopeKindEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getReliabilityQosPolicyKind() {
        return reliabilityQosPolicyKindEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getComponentType() {
        return componentTypeEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getLanguageType() {
        return languageTypeEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public EEnum getPlatformType() {
        return platformTypeEEnum;
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public ModelFactory getModelFactory() {
        return (ModelFactory) getEFactoryInstance();
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private boolean isCreated = false;

    /**
     * Creates the meta-model objects for the package.  This method is
     * guarded to have no affect on any invocation but its first.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void createPackageContents() {
        if (isCreated) {
            return;
        }
        isCreated = true;

        // Create classes and their features
        entityEClass = createEClass(ENTITY);

        namedEntityEClass = createEClass(NAMED_ENTITY);
        createEAttribute(namedEntityEClass, NAMED_ENTITY__NAME);

        specificationEClass = createEClass(SPECIFICATION);

        domainEntityEClass = createEClass(DOMAIN_ENTITY);

        contentFilteredTopicEClass = createEClass(CONTENT_FILTERED_TOPIC);
        createEAttribute(contentFilteredTopicEClass, CONTENT_FILTERED_TOPIC__FILTER_EXPRESSION);

        multiTopicEClass = createEClass(MULTI_TOPIC);
        createEAttribute(multiTopicEClass, MULTI_TOPIC__SUBSCRIPTION_EXPRESSION);

        topicEClass = createEClass(TOPIC);
        createEReference(topicEClass, TOPIC__DURABILITY_SERVICE);
        createEReference(topicEClass, TOPIC__TRANSPORT_PRIORITY);
        createEReference(topicEClass, TOPIC__TOPIC_DATA);
        createEReference(topicEClass, TOPIC__RESOURCE_LIMITS);
        createEReference(topicEClass, TOPIC__RELIABILITY);
        createEReference(topicEClass, TOPIC__OWNERSHIP);
        createEReference(topicEClass, TOPIC__LIVELINESS);
        createEReference(topicEClass, TOPIC__HISTORY);
        createEReference(topicEClass, TOPIC__DURABILITY);
        createEReference(topicEClass, TOPIC__DESTINATION_ORDER);
        createEReference(topicEClass, TOPIC__DEADLINE);
        createEReference(topicEClass, TOPIC__LATENCY_BUDGET);

        topicDescriptionEClass = createEClass(TOPIC_DESCRIPTION);
        createEReference(topicDescriptionEClass, TOPIC_DESCRIPTION__TYPE);

        arrayEClass = createEClass(ARRAY);
        createEAttribute(arrayEClass, ARRAY__LENGTH);

        oBooleanEClass = createEClass(OBOOLEAN);

        caseEClass = createEClass(CASE);
        createEAttribute(caseEClass, CASE__LABELS);
        createEReference(caseEClass, CASE__TYPE);

        oCharEClass = createEClass(OCHAR);

        collectionEClass = createEClass(COLLECTION);
        createEReference(collectionEClass, COLLECTION__TYPE);

        constructedTopicTypeEClass = createEClass(CONSTRUCTED_TOPIC_TYPE);

        oDoubleEClass = createEClass(ODOUBLE);

        enumEClass = createEClass(ENUM);
        createEAttribute(enumEClass, ENUM__LABELS);

        oFloatEClass = createEClass(OFLOAT);

        keyEClass = createEClass(KEY);
        createEReference(keyEClass, KEY__MEMBER);

        keyFieldEClass = createEClass(KEY_FIELD);

        oLongEClass = createEClass(OLONG);

        oLongLongEClass = createEClass(OLONG_LONG);

        oOctetEClass = createEClass(OOCTET);

        sequenceEClass = createEClass(SEQUENCE);

        oShortEClass = createEClass(OSHORT);

        simpleEClass = createEClass(SIMPLE);

        oStringEClass = createEClass(OSTRING);

        topicStructEClass = createEClass(TOPIC_STRUCT);
        createEReference(topicStructEClass, TOPIC_STRUCT__MEMBERS);
        createEReference(topicStructEClass, TOPIC_STRUCT__KEYS);

        topicFieldEClass = createEClass(TOPIC_FIELD);

        typedefEClass = createEClass(TYPEDEF);
        createEReference(typedefEClass, TYPEDEF__TYPE);

        ouLongEClass = createEClass(OU_LONG);

        ouLongLongEClass = createEClass(OU_LONG_LONG);

        unionEClass = createEClass(UNION);
        createEReference(unionEClass, UNION__SWITCH);
        createEReference(unionEClass, UNION__CASES);

        ouShortEClass = createEClass(OU_SHORT);

        dataReaderEClass = createEClass(DATA_READER);
        createEReference(dataReaderEClass, DATA_READER__TOPIC);
        createEReference(dataReaderEClass, DATA_READER__READER_DATA_LIFECYCLE);
        createEReference(dataReaderEClass, DATA_READER__TRANSPORT_PRIORITY);
        createEReference(dataReaderEClass, DATA_READER__DURABILITY_SERVICE);
        createEReference(dataReaderEClass, DATA_READER__OWNERSHIP_STRENGTH);

        dataReaderWriterEClass = createEClass(DATA_READER_WRITER);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__DURABILITY);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__DESTINATION_ORDER);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__DEADLINE);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__HISTORY);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__USER_DATA);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__RESOURCE_LIMITS);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__OWNERSHIP);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__LIVELINESS);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__LATENCY_BUDGET);
        createEReference(dataReaderWriterEClass, DATA_READER_WRITER__RELIABILITY);

        dataWriterEClass = createEClass(DATA_WRITER);
        createEReference(dataWriterEClass, DATA_WRITER__TOPIC);
        createEReference(dataWriterEClass, DATA_WRITER__WRITER_DATA_LIFECYCLE);

        domainEClass = createEClass(DOMAIN);

        domainParticipantEClass = createEClass(DOMAIN_PARTICIPANT);
        createEReference(domainParticipantEClass, DOMAIN_PARTICIPANT__SUBSCRIBERS);
        createEReference(domainParticipantEClass, DOMAIN_PARTICIPANT__PUBLISHERS);
        createEReference(domainParticipantEClass, DOMAIN_PARTICIPANT__ENTITY_FACTORY);
        createEReference(domainParticipantEClass, DOMAIN_PARTICIPANT__USER_DATA);
        createEReference(domainParticipantEClass, DOMAIN_PARTICIPANT__DOMAIN);

        publisherEClass = createEClass(PUBLISHER);
        createEReference(publisherEClass, PUBLISHER__WRITERS);

        publisherSubscriberEClass = createEClass(PUBLISHER_SUBSCRIBER);
        createEReference(publisherSubscriberEClass, PUBLISHER_SUBSCRIBER__ENTITY_FACTORY);
        createEReference(publisherSubscriberEClass, PUBLISHER_SUBSCRIBER__PRESENTATION);
        createEReference(publisherSubscriberEClass, PUBLISHER_SUBSCRIBER__GROUP_DATA);
        createEReference(publisherSubscriberEClass, PUBLISHER_SUBSCRIBER__PARTITION);
        createEReference(publisherSubscriberEClass, PUBLISHER_SUBSCRIBER__TRANSPORT);

        subscriberEClass = createEClass(SUBSCRIBER);
        createEReference(subscriberEClass, SUBSCRIBER__READERS);

        deadlineQosPolicyEClass = createEClass(DEADLINE_QOS_POLICY);
        createEReference(deadlineQosPolicyEClass, DEADLINE_QOS_POLICY__PERIOD);

        destinationOrderQosPolicyEClass = createEClass(DESTINATION_ORDER_QOS_POLICY);
        createEAttribute(destinationOrderQosPolicyEClass, DESTINATION_ORDER_QOS_POLICY__KIND);

        durabilityQosPolicyEClass = createEClass(DURABILITY_QOS_POLICY);
        createEAttribute(durabilityQosPolicyEClass, DURABILITY_QOS_POLICY__KIND);

        durabilityServiceQosPolicyEClass = createEClass(DURABILITY_SERVICE_QOS_POLICY);
        createEAttribute(durabilityServiceQosPolicyEClass, DURABILITY_SERVICE_QOS_POLICY__HISTORY_DEPTH);
        createEAttribute(durabilityServiceQosPolicyEClass, DURABILITY_SERVICE_QOS_POLICY__HISTORY_KIND);
        createEAttribute(durabilityServiceQosPolicyEClass, DURABILITY_SERVICE_QOS_POLICY__MAX_INSTANCES);
        createEAttribute(durabilityServiceQosPolicyEClass, DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES);
        createEAttribute(durabilityServiceQosPolicyEClass, DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE);
        createEReference(durabilityServiceQosPolicyEClass, DURABILITY_SERVICE_QOS_POLICY__SERVICE_CLEANUP_DELAY);

        entityFactoryQosPolicyEClass = createEClass(ENTITY_FACTORY_QOS_POLICY);
        createEAttribute(entityFactoryQosPolicyEClass, ENTITY_FACTORY_QOS_POLICY__AUTOENABLE_CREATED_ENTITIES);

        groupDataQosPolicyEClass = createEClass(GROUP_DATA_QOS_POLICY);
        createEAttribute(groupDataQosPolicyEClass, GROUP_DATA_QOS_POLICY__VALUE);

        historyQosPolicyEClass = createEClass(HISTORY_QOS_POLICY);
        createEAttribute(historyQosPolicyEClass, HISTORY_QOS_POLICY__DEPTH);
        createEAttribute(historyQosPolicyEClass, HISTORY_QOS_POLICY__KIND);

        latencyBudgetQosPolicyEClass = createEClass(LATENCY_BUDGET_QOS_POLICY);
        createEReference(latencyBudgetQosPolicyEClass, LATENCY_BUDGET_QOS_POLICY__DURATION);

        lifespanQosPolicyEClass = createEClass(LIFESPAN_QOS_POLICY);
        createEReference(lifespanQosPolicyEClass, LIFESPAN_QOS_POLICY__DURATION);

        livelinessQosPolicyEClass = createEClass(LIVELINESS_QOS_POLICY);
        createEAttribute(livelinessQosPolicyEClass, LIVELINESS_QOS_POLICY__KIND);
        createEReference(livelinessQosPolicyEClass, LIVELINESS_QOS_POLICY__LEASE_DURATION);

        ownershipQosPolicyEClass = createEClass(OWNERSHIP_QOS_POLICY);
        createEAttribute(ownershipQosPolicyEClass, OWNERSHIP_QOS_POLICY__KIND);

        ownershipStrengthQosPolicyEClass = createEClass(OWNERSHIP_STRENGTH_QOS_POLICY);
        createEAttribute(ownershipStrengthQosPolicyEClass, OWNERSHIP_STRENGTH_QOS_POLICY__VALUE);

        partitionQosPolicyEClass = createEClass(PARTITION_QOS_POLICY);
        createEAttribute(partitionQosPolicyEClass, PARTITION_QOS_POLICY__NAME);

        presentationQosPolicyEClass = createEClass(PRESENTATION_QOS_POLICY);
        createEAttribute(presentationQosPolicyEClass, PRESENTATION_QOS_POLICY__ACCESS_SCOPE);
        createEAttribute(presentationQosPolicyEClass, PRESENTATION_QOS_POLICY__COHERENT_ACCESS);
        createEAttribute(presentationQosPolicyEClass, PRESENTATION_QOS_POLICY__ORDERED_ACCESS);

        qosPolicyEClass = createEClass(QOS_POLICY);

        readerDataLifecycleQosPolicyEClass = createEClass(READER_DATA_LIFECYCLE_QOS_POLICY);
        createEReference(readerDataLifecycleQosPolicyEClass,
                READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY);

        reliabilityQosPolicyEClass = createEClass(RELIABILITY_QOS_POLICY);
        createEAttribute(reliabilityQosPolicyEClass, RELIABILITY_QOS_POLICY__KIND);
        createEReference(reliabilityQosPolicyEClass, RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME);

        resourceLimitsQosPolicyEClass = createEClass(RESOURCE_LIMITS_QOS_POLICY);
        createEAttribute(resourceLimitsQosPolicyEClass, RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES);
        createEAttribute(resourceLimitsQosPolicyEClass, RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES);
        createEAttribute(resourceLimitsQosPolicyEClass, RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE);

        timeBasedFilterQosPolicyEClass = createEClass(TIME_BASED_FILTER_QOS_POLICY);
        createEReference(timeBasedFilterQosPolicyEClass, TIME_BASED_FILTER_QOS_POLICY__MINIMUM_SEPARATION);

        topicDataQosPolicyEClass = createEClass(TOPIC_DATA_QOS_POLICY);
        createEAttribute(topicDataQosPolicyEClass, TOPIC_DATA_QOS_POLICY__VALUE);

        transportPriorityQosPolicyEClass = createEClass(TRANSPORT_PRIORITY_QOS_POLICY);
        createEAttribute(transportPriorityQosPolicyEClass, TRANSPORT_PRIORITY_QOS_POLICY__VALUE);

        userDataQosPolicyEClass = createEClass(USER_DATA_QOS_POLICY);
        createEAttribute(userDataQosPolicyEClass, USER_DATA_QOS_POLICY__VALUE);

        periodEClass = createEClass(PERIOD);
        createEAttribute(periodEClass, PERIOD__SECONDS);
        createEAttribute(periodEClass, PERIOD__NANOSECONDS);

        writerDataLifecycleQosPolicyEClass = createEClass(WRITER_DATA_LIFECYCLE_QOS_POLICY);
        createEAttribute(writerDataLifecycleQosPolicyEClass,
                WRITER_DATA_LIFECYCLE_QOS_POLICY__AUTODISPOSE_UNREGISTERED_INSTANCES);

        applicationTargetEClass = createEClass(APPLICATION_TARGET);
        createEAttribute(applicationTargetEClass, APPLICATION_TARGET__COMPONENT_TYPE);
        createEAttribute(applicationTargetEClass, APPLICATION_TARGET__LANGUAGE);
        createEAttribute(applicationTargetEClass, APPLICATION_TARGET__PLATFORM);
        createEReference(applicationTargetEClass, APPLICATION_TARGET__PARTICIPANTS);
        createEAttribute(applicationTargetEClass, APPLICATION_TARGET__SERVICE_ARGUMENTS);

        transportEClass = createEClass(TRANSPORT);
        createEAttribute(transportEClass, TRANSPORT__TRANSPORT_ID);

        // Create enums
        destinationOrderQosPolicyKindEEnum = createEEnum(DESTINATION_ORDER_QOS_POLICY_KIND);
        durabilityQosPolicyKindEEnum = createEEnum(DURABILITY_QOS_POLICY_KIND);
        historyQosPolicyKindEEnum = createEEnum(HISTORY_QOS_POLICY_KIND);
        livelinessQosPolicyKindEEnum = createEEnum(LIVELINESS_QOS_POLICY_KIND);
        ownershipQosPolicyKindEEnum = createEEnum(OWNERSHIP_QOS_POLICY_KIND);
        presentationQosPolicyAccessScopeKindEEnum = createEEnum(PRESENTATION_QOS_POLICY_ACCESS_SCOPE_KIND);
        reliabilityQosPolicyKindEEnum = createEEnum(RELIABILITY_QOS_POLICY_KIND);
        componentTypeEEnum = createEEnum(COMPONENT_TYPE);
        languageTypeEEnum = createEEnum(LANGUAGE_TYPE);
        platformTypeEEnum = createEEnum(PLATFORM_TYPE);
    }

    /**
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    private boolean isInitialized = false;

    /**
     * Complete the initialization of the package and its meta-model.  This
     * method is guarded to have no affect on any invocation but its first.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    public void initializePackageContents() {
        if (isInitialized) {
            return;
        }
        isInitialized = true;

        // Initialize package
        setName(eNAME);
        setNsPrefix(eNS_PREFIX);
        setNsURI(eNS_URI);

        // Create type parameters

        // Set bounds for type parameters

        // Add supertypes to classes
        namedEntityEClass.getESuperTypes().add(this.getEntity());
        specificationEClass.getESuperTypes().add(this.getEntity());
        domainEntityEClass.getESuperTypes().add(this.getNamedEntity());
        contentFilteredTopicEClass.getESuperTypes().add(this.getTopicDescription());
        multiTopicEClass.getESuperTypes().add(this.getTopicDescription());
        topicEClass.getESuperTypes().add(this.getDomainEntity());
        topicEClass.getESuperTypes().add(this.getTopicDescription());
        topicDescriptionEClass.getESuperTypes().add(this.getNamedEntity());
        arrayEClass.getESuperTypes().add(this.getCollection());
        oBooleanEClass.getESuperTypes().add(this.getSimple());
        caseEClass.getESuperTypes().add(this.getTopicField());
        oCharEClass.getESuperTypes().add(this.getSimple());
        collectionEClass.getESuperTypes().add(this.getTopicField());
        constructedTopicTypeEClass.getESuperTypes().add(this.getTopicField());
        oDoubleEClass.getESuperTypes().add(this.getSimple());
        enumEClass.getESuperTypes().add(this.getTopicField());
        oFloatEClass.getESuperTypes().add(this.getSimple());
        oLongEClass.getESuperTypes().add(this.getSimple());
        oLongLongEClass.getESuperTypes().add(this.getSimple());
        oOctetEClass.getESuperTypes().add(this.getSimple());
        sequenceEClass.getESuperTypes().add(this.getCollection());
        oShortEClass.getESuperTypes().add(this.getSimple());
        simpleEClass.getESuperTypes().add(this.getTopicField());
        simpleEClass.getESuperTypes().add(this.getKeyField());
        oStringEClass.getESuperTypes().add(this.getSimple());
        topicStructEClass.getESuperTypes().add(this.getConstructedTopicType());
        topicFieldEClass.getESuperTypes().add(this.getNamedEntity());
        typedefEClass.getESuperTypes().add(this.getTopicField());
        unionEClass.getESuperTypes().add(this.getConstructedTopicType());
        dataReaderEClass.getESuperTypes().add(this.getDataReaderWriter());
        dataReaderWriterEClass.getESuperTypes().add(this.getDomainEntity());
        dataWriterEClass.getESuperTypes().add(this.getDataReaderWriter());
        domainEClass.getESuperTypes().add(this.getNamedEntity());
        domainParticipantEClass.getESuperTypes().add(this.getDomainEntity());
        publisherEClass.getESuperTypes().add(this.getPublisherSubscriber());
        publisherSubscriberEClass.getESuperTypes().add(this.getDomainEntity());
        subscriberEClass.getESuperTypes().add(this.getPublisherSubscriber());
        deadlineQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        destinationOrderQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        durabilityQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        durabilityServiceQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        entityFactoryQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        groupDataQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        historyQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        latencyBudgetQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        lifespanQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        livelinessQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        ownershipQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        ownershipStrengthQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        partitionQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        presentationQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        qosPolicyEClass.getESuperTypes().add(this.getSpecification());
        readerDataLifecycleQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        reliabilityQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        resourceLimitsQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        timeBasedFilterQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        topicDataQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        transportPriorityQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        userDataQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        writerDataLifecycleQosPolicyEClass.getESuperTypes().add(this.getQosPolicy());
        applicationTargetEClass.getESuperTypes().add(this.getEntity());

        // Initialize classes and features; add operations and parameters
        initEClass(entityEClass, Entity.class, "Entity", IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(namedEntityEClass, NamedEntity.class, "NamedEntity", IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getNamedEntity_Name(), ecorePackage.getEString(), "name", null, 0, 1, NamedEntity.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(specificationEClass, Specification.class, "Specification", IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);

        initEClass(domainEntityEClass, DomainEntity.class, "DomainEntity", IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);

        initEClass(contentFilteredTopicEClass, ContentFilteredTopic.class, "ContentFilteredTopic", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getContentFilteredTopic_Filter_expression(), ecorePackage.getEString(), "filter_expression",
                null, 0, 1, ContentFilteredTopic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE,
                !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(multiTopicEClass, MultiTopic.class, "MultiTopic", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getMultiTopic_Subscription_expression(), ecorePackage.getEString(), "subscription_expression",
                null, 0, 1, MultiTopic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(topicEClass, Topic.class, "Topic", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEReference(getTopic_Durability_service(), this.getDurabilityServiceQosPolicy(), null, "durability_service",
                null, 0, 1, Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Transport_priority(), this.getTransportPriorityQosPolicy(), null, "transport_priority",
                null, 0, 1, Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Topic_data(), this.getTopicDataQosPolicy(), null, "topic_data", null, 0, 1,
                Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Resource_limits(), this.getResourceLimitsQosPolicy(), null, "resource_limits", null, 0,
                1, Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Reliability(), this.getReliabilityQosPolicy(), null, "reliability", null, 0, 1,
                Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Ownership(), this.getOwnershipQosPolicy(), null, "ownership", null, 0, 1, Topic.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Liveliness(), this.getLivelinessQosPolicy(), null, "liveliness", null, 0, 1,
                Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_History(), this.getHistoryQosPolicy(), null, "history", null, 0, 1, Topic.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Durability(), this.getDurabilityQosPolicy(), null, "durability", null, 0, 1,
                Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Destination_order(), this.getDestinationOrderQosPolicy(), null, "destination_order",
                null, 0, 1, Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Deadline(), this.getDeadlineQosPolicy(), null, "deadline", null, 0, 1, Topic.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopic_Latency_budget(), this.getLatencyBudgetQosPolicy(), null, "latency_budget", null, 0, 1,
                Topic.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(topicDescriptionEClass, TopicDescription.class, "TopicDescription", IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getTopicDescription_Type(), this.getTopicField(), null, "type", null, 1, 1,
                TopicDescription.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(arrayEClass, Array.class, "Array", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getArray_Length(), ecorePackage.getELong(), "length", null, 1, 1, Array.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(oBooleanEClass, OBoolean.class, "OBoolean", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(caseEClass, Case.class, "Case", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getCase_Labels(), ecorePackage.getEString(), "labels", null, 0, 1, Case.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getCase_Type(), this.getTopicField(), null, "type", null, 1, 1, Case.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED,
                IS_ORDERED);

        initEClass(oCharEClass, OChar.class, "OChar", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(collectionEClass, Collection.class, "Collection", IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getCollection_Type(), this.getTopicField(), null, "type", null, 1, 1, Collection.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(constructedTopicTypeEClass, ConstructedTopicType.class, "ConstructedTopicType", IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(oDoubleEClass, ODouble.class, "ODouble", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(enumEClass, OpenDDS.Enum.class, "Enum", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getEnum_Labels(), ecorePackage.getEString(), "labels", null, 0, 1, OpenDDS.Enum.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(oFloatEClass, OFloat.class, "OFloat", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(keyEClass, Key.class, "Key", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEReference(getKey_Member(), this.getKeyField(), null, "member", null, 1, 1, Key.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED,
                IS_ORDERED);

        initEClass(keyFieldEClass, KeyField.class, "KeyField", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(oLongEClass, OLong.class, "OLong", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(oLongLongEClass, OLongLong.class, "OLongLong", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);

        initEClass(oOctetEClass, OOctet.class, "OOctet", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(sequenceEClass, Sequence.class, "Sequence", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(oShortEClass, OShort.class, "OShort", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(simpleEClass, Simple.class, "Simple", IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(oStringEClass, OString.class, "OString", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(topicStructEClass, TopicStruct.class, "TopicStruct", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getTopicStruct_Members(), this.getTopicField(), null, "members", null, 1, -1, TopicStruct.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getTopicStruct_Keys(), this.getKey(), null, "keys", null, 0, -1, TopicStruct.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(topicFieldEClass, TopicField.class, "TopicField", IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);

        initEClass(typedefEClass, Typedef.class, "Typedef", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEReference(getTypedef_Type(), this.getTopicField(), null, "type", null, 0, 1, Typedef.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED,
                IS_ORDERED);

        initEClass(ouLongEClass, OULong.class, "OULong", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(ouLongLongEClass, OULongLong.class, "OULongLong", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);

        initEClass(unionEClass, Union.class, "Union", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEReference(getUnion_Switch(), this.getTopicField(), null, "switch", null, 1, 1, Union.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED,
                IS_ORDERED);
        initEReference(getUnion_Cases(), this.getCase(), null, "cases", null, 1, 1, Union.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED,
                IS_ORDERED);

        initEClass(ouShortEClass, OUShort.class, "OUShort", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(dataReaderEClass, DataReader.class, "DataReader", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getDataReader_Topic(), this.getTopicDescription(), null, "topic", null, 1, 1, DataReader.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReader_Reader_data_lifecycle(), this.getReaderDataLifecycleQosPolicy(), null,
                "reader_data_lifecycle", null, 0, 1, DataReader.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReader_Transport_priority(), this.getTransportPriorityQosPolicy(), null,
                "transport_priority", null, 0, 1, DataReader.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReader_Durability_service(), this.getDurabilityServiceQosPolicy(), null,
                "durability_service", null, 0, 1, DataReader.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReader_Ownership_strength(), this.getOwnershipStrengthQosPolicy(), null,
                "ownership_strength", null, 0, 1, DataReader.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(dataReaderWriterEClass, DataReaderWriter.class, "DataReaderWriter", IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getDataReaderWriter_Durability(), this.getDurabilityQosPolicy(), null, "durability", null, 0, 1,
                DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_Destination_order(), this.getDestinationOrderQosPolicy(), null,
                "destination_order", null, 0, 1, DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_Deadline(), this.getDeadlineQosPolicy(), null, "deadline", null, 0, 1,
                DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_History(), this.getHistoryQosPolicy(), null, "history", null, 0, 1,
                DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_User_data(), this.getUserDataQosPolicy(), null, "user_data", null, 0, 1,
                DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_Resource_limits(), this.getResourceLimitsQosPolicy(), null,
                "resource_limits", null, 0, 1, DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_Ownership(), this.getOwnershipQosPolicy(), null, "ownership", null, 0, 1,
                DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_Liveliness(), this.getLivelinessQosPolicy(), null, "liveliness", null, 0, 1,
                DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_Latency_budget(), this.getLatencyBudgetQosPolicy(), null, "latency_budget",
                null, 0, 1, DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE,
                IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataReaderWriter_Reliability(), this.getReliabilityQosPolicy(), null, "reliability", null, 0,
                1, DataReaderWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE,
                IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(dataWriterEClass, DataWriter.class, "DataWriter", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getDataWriter_Topic(), this.getTopic(), null, "topic", null, 1, 1, DataWriter.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDataWriter_Writer_data_lifecycle(), this.getWriterDataLifecycleQosPolicy(), null,
                "writer_data_lifecycle", null, 0, 1, DataWriter.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(domainEClass, Domain.class, "Domain", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);

        initEClass(domainParticipantEClass, DomainParticipant.class, "DomainParticipant", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getDomainParticipant_Subscribers(), this.getSubscriber(), null, "subscribers", null, 0, -1,
                DomainParticipant.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDomainParticipant_Publishers(), this.getPublisher(), null, "publishers", null, 0, -1,
                DomainParticipant.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDomainParticipant_Entity_factory(), this.getEntityFactoryQosPolicy(), null, "entity_factory",
                null, 0, 1, DomainParticipant.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE,
                IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDomainParticipant_User_data(), this.getUserDataQosPolicy(), null, "user_data", null, 0, 1,
                DomainParticipant.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDomainParticipant_Domain(), this.getDomain(), null, "domain", null, 1, 1,
                DomainParticipant.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE, IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(publisherEClass, Publisher.class, "Publisher", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getPublisher_Writers(), this.getDataWriter(), null, "writers", null, 1, -1, Publisher.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(publisherSubscriberEClass, PublisherSubscriber.class, "PublisherSubscriber", IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEReference(getPublisherSubscriber_Entity_factory(), this.getEntityFactoryQosPolicy(), null,
                "entity_factory", null, 0, 1, PublisherSubscriber.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_COMPOSITE, IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getPublisherSubscriber_Presentation(), this.getPresentationQosPolicy(), null, "presentation",
                null, 0, 1, PublisherSubscriber.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE,
                IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getPublisherSubscriber_Group_data(), this.getGroupDataQosPolicy(), null, "group_data", null, 0,
                1, PublisherSubscriber.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE,
                IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getPublisherSubscriber_Partition(), this.getPartitionQosPolicy(), null, "partition", null, 0, 1,
                PublisherSubscriber.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE,
                IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getPublisherSubscriber_Transport(), this.getTransport(), null, "transport", null, 1, 1,
                PublisherSubscriber.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE,
                IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(subscriberEClass, Subscriber.class, "Subscriber", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getSubscriber_Readers(), this.getDataReader(), null, "readers", null, 1, -1, Subscriber.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES, !IS_UNSETTABLE,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(deadlineQosPolicyEClass, DeadlineQosPolicy.class, "DeadlineQosPolicy", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getDeadlineQosPolicy_Period(), this.getPeriod(), null, "period", null, 0, 1,
                DeadlineQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(destinationOrderQosPolicyEClass, DestinationOrderQosPolicy.class, "DestinationOrderQosPolicy",
                !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getDestinationOrderQosPolicy_Kind(), this.getDestinationOrderQosPolicyKind(), "kind", null, 0,
                1, DestinationOrderQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(durabilityQosPolicyEClass, DurabilityQosPolicy.class, "DurabilityQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getDurabilityQosPolicy_Kind(), this.getDurabilityQosPolicyKind(), "kind", null, 0, 1,
                DurabilityQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(durabilityServiceQosPolicyEClass, DurabilityServiceQosPolicy.class, "DurabilityServiceQosPolicy",
                !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getDurabilityServiceQosPolicy_History_depth(), ecorePackage.getELong(), "history_depth", null,
                0, 1, DurabilityServiceQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE,
                !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getDurabilityServiceQosPolicy_History_kind(), this.getHistoryQosPolicyKind(), "history_kind",
                null, 0, 1, DurabilityServiceQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getDurabilityServiceQosPolicy_Max_instances(), ecorePackage.getELong(), "max_instances", null,
                0, 1, DurabilityServiceQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE,
                !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getDurabilityServiceQosPolicy_Max_samples(), ecorePackage.getELong(), "max_samples", null, 0, 1,
                DurabilityServiceQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getDurabilityServiceQosPolicy_Max_samples_per_instance(), ecorePackage.getELong(),
                "max_samples_per_instance", null, 0, 1, DurabilityServiceQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE,
                IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getDurabilityServiceQosPolicy_Service_cleanup_delay(), this.getPeriod(), null,
                "service_cleanup_delay", null, 0, 1, DurabilityServiceQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE,
                IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(entityFactoryQosPolicyEClass, EntityFactoryQosPolicy.class, "EntityFactoryQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getEntityFactoryQosPolicy_Autoenable_created_entities(), ecorePackage.getEBoolean(),
                "autoenable_created_entities", null, 0, 1, EntityFactoryQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE,
                IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(groupDataQosPolicyEClass, GroupDataQosPolicy.class, "GroupDataQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getGroupDataQosPolicy_Value(), ecorePackage.getEString(), "value", null, 0, 1,
                GroupDataQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(historyQosPolicyEClass, HistoryQosPolicy.class, "HistoryQosPolicy", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getHistoryQosPolicy_Depth(), ecorePackage.getELong(), "depth", null, 0, 1,
                HistoryQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE,
                !IS_DERIVED, IS_ORDERED);
        initEAttribute(getHistoryQosPolicy_Kind(), this.getHistoryQosPolicyKind(), "kind", null, 0, 1,
                HistoryQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE,
                !IS_DERIVED, IS_ORDERED);

        initEClass(latencyBudgetQosPolicyEClass, LatencyBudgetQosPolicy.class, "LatencyBudgetQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEReference(getLatencyBudgetQosPolicy_Duration(), this.getPeriod(), null, "duration", null, 0, 1,
                LatencyBudgetQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE,
                !IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(lifespanQosPolicyEClass, LifespanQosPolicy.class, "LifespanQosPolicy", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEReference(getLifespanQosPolicy_Duration(), this.getPeriod(), null, "duration", null, 0, 1,
                LifespanQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES,
                !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(livelinessQosPolicyEClass, LivelinessQosPolicy.class, "LivelinessQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getLivelinessQosPolicy_Kind(), this.getLivelinessQosPolicyKind(), "kind", null, 0, 1,
                LivelinessQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getLivelinessQosPolicy_Lease_duration(), this.getPeriod(), null, "lease_duration", null, 0, 1,
                LivelinessQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE,
                !IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(ownershipQosPolicyEClass, OwnershipQosPolicy.class, "OwnershipQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getOwnershipQosPolicy_Kind(), this.getOwnershipQosPolicyKind(), "kind", null, 0, 1,
                OwnershipQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(ownershipStrengthQosPolicyEClass, OwnershipStrengthQosPolicy.class, "OwnershipStrengthQosPolicy",
                !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getOwnershipStrengthQosPolicy_Value(), ecorePackage.getELong(), "value", null, 0, 1,
                OwnershipStrengthQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(partitionQosPolicyEClass, PartitionQosPolicy.class, "PartitionQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getPartitionQosPolicy_Name(), ecorePackage.getEString(), "name", null, 0, 1,
                PartitionQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(presentationQosPolicyEClass, PresentationQosPolicy.class, "PresentationQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getPresentationQosPolicy_Access_scope(), this.getPresentationQosPolicyAccessScopeKind(),
                "access_scope", null, 0, 1, PresentationQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE,
                !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getPresentationQosPolicy_Coherent_access(), ecorePackage.getEBoolean(), "coherent_access", null,
                0, 1, PresentationQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getPresentationQosPolicy_Ordered_access(), ecorePackage.getEBoolean(), "ordered_access", null,
                0, 1, PresentationQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(qosPolicyEClass, QosPolicy.class, "QosPolicy", IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);

        initEClass(readerDataLifecycleQosPolicyEClass, ReaderDataLifecycleQosPolicy.class,
                "ReaderDataLifecycleQosPolicy", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEReference(getReaderDataLifecycleQosPolicy_Autopurge_nowriter_samples_delay(), this.getPeriod(), null,
                "autopurge_nowriter_samples_delay", null, 0, 1, ReaderDataLifecycleQosPolicy.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE, !IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED,
                IS_ORDERED);

        initEClass(reliabilityQosPolicyEClass, ReliabilityQosPolicy.class, "ReliabilityQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getReliabilityQosPolicy_Kind(), this.getReliabilityQosPolicyKind(), "kind", null, 0, 1,
                ReliabilityQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEReference(getReliabilityQosPolicy_Max_blocking_time(), this.getPeriod(), null, "max_blocking_time", null,
                0, 1, ReliabilityQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE,
                !IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(resourceLimitsQosPolicyEClass, ResourceLimitsQosPolicy.class, "ResourceLimitsQosPolicy",
                !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getResourceLimitsQosPolicy_Max_instances(), ecorePackage.getELong(), "max_instances", null, 0,
                1, ResourceLimitsQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getResourceLimitsQosPolicy_Max_samples(), ecorePackage.getELong(), "max_samples", null, 0, 1,
                ResourceLimitsQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getResourceLimitsQosPolicy_Max_samples_per_instance(), ecorePackage.getELong(),
                "max_samples_per_instance", null, 0, 1, ResourceLimitsQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE,
                IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(timeBasedFilterQosPolicyEClass, TimeBasedFilterQosPolicy.class, "TimeBasedFilterQosPolicy",
                !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEReference(getTimeBasedFilterQosPolicy_Minimum_separation(), this.getPeriod(), null, "minimum_separation",
                null, 0, 1, TimeBasedFilterQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, IS_COMPOSITE,
                !IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(topicDataQosPolicyEClass, TopicDataQosPolicy.class, "TopicDataQosPolicy", !IS_ABSTRACT,
                !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getTopicDataQosPolicy_Value(), ecorePackage.getEString(), "value", null, 0, 1,
                TopicDataQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(transportPriorityQosPolicyEClass, TransportPriorityQosPolicy.class, "TransportPriorityQosPolicy",
                !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getTransportPriorityQosPolicy_Value(), ecorePackage.getELong(), "value", null, 0, 1,
                TransportPriorityQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(userDataQosPolicyEClass, UserDataQosPolicy.class, "UserDataQosPolicy", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getUserDataQosPolicy_Value(), ecorePackage.getEString(), "value", null, 0, 1,
                UserDataQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE,
                !IS_DERIVED, IS_ORDERED);

        initEClass(periodEClass, Period.class, "Period", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getPeriod_Seconds(), ecorePackage.getELong(), "seconds", "0", 0, 1, Period.class, !IS_TRANSIENT,
                !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getPeriod_Nanoseconds(), ecorePackage.getELong(), "nanoseconds", "0", 0, 1, Period.class,
                !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(writerDataLifecycleQosPolicyEClass, WriterDataLifecycleQosPolicy.class,
                "WriterDataLifecycleQosPolicy", !IS_ABSTRACT, !IS_INTERFACE, IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getWriterDataLifecycleQosPolicy_Autodispose_unregistered_instances(),
                ecorePackage.getEBoolean(), "autodispose_unregistered_instances", null, 0, 1,
                WriterDataLifecycleQosPolicy.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(applicationTargetEClass, ApplicationTarget.class, "ApplicationTarget", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getApplicationTarget_Component_type(), this.getComponentType(), "component_type", null, 1, 1,
                ApplicationTarget.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE,
                !IS_DERIVED, IS_ORDERED);
        initEAttribute(getApplicationTarget_Language(), this.getLanguageType(), "language", null, 1, 1,
                ApplicationTarget.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE,
                !IS_DERIVED, IS_ORDERED);
        initEAttribute(getApplicationTarget_Platform(), this.getPlatformType(), "platform", null, 1, 1,
                ApplicationTarget.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE,
                !IS_DERIVED, IS_ORDERED);
        initEReference(getApplicationTarget_Participants(), this.getDomainParticipant(), null, "participants", null, 1,
                -1, ApplicationTarget.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_COMPOSITE,
                IS_RESOLVE_PROXIES, !IS_UNSETTABLE, IS_UNIQUE, !IS_DERIVED, IS_ORDERED);
        initEAttribute(getApplicationTarget_Service_arguments(), ecorePackage.getEString(), "service_arguments", null,
                0, 1, ApplicationTarget.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID,
                IS_UNIQUE, !IS_DERIVED, IS_ORDERED);

        initEClass(transportEClass, Transport.class, "Transport", !IS_ABSTRACT, !IS_INTERFACE,
                IS_GENERATED_INSTANCE_CLASS);
        initEAttribute(getTransport_Transport_id(), ecorePackage.getELong(), "transport_id", null, 1, 1,
                Transport.class, !IS_TRANSIENT, !IS_VOLATILE, IS_CHANGEABLE, !IS_UNSETTABLE, !IS_ID, IS_UNIQUE,
                !IS_DERIVED, IS_ORDERED);

        // Initialize enums and add enum literals
        initEEnum(destinationOrderQosPolicyKindEEnum, DestinationOrderQosPolicyKind.class,
                "DestinationOrderQosPolicyKind");
        addEEnumLiteral(destinationOrderQosPolicyKindEEnum, DestinationOrderQosPolicyKind.BY_RECEPTION_TIMESTAMP);
        addEEnumLiteral(destinationOrderQosPolicyKindEEnum, DestinationOrderQosPolicyKind.BY_SOURCE_TIMESTAMP);

        initEEnum(durabilityQosPolicyKindEEnum, DurabilityQosPolicyKind.class, "DurabilityQosPolicyKind");
        addEEnumLiteral(durabilityQosPolicyKindEEnum, DurabilityQosPolicyKind.VOLATILE);
        addEEnumLiteral(durabilityQosPolicyKindEEnum, DurabilityQosPolicyKind.TRANSIENT);
        addEEnumLiteral(durabilityQosPolicyKindEEnum, DurabilityQosPolicyKind.TRANSIENT_LOCAL);
        addEEnumLiteral(durabilityQosPolicyKindEEnum, DurabilityQosPolicyKind.PERSISTENT);

        initEEnum(historyQosPolicyKindEEnum, HistoryQosPolicyKind.class, "HistoryQosPolicyKind");
        addEEnumLiteral(historyQosPolicyKindEEnum, HistoryQosPolicyKind.KEEP_LAST);
        addEEnumLiteral(historyQosPolicyKindEEnum, HistoryQosPolicyKind.KEEP_ALL);

        initEEnum(livelinessQosPolicyKindEEnum, LivelinessQosPolicyKind.class, "LivelinessQosPolicyKind");
        addEEnumLiteral(livelinessQosPolicyKindEEnum, LivelinessQosPolicyKind.AUTOMATIC);
        addEEnumLiteral(livelinessQosPolicyKindEEnum, LivelinessQosPolicyKind.MANUAL_BY_PARTICIPANT);
        addEEnumLiteral(livelinessQosPolicyKindEEnum, LivelinessQosPolicyKind.MANUAL_BY_TOPIC);

        initEEnum(ownershipQosPolicyKindEEnum, OwnershipQosPolicyKind.class, "OwnershipQosPolicyKind");
        addEEnumLiteral(ownershipQosPolicyKindEEnum, OwnershipQosPolicyKind.SHARED);
        addEEnumLiteral(ownershipQosPolicyKindEEnum, OwnershipQosPolicyKind.EXCLUSIVE);

        initEEnum(presentationQosPolicyAccessScopeKindEEnum, PresentationQosPolicyAccessScopeKind.class,
                "PresentationQosPolicyAccessScopeKind");
        addEEnumLiteral(presentationQosPolicyAccessScopeKindEEnum, PresentationQosPolicyAccessScopeKind.INSTANCE);
        addEEnumLiteral(presentationQosPolicyAccessScopeKindEEnum, PresentationQosPolicyAccessScopeKind.TOPIC);
        addEEnumLiteral(presentationQosPolicyAccessScopeKindEEnum, PresentationQosPolicyAccessScopeKind.GROUP);

        initEEnum(reliabilityQosPolicyKindEEnum, ReliabilityQosPolicyKind.class, "ReliabilityQosPolicyKind");
        addEEnumLiteral(reliabilityQosPolicyKindEEnum, ReliabilityQosPolicyKind.BEST_EFFORT);
        addEEnumLiteral(reliabilityQosPolicyKindEEnum, ReliabilityQosPolicyKind.RELIABLE);

        initEEnum(componentTypeEEnum, ComponentType.class, "ComponentType");
        addEEnumLiteral(componentTypeEEnum, ComponentType.EXECUTABLE);
        addEEnumLiteral(componentTypeEEnum, ComponentType.SHARED_LIBRARY);
        addEEnumLiteral(componentTypeEEnum, ComponentType.STATIC_LIBRARY);

        initEEnum(languageTypeEEnum, LanguageType.class, "LanguageType");
        addEEnumLiteral(languageTypeEEnum, LanguageType.CXX);

        initEEnum(platformTypeEEnum, PlatformType.class, "PlatformType");
        addEEnumLiteral(platformTypeEEnum, PlatformType.MPC_CDT);
        addEEnumLiteral(platformTypeEEnum, PlatformType.MPC_GNUACE);
        addEEnumLiteral(platformTypeEEnum, PlatformType.MPC_NMAKE);
        addEEnumLiteral(platformTypeEEnum, PlatformType.MPC_VC71);
        addEEnumLiteral(platformTypeEEnum, PlatformType.MPC_VC8);
        addEEnumLiteral(platformTypeEEnum, PlatformType.MPC_VC9);

        // Create resource
        createResource(eNS_URI);
    }

} //ModelPackageImpl
