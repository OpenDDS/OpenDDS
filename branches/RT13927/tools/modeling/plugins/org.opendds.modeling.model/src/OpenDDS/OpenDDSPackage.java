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

/**
 * <!-- begin-user-doc -->
 * The <b>Package</b> for the model.
 * It contains accessors for the meta objects to represent
 * <ul>
 *   <li>each class,</li>
 *   <li>each feature of each class,</li>
 *   <li>each enum,</li>
 *   <li>and each data type</li>
 * </ul>
 * <!-- end-user-doc -->
 * @see OpenDDS.OpenDDSFactory
 * @model kind="package"
 * @generated
 */
public interface OpenDDSPackage extends EPackage {
    /**
     * The package name.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    String eNAME = "OpenDDS";

    /**
     * The package namespace URI.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    String eNS_URI = "http://www.opendds.org/schemas/modeling/OpenDDS";

    /**
     * The package namespace name.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    String eNS_PREFIX = "opendds";

    /**
     * The singleton instance of the package.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     */
    OpenDDSPackage eINSTANCE = OpenDDS.OpenDDSPackageImpl.init();

    /**
     * The meta object id for the '{@link OpenDDS.EntityImpl <em>Entity</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.EntityImpl
     * @see OpenDDS.OpenDDSPackageImpl#getEntity()
     * @generated
     */
    int ENTITY = 0;

    /**
     * The number of structural features of the '<em>Entity</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ENTITY_FEATURE_COUNT = 0;

    /**
     * The meta object id for the '{@link OpenDDS.NamedEntityImpl <em>Named Entity</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.NamedEntityImpl
     * @see OpenDDS.OpenDDSPackageImpl#getNamedEntity()
     * @generated
     */
    int NAMED_ENTITY = 1;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int NAMED_ENTITY__NAME = ENTITY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Named Entity</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int NAMED_ENTITY_FEATURE_COUNT = ENTITY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.SpecificationImpl <em>Specification</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.SpecificationImpl
     * @see OpenDDS.OpenDDSPackageImpl#getSpecification()
     * @generated
     */
    int SPECIFICATION = 2;

    /**
     * The number of structural features of the '<em>Specification</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SPECIFICATION_FEATURE_COUNT = ENTITY_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.DomainEntityImpl <em>Domain Entity</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DomainEntityImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDomainEntity()
     * @generated
     */
    int DOMAIN_ENTITY = 3;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_ENTITY__NAME = NAMED_ENTITY__NAME;

    /**
     * The number of structural features of the '<em>Domain Entity</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_ENTITY_FEATURE_COUNT = NAMED_ENTITY_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.TopicDescriptionImpl <em>Topic Description</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TopicDescriptionImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTopicDescription()
     * @generated
     */
    int TOPIC_DESCRIPTION = 7;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_DESCRIPTION__NAME = NAMED_ENTITY__NAME;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_DESCRIPTION__TYPE = NAMED_ENTITY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Topic Description</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_DESCRIPTION_FEATURE_COUNT = NAMED_ENTITY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.ContentFilteredTopicImpl <em>Content Filtered Topic</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ContentFilteredTopicImpl
     * @see OpenDDS.OpenDDSPackageImpl#getContentFilteredTopic()
     * @generated
     */
    int CONTENT_FILTERED_TOPIC = 4;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CONTENT_FILTERED_TOPIC__NAME = TOPIC_DESCRIPTION__NAME;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CONTENT_FILTERED_TOPIC__TYPE = TOPIC_DESCRIPTION__TYPE;

    /**
     * The feature id for the '<em><b>Filter expression</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CONTENT_FILTERED_TOPIC__FILTER_EXPRESSION = TOPIC_DESCRIPTION_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Content Filtered Topic</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CONTENT_FILTERED_TOPIC_FEATURE_COUNT = TOPIC_DESCRIPTION_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.MultiTopicImpl <em>Multi Topic</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.MultiTopicImpl
     * @see OpenDDS.OpenDDSPackageImpl#getMultiTopic()
     * @generated
     */
    int MULTI_TOPIC = 5;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int MULTI_TOPIC__NAME = TOPIC_DESCRIPTION__NAME;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int MULTI_TOPIC__TYPE = TOPIC_DESCRIPTION__TYPE;

    /**
     * The feature id for the '<em><b>Subscription expression</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int MULTI_TOPIC__SUBSCRIPTION_EXPRESSION = TOPIC_DESCRIPTION_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Multi Topic</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int MULTI_TOPIC_FEATURE_COUNT = TOPIC_DESCRIPTION_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.TopicImpl <em>Topic</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TopicImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTopic()
     * @generated
     */
    int TOPIC = 6;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__NAME = DOMAIN_ENTITY__NAME;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__TYPE = DOMAIN_ENTITY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Durability service</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__DURABILITY_SERVICE = DOMAIN_ENTITY_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Transport priority</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__TRANSPORT_PRIORITY = DOMAIN_ENTITY_FEATURE_COUNT + 2;

    /**
     * The feature id for the '<em><b>Topic data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__TOPIC_DATA = DOMAIN_ENTITY_FEATURE_COUNT + 3;

    /**
     * The feature id for the '<em><b>Resource limits</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__RESOURCE_LIMITS = DOMAIN_ENTITY_FEATURE_COUNT + 4;

    /**
     * The feature id for the '<em><b>Reliability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__RELIABILITY = DOMAIN_ENTITY_FEATURE_COUNT + 5;

    /**
     * The feature id for the '<em><b>Ownership</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__OWNERSHIP = DOMAIN_ENTITY_FEATURE_COUNT + 6;

    /**
     * The feature id for the '<em><b>Liveliness</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__LIVELINESS = DOMAIN_ENTITY_FEATURE_COUNT + 7;

    /**
     * The feature id for the '<em><b>History</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__HISTORY = DOMAIN_ENTITY_FEATURE_COUNT + 8;

    /**
     * The feature id for the '<em><b>Durability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__DURABILITY = DOMAIN_ENTITY_FEATURE_COUNT + 9;

    /**
     * The feature id for the '<em><b>Destination order</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__DESTINATION_ORDER = DOMAIN_ENTITY_FEATURE_COUNT + 10;

    /**
     * The feature id for the '<em><b>Deadline</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__DEADLINE = DOMAIN_ENTITY_FEATURE_COUNT + 11;

    /**
     * The feature id for the '<em><b>Latency budget</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC__LATENCY_BUDGET = DOMAIN_ENTITY_FEATURE_COUNT + 12;

    /**
     * The number of structural features of the '<em>Topic</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_FEATURE_COUNT = DOMAIN_ENTITY_FEATURE_COUNT + 13;

    /**
     * The meta object id for the '{@link OpenDDS.TopicFieldImpl <em>Topic Field</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TopicFieldImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTopicField()
     * @generated
     */
    int TOPIC_FIELD = 27;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_FIELD__NAME = NAMED_ENTITY__NAME;

    /**
     * The number of structural features of the '<em>Topic Field</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_FIELD_FEATURE_COUNT = NAMED_ENTITY_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.CollectionImpl <em>Collection</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.CollectionImpl
     * @see OpenDDS.OpenDDSPackageImpl#getCollection()
     * @generated
     */
    int COLLECTION = 12;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int COLLECTION__NAME = TOPIC_FIELD__NAME;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int COLLECTION__TYPE = TOPIC_FIELD_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Collection</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int COLLECTION_FEATURE_COUNT = TOPIC_FIELD_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.ArrayImpl <em>Array</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ArrayImpl
     * @see OpenDDS.OpenDDSPackageImpl#getArray()
     * @generated
     */
    int ARRAY = 8;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ARRAY__NAME = COLLECTION__NAME;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ARRAY__TYPE = COLLECTION__TYPE;

    /**
     * The feature id for the '<em><b>Length</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ARRAY__LENGTH = COLLECTION_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Array</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ARRAY_FEATURE_COUNT = COLLECTION_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.SimpleImpl <em>Simple</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.SimpleImpl
     * @see OpenDDS.OpenDDSPackageImpl#getSimple()
     * @generated
     */
    int SIMPLE = 24;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SIMPLE__NAME = TOPIC_FIELD__NAME;

    /**
     * The number of structural features of the '<em>Simple</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SIMPLE_FEATURE_COUNT = TOPIC_FIELD_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.OBooleanImpl <em>OBoolean</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OBooleanImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOBoolean()
     * @generated
     */
    int OBOOLEAN = 9;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OBOOLEAN__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>OBoolean</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OBOOLEAN_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.CaseImpl <em>Case</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.CaseImpl
     * @see OpenDDS.OpenDDSPackageImpl#getCase()
     * @generated
     */
    int CASE = 10;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CASE__NAME = TOPIC_FIELD__NAME;

    /**
     * The feature id for the '<em><b>Labels</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CASE__LABELS = TOPIC_FIELD_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CASE__TYPE = TOPIC_FIELD_FEATURE_COUNT + 1;

    /**
     * The number of structural features of the '<em>Case</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CASE_FEATURE_COUNT = TOPIC_FIELD_FEATURE_COUNT + 2;

    /**
     * The meta object id for the '{@link OpenDDS.OCharImpl <em>OChar</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OCharImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOChar()
     * @generated
     */
    int OCHAR = 11;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OCHAR__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>OChar</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OCHAR_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.ConstructedTopicTypeImpl <em>Constructed Topic Type</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ConstructedTopicTypeImpl
     * @see OpenDDS.OpenDDSPackageImpl#getConstructedTopicType()
     * @generated
     */
    int CONSTRUCTED_TOPIC_TYPE = 13;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CONSTRUCTED_TOPIC_TYPE__NAME = TOPIC_FIELD__NAME;

    /**
     * The number of structural features of the '<em>Constructed Topic Type</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int CONSTRUCTED_TOPIC_TYPE_FEATURE_COUNT = TOPIC_FIELD_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.ODoubleImpl <em>ODouble</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ODoubleImpl
     * @see OpenDDS.OpenDDSPackageImpl#getODouble()
     * @generated
     */
    int ODOUBLE = 14;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ODOUBLE__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>ODouble</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ODOUBLE_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.EnumImpl <em>Enum</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.EnumImpl
     * @see OpenDDS.OpenDDSPackageImpl#getEnum()
     * @generated
     */
    int ENUM = 15;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ENUM__NAME = TOPIC_FIELD__NAME;

    /**
     * The feature id for the '<em><b>Labels</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ENUM__LABELS = TOPIC_FIELD_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Enum</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ENUM_FEATURE_COUNT = TOPIC_FIELD_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.OFloatImpl <em>OFloat</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OFloatImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOFloat()
     * @generated
     */
    int OFLOAT = 16;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OFLOAT__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>OFloat</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OFLOAT_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.KeyImpl <em>Key</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.KeyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getKey()
     * @generated
     */
    int KEY = 17;

    /**
     * The feature id for the '<em><b>Member</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int KEY__MEMBER = 0;

    /**
     * The number of structural features of the '<em>Key</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int KEY_FEATURE_COUNT = 1;

    /**
     * The meta object id for the '{@link OpenDDS.KeyFieldImpl <em>Key Field</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.KeyFieldImpl
     * @see OpenDDS.OpenDDSPackageImpl#getKeyField()
     * @generated
     */
    int KEY_FIELD = 18;

    /**
     * The number of structural features of the '<em>Key Field</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int KEY_FIELD_FEATURE_COUNT = 0;

    /**
     * The meta object id for the '{@link OpenDDS.OLongImpl <em>OLong</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OLongImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOLong()
     * @generated
     */
    int OLONG = 19;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OLONG__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>OLong</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OLONG_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.OLongLongImpl <em>OLong Long</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OLongLongImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOLongLong()
     * @generated
     */
    int OLONG_LONG = 20;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OLONG_LONG__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>OLong Long</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OLONG_LONG_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.OOctetImpl <em>OOctet</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OOctetImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOOctet()
     * @generated
     */
    int OOCTET = 21;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OOCTET__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>OOctet</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OOCTET_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.SequenceImpl <em>Sequence</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.SequenceImpl
     * @see OpenDDS.OpenDDSPackageImpl#getSequence()
     * @generated
     */
    int SEQUENCE = 22;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SEQUENCE__NAME = COLLECTION__NAME;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SEQUENCE__TYPE = COLLECTION__TYPE;

    /**
     * The number of structural features of the '<em>Sequence</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SEQUENCE_FEATURE_COUNT = COLLECTION_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.OShortImpl <em>OShort</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OShortImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOShort()
     * @generated
     */
    int OSHORT = 23;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OSHORT__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>OShort</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OSHORT_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.OStringImpl <em>OString</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OStringImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOString()
     * @generated
     */
    int OSTRING = 25;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OSTRING__NAME = SIMPLE__NAME;

    /**
     * The number of structural features of the '<em>OString</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OSTRING_FEATURE_COUNT = SIMPLE_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.TopicStructImpl <em>Topic Struct</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TopicStructImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTopicStruct()
     * @generated
     */
    int TOPIC_STRUCT = 26;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_STRUCT__NAME = CONSTRUCTED_TOPIC_TYPE__NAME;

    /**
     * The feature id for the '<em><b>Members</b></em>' containment reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_STRUCT__MEMBERS = CONSTRUCTED_TOPIC_TYPE_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Keys</b></em>' containment reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_STRUCT__KEYS = CONSTRUCTED_TOPIC_TYPE_FEATURE_COUNT + 1;

    /**
     * The number of structural features of the '<em>Topic Struct</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_STRUCT_FEATURE_COUNT = CONSTRUCTED_TOPIC_TYPE_FEATURE_COUNT + 2;

    /**
     * The meta object id for the '{@link OpenDDS.TypedefImpl <em>Typedef</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TypedefImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTypedef()
     * @generated
     */
    int TYPEDEF = 28;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TYPEDEF__NAME = TOPIC_FIELD__NAME;

    /**
     * The feature id for the '<em><b>Type</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TYPEDEF__TYPE = TOPIC_FIELD_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Typedef</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TYPEDEF_FEATURE_COUNT = TOPIC_FIELD_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.OULongImpl <em>OU Long</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OULongImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOULong()
     * @generated
     */
    int OU_LONG = 29;

    /**
     * The number of structural features of the '<em>OU Long</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OU_LONG_FEATURE_COUNT = 0;

    /**
     * The meta object id for the '{@link OpenDDS.OULongLongImpl <em>OU Long Long</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OULongLongImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOULongLong()
     * @generated
     */
    int OU_LONG_LONG = 30;

    /**
     * The number of structural features of the '<em>OU Long Long</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OU_LONG_LONG_FEATURE_COUNT = 0;

    /**
     * The meta object id for the '{@link OpenDDS.UnionImpl <em>Union</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.UnionImpl
     * @see OpenDDS.OpenDDSPackageImpl#getUnion()
     * @generated
     */
    int UNION = 31;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int UNION__NAME = CONSTRUCTED_TOPIC_TYPE__NAME;

    /**
     * The feature id for the '<em><b>Switch</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int UNION__SWITCH = CONSTRUCTED_TOPIC_TYPE_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Cases</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int UNION__CASES = CONSTRUCTED_TOPIC_TYPE_FEATURE_COUNT + 1;

    /**
     * The number of structural features of the '<em>Union</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int UNION_FEATURE_COUNT = CONSTRUCTED_TOPIC_TYPE_FEATURE_COUNT + 2;

    /**
     * The meta object id for the '{@link OpenDDS.OUShortImpl <em>OU Short</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OUShortImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOUShort()
     * @generated
     */
    int OU_SHORT = 32;

    /**
     * The number of structural features of the '<em>OU Short</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OU_SHORT_FEATURE_COUNT = 0;

    /**
     * The meta object id for the '{@link OpenDDS.DataReaderWriterImpl <em>Data Reader Writer</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DataReaderWriterImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDataReaderWriter()
     * @generated
     */
    int DATA_READER_WRITER = 34;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__NAME = DOMAIN_ENTITY__NAME;

    /**
     * The feature id for the '<em><b>Durability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__DURABILITY = DOMAIN_ENTITY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Destination order</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__DESTINATION_ORDER = DOMAIN_ENTITY_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Deadline</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__DEADLINE = DOMAIN_ENTITY_FEATURE_COUNT + 2;

    /**
     * The feature id for the '<em><b>History</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__HISTORY = DOMAIN_ENTITY_FEATURE_COUNT + 3;

    /**
     * The feature id for the '<em><b>User data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__USER_DATA = DOMAIN_ENTITY_FEATURE_COUNT + 4;

    /**
     * The feature id for the '<em><b>Resource limits</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__RESOURCE_LIMITS = DOMAIN_ENTITY_FEATURE_COUNT + 5;

    /**
     * The feature id for the '<em><b>Ownership</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__OWNERSHIP = DOMAIN_ENTITY_FEATURE_COUNT + 6;

    /**
     * The feature id for the '<em><b>Liveliness</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__LIVELINESS = DOMAIN_ENTITY_FEATURE_COUNT + 7;

    /**
     * The feature id for the '<em><b>Latency budget</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__LATENCY_BUDGET = DOMAIN_ENTITY_FEATURE_COUNT + 8;

    /**
     * The feature id for the '<em><b>Reliability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER__RELIABILITY = DOMAIN_ENTITY_FEATURE_COUNT + 9;

    /**
     * The number of structural features of the '<em>Data Reader Writer</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_WRITER_FEATURE_COUNT = DOMAIN_ENTITY_FEATURE_COUNT + 10;

    /**
     * The meta object id for the '{@link OpenDDS.DataReaderImpl <em>Data Reader</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DataReaderImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDataReader()
     * @generated
     */
    int DATA_READER = 33;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__NAME = DATA_READER_WRITER__NAME;

    /**
     * The feature id for the '<em><b>Durability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__DURABILITY = DATA_READER_WRITER__DURABILITY;

    /**
     * The feature id for the '<em><b>Destination order</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__DESTINATION_ORDER = DATA_READER_WRITER__DESTINATION_ORDER;

    /**
     * The feature id for the '<em><b>Deadline</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__DEADLINE = DATA_READER_WRITER__DEADLINE;

    /**
     * The feature id for the '<em><b>History</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__HISTORY = DATA_READER_WRITER__HISTORY;

    /**
     * The feature id for the '<em><b>User data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__USER_DATA = DATA_READER_WRITER__USER_DATA;

    /**
     * The feature id for the '<em><b>Resource limits</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__RESOURCE_LIMITS = DATA_READER_WRITER__RESOURCE_LIMITS;

    /**
     * The feature id for the '<em><b>Ownership</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__OWNERSHIP = DATA_READER_WRITER__OWNERSHIP;

    /**
     * The feature id for the '<em><b>Liveliness</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__LIVELINESS = DATA_READER_WRITER__LIVELINESS;

    /**
     * The feature id for the '<em><b>Latency budget</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__LATENCY_BUDGET = DATA_READER_WRITER__LATENCY_BUDGET;

    /**
     * The feature id for the '<em><b>Reliability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__RELIABILITY = DATA_READER_WRITER__RELIABILITY;

    /**
     * The feature id for the '<em><b>Topic</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__TOPIC = DATA_READER_WRITER_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Reader data lifecycle</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__READER_DATA_LIFECYCLE = DATA_READER_WRITER_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Transport priority</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__TRANSPORT_PRIORITY = DATA_READER_WRITER_FEATURE_COUNT + 2;

    /**
     * The feature id for the '<em><b>Durability service</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__DURABILITY_SERVICE = DATA_READER_WRITER_FEATURE_COUNT + 3;

    /**
     * The feature id for the '<em><b>Ownership strength</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER__OWNERSHIP_STRENGTH = DATA_READER_WRITER_FEATURE_COUNT + 4;

    /**
     * The number of structural features of the '<em>Data Reader</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_READER_FEATURE_COUNT = DATA_READER_WRITER_FEATURE_COUNT + 5;

    /**
     * The meta object id for the '{@link OpenDDS.DataWriterImpl <em>Data Writer</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DataWriterImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDataWriter()
     * @generated
     */
    int DATA_WRITER = 35;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__NAME = DATA_READER_WRITER__NAME;

    /**
     * The feature id for the '<em><b>Durability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__DURABILITY = DATA_READER_WRITER__DURABILITY;

    /**
     * The feature id for the '<em><b>Destination order</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__DESTINATION_ORDER = DATA_READER_WRITER__DESTINATION_ORDER;

    /**
     * The feature id for the '<em><b>Deadline</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__DEADLINE = DATA_READER_WRITER__DEADLINE;

    /**
     * The feature id for the '<em><b>History</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__HISTORY = DATA_READER_WRITER__HISTORY;

    /**
     * The feature id for the '<em><b>User data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__USER_DATA = DATA_READER_WRITER__USER_DATA;

    /**
     * The feature id for the '<em><b>Resource limits</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__RESOURCE_LIMITS = DATA_READER_WRITER__RESOURCE_LIMITS;

    /**
     * The feature id for the '<em><b>Ownership</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__OWNERSHIP = DATA_READER_WRITER__OWNERSHIP;

    /**
     * The feature id for the '<em><b>Liveliness</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__LIVELINESS = DATA_READER_WRITER__LIVELINESS;

    /**
     * The feature id for the '<em><b>Latency budget</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__LATENCY_BUDGET = DATA_READER_WRITER__LATENCY_BUDGET;

    /**
     * The feature id for the '<em><b>Reliability</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__RELIABILITY = DATA_READER_WRITER__RELIABILITY;

    /**
     * The feature id for the '<em><b>Topic</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__TOPIC = DATA_READER_WRITER_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Writer data lifecycle</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER__WRITER_DATA_LIFECYCLE = DATA_READER_WRITER_FEATURE_COUNT + 1;

    /**
     * The number of structural features of the '<em>Data Writer</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DATA_WRITER_FEATURE_COUNT = DATA_READER_WRITER_FEATURE_COUNT + 2;

    /**
     * The meta object id for the '{@link OpenDDS.DomainImpl <em>Domain</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DomainImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDomain()
     * @generated
     */
    int DOMAIN = 36;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN__NAME = NAMED_ENTITY__NAME;

    /**
     * The number of structural features of the '<em>Domain</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_FEATURE_COUNT = NAMED_ENTITY_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.DomainParticipantImpl <em>Domain Participant</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DomainParticipantImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDomainParticipant()
     * @generated
     */
    int DOMAIN_PARTICIPANT = 37;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_PARTICIPANT__NAME = DOMAIN_ENTITY__NAME;

    /**
     * The feature id for the '<em><b>Subscribers</b></em>' containment reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_PARTICIPANT__SUBSCRIBERS = DOMAIN_ENTITY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Publishers</b></em>' containment reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_PARTICIPANT__PUBLISHERS = DOMAIN_ENTITY_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Entity factory</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_PARTICIPANT__ENTITY_FACTORY = DOMAIN_ENTITY_FEATURE_COUNT + 2;

    /**
     * The feature id for the '<em><b>User data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_PARTICIPANT__USER_DATA = DOMAIN_ENTITY_FEATURE_COUNT + 3;

    /**
     * The feature id for the '<em><b>Domain</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_PARTICIPANT__DOMAIN = DOMAIN_ENTITY_FEATURE_COUNT + 4;

    /**
     * The number of structural features of the '<em>Domain Participant</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DOMAIN_PARTICIPANT_FEATURE_COUNT = DOMAIN_ENTITY_FEATURE_COUNT + 5;

    /**
     * The meta object id for the '{@link OpenDDS.PublisherSubscriberImpl <em>Publisher Subscriber</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.PublisherSubscriberImpl
     * @see OpenDDS.OpenDDSPackageImpl#getPublisherSubscriber()
     * @generated
     */
    int PUBLISHER_SUBSCRIBER = 39;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER_SUBSCRIBER__NAME = DOMAIN_ENTITY__NAME;

    /**
     * The feature id for the '<em><b>Entity factory</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER_SUBSCRIBER__ENTITY_FACTORY = DOMAIN_ENTITY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Presentation</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER_SUBSCRIBER__PRESENTATION = DOMAIN_ENTITY_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Group data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER_SUBSCRIBER__GROUP_DATA = DOMAIN_ENTITY_FEATURE_COUNT + 2;

    /**
     * The feature id for the '<em><b>Partition</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER_SUBSCRIBER__PARTITION = DOMAIN_ENTITY_FEATURE_COUNT + 3;

    /**
     * The feature id for the '<em><b>Transport</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER_SUBSCRIBER__TRANSPORT = DOMAIN_ENTITY_FEATURE_COUNT + 4;

    /**
     * The number of structural features of the '<em>Publisher Subscriber</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER_SUBSCRIBER_FEATURE_COUNT = DOMAIN_ENTITY_FEATURE_COUNT + 5;

    /**
     * The meta object id for the '{@link OpenDDS.PublisherImpl <em>Publisher</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.PublisherImpl
     * @see OpenDDS.OpenDDSPackageImpl#getPublisher()
     * @generated
     */
    int PUBLISHER = 38;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER__NAME = PUBLISHER_SUBSCRIBER__NAME;

    /**
     * The feature id for the '<em><b>Entity factory</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER__ENTITY_FACTORY = PUBLISHER_SUBSCRIBER__ENTITY_FACTORY;

    /**
     * The feature id for the '<em><b>Presentation</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER__PRESENTATION = PUBLISHER_SUBSCRIBER__PRESENTATION;

    /**
     * The feature id for the '<em><b>Group data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER__GROUP_DATA = PUBLISHER_SUBSCRIBER__GROUP_DATA;

    /**
     * The feature id for the '<em><b>Partition</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER__PARTITION = PUBLISHER_SUBSCRIBER__PARTITION;

    /**
     * The feature id for the '<em><b>Transport</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER__TRANSPORT = PUBLISHER_SUBSCRIBER__TRANSPORT;

    /**
     * The feature id for the '<em><b>Writers</b></em>' containment reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER__WRITERS = PUBLISHER_SUBSCRIBER_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Publisher</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PUBLISHER_FEATURE_COUNT = PUBLISHER_SUBSCRIBER_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.SubscriberImpl <em>Subscriber</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.SubscriberImpl
     * @see OpenDDS.OpenDDSPackageImpl#getSubscriber()
     * @generated
     */
    int SUBSCRIBER = 40;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SUBSCRIBER__NAME = PUBLISHER_SUBSCRIBER__NAME;

    /**
     * The feature id for the '<em><b>Entity factory</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SUBSCRIBER__ENTITY_FACTORY = PUBLISHER_SUBSCRIBER__ENTITY_FACTORY;

    /**
     * The feature id for the '<em><b>Presentation</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SUBSCRIBER__PRESENTATION = PUBLISHER_SUBSCRIBER__PRESENTATION;

    /**
     * The feature id for the '<em><b>Group data</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SUBSCRIBER__GROUP_DATA = PUBLISHER_SUBSCRIBER__GROUP_DATA;

    /**
     * The feature id for the '<em><b>Partition</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SUBSCRIBER__PARTITION = PUBLISHER_SUBSCRIBER__PARTITION;

    /**
     * The feature id for the '<em><b>Transport</b></em>' reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SUBSCRIBER__TRANSPORT = PUBLISHER_SUBSCRIBER__TRANSPORT;

    /**
     * The feature id for the '<em><b>Readers</b></em>' containment reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SUBSCRIBER__READERS = PUBLISHER_SUBSCRIBER_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Subscriber</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int SUBSCRIBER_FEATURE_COUNT = PUBLISHER_SUBSCRIBER_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.QosPolicyImpl <em>Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.QosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getQosPolicy()
     * @generated
     */
    int QOS_POLICY = 55;

    /**
     * The number of structural features of the '<em>Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int QOS_POLICY_FEATURE_COUNT = SPECIFICATION_FEATURE_COUNT + 0;

    /**
     * The meta object id for the '{@link OpenDDS.DeadlineQosPolicyImpl <em>Deadline Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DeadlineQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDeadlineQosPolicy()
     * @generated
     */
    int DEADLINE_QOS_POLICY = 41;

    /**
     * The feature id for the '<em><b>Period</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DEADLINE_QOS_POLICY__PERIOD = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Deadline Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DEADLINE_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.DestinationOrderQosPolicyImpl <em>Destination Order Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DestinationOrderQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDestinationOrderQosPolicy()
     * @generated
     */
    int DESTINATION_ORDER_QOS_POLICY = 42;

    /**
     * The feature id for the '<em><b>Kind</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DESTINATION_ORDER_QOS_POLICY__KIND = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Destination Order Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DESTINATION_ORDER_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.DurabilityQosPolicyImpl <em>Durability Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DurabilityQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDurabilityQosPolicy()
     * @generated
     */
    int DURABILITY_QOS_POLICY = 43;

    /**
     * The feature id for the '<em><b>Kind</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_QOS_POLICY__KIND = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Durability Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.DurabilityServiceQosPolicyImpl <em>Durability Service Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DurabilityServiceQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getDurabilityServiceQosPolicy()
     * @generated
     */
    int DURABILITY_SERVICE_QOS_POLICY = 44;

    /**
     * The feature id for the '<em><b>History depth</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_SERVICE_QOS_POLICY__HISTORY_DEPTH = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>History kind</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_SERVICE_QOS_POLICY__HISTORY_KIND = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Max instances</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_SERVICE_QOS_POLICY__MAX_INSTANCES = QOS_POLICY_FEATURE_COUNT + 2;

    /**
     * The feature id for the '<em><b>Max samples</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES = QOS_POLICY_FEATURE_COUNT + 3;

    /**
     * The feature id for the '<em><b>Max samples per instance</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE = QOS_POLICY_FEATURE_COUNT + 4;

    /**
     * The feature id for the '<em><b>Service cleanup delay</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_SERVICE_QOS_POLICY__SERVICE_CLEANUP_DELAY = QOS_POLICY_FEATURE_COUNT + 5;

    /**
     * The number of structural features of the '<em>Durability Service Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int DURABILITY_SERVICE_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 6;

    /**
     * The meta object id for the '{@link OpenDDS.EntityFactoryQosPolicyImpl <em>Entity Factory Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.EntityFactoryQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getEntityFactoryQosPolicy()
     * @generated
     */
    int ENTITY_FACTORY_QOS_POLICY = 45;

    /**
     * The feature id for the '<em><b>Autoenable created entities</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ENTITY_FACTORY_QOS_POLICY__AUTOENABLE_CREATED_ENTITIES = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Entity Factory Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int ENTITY_FACTORY_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.GroupDataQosPolicyImpl <em>Group Data Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.GroupDataQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getGroupDataQosPolicy()
     * @generated
     */
    int GROUP_DATA_QOS_POLICY = 46;

    /**
     * The feature id for the '<em><b>Value</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int GROUP_DATA_QOS_POLICY__VALUE = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Group Data Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int GROUP_DATA_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.HistoryQosPolicyImpl <em>History Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.HistoryQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getHistoryQosPolicy()
     * @generated
     */
    int HISTORY_QOS_POLICY = 47;

    /**
     * The feature id for the '<em><b>Depth</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int HISTORY_QOS_POLICY__DEPTH = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Kind</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int HISTORY_QOS_POLICY__KIND = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The number of structural features of the '<em>History Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int HISTORY_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 2;

    /**
     * The meta object id for the '{@link OpenDDS.LatencyBudgetQosPolicyImpl <em>Latency Budget Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.LatencyBudgetQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getLatencyBudgetQosPolicy()
     * @generated
     */
    int LATENCY_BUDGET_QOS_POLICY = 48;

    /**
     * The feature id for the '<em><b>Duration</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int LATENCY_BUDGET_QOS_POLICY__DURATION = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Latency Budget Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int LATENCY_BUDGET_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.LifespanQosPolicyImpl <em>Lifespan Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.LifespanQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getLifespanQosPolicy()
     * @generated
     */
    int LIFESPAN_QOS_POLICY = 49;

    /**
     * The feature id for the '<em><b>Duration</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int LIFESPAN_QOS_POLICY__DURATION = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Lifespan Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int LIFESPAN_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.LivelinessQosPolicyImpl <em>Liveliness Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.LivelinessQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getLivelinessQosPolicy()
     * @generated
     */
    int LIVELINESS_QOS_POLICY = 50;

    /**
     * The feature id for the '<em><b>Kind</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int LIVELINESS_QOS_POLICY__KIND = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Lease duration</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int LIVELINESS_QOS_POLICY__LEASE_DURATION = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The number of structural features of the '<em>Liveliness Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int LIVELINESS_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 2;

    /**
     * The meta object id for the '{@link OpenDDS.OwnershipQosPolicyImpl <em>Ownership Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OwnershipQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOwnershipQosPolicy()
     * @generated
     */
    int OWNERSHIP_QOS_POLICY = 51;

    /**
     * The feature id for the '<em><b>Kind</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OWNERSHIP_QOS_POLICY__KIND = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Ownership Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OWNERSHIP_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.OwnershipStrengthQosPolicyImpl <em>Ownership Strength Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OwnershipStrengthQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getOwnershipStrengthQosPolicy()
     * @generated
     */
    int OWNERSHIP_STRENGTH_QOS_POLICY = 52;

    /**
     * The feature id for the '<em><b>Value</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OWNERSHIP_STRENGTH_QOS_POLICY__VALUE = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Ownership Strength Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int OWNERSHIP_STRENGTH_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.PartitionQosPolicyImpl <em>Partition Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.PartitionQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getPartitionQosPolicy()
     * @generated
     */
    int PARTITION_QOS_POLICY = 53;

    /**
     * The feature id for the '<em><b>Name</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PARTITION_QOS_POLICY__NAME = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Partition Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PARTITION_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.PresentationQosPolicyImpl <em>Presentation Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.PresentationQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getPresentationQosPolicy()
     * @generated
     */
    int PRESENTATION_QOS_POLICY = 54;

    /**
     * The feature id for the '<em><b>Access scope</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PRESENTATION_QOS_POLICY__ACCESS_SCOPE = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Coherent access</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PRESENTATION_QOS_POLICY__COHERENT_ACCESS = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Ordered access</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PRESENTATION_QOS_POLICY__ORDERED_ACCESS = QOS_POLICY_FEATURE_COUNT + 2;

    /**
     * The number of structural features of the '<em>Presentation Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PRESENTATION_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 3;

    /**
     * The meta object id for the '{@link OpenDDS.ReaderDataLifecycleQosPolicyImpl <em>Reader Data Lifecycle Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ReaderDataLifecycleQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getReaderDataLifecycleQosPolicy()
     * @generated
     */
    int READER_DATA_LIFECYCLE_QOS_POLICY = 56;

    /**
     * The feature id for the '<em><b>Autopurge nowriter samples delay</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Reader Data Lifecycle Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int READER_DATA_LIFECYCLE_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.ReliabilityQosPolicyImpl <em>Reliability Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ReliabilityQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getReliabilityQosPolicy()
     * @generated
     */
    int RELIABILITY_QOS_POLICY = 57;

    /**
     * The feature id for the '<em><b>Kind</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int RELIABILITY_QOS_POLICY__KIND = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Max blocking time</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The number of structural features of the '<em>Reliability Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int RELIABILITY_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 2;

    /**
     * The meta object id for the '{@link OpenDDS.ResourceLimitsQosPolicyImpl <em>Resource Limits Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ResourceLimitsQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getResourceLimitsQosPolicy()
     * @generated
     */
    int RESOURCE_LIMITS_QOS_POLICY = 58;

    /**
     * The feature id for the '<em><b>Max instances</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Max samples</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Max samples per instance</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE = QOS_POLICY_FEATURE_COUNT + 2;

    /**
     * The number of structural features of the '<em>Resource Limits Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int RESOURCE_LIMITS_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 3;

    /**
     * The meta object id for the '{@link OpenDDS.TimeBasedFilterQosPolicyImpl <em>Time Based Filter Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TimeBasedFilterQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTimeBasedFilterQosPolicy()
     * @generated
     */
    int TIME_BASED_FILTER_QOS_POLICY = 59;

    /**
     * The feature id for the '<em><b>Minimum separation</b></em>' containment reference.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TIME_BASED_FILTER_QOS_POLICY__MINIMUM_SEPARATION = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Time Based Filter Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TIME_BASED_FILTER_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.TopicDataQosPolicyImpl <em>Topic Data Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TopicDataQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTopicDataQosPolicy()
     * @generated
     */
    int TOPIC_DATA_QOS_POLICY = 60;

    /**
     * The feature id for the '<em><b>Value</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_DATA_QOS_POLICY__VALUE = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Topic Data Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TOPIC_DATA_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.TransportPriorityQosPolicyImpl <em>Transport Priority Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TransportPriorityQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTransportPriorityQosPolicy()
     * @generated
     */
    int TRANSPORT_PRIORITY_QOS_POLICY = 61;

    /**
     * The feature id for the '<em><b>Value</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TRANSPORT_PRIORITY_QOS_POLICY__VALUE = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Transport Priority Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TRANSPORT_PRIORITY_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.UserDataQosPolicyImpl <em>User Data Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.UserDataQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getUserDataQosPolicy()
     * @generated
     */
    int USER_DATA_QOS_POLICY = 62;

    /**
     * The feature id for the '<em><b>Value</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int USER_DATA_QOS_POLICY__VALUE = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>User Data Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int USER_DATA_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.PeriodImpl <em>Period</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.PeriodImpl
     * @see OpenDDS.OpenDDSPackageImpl#getPeriod()
     * @generated
     */
    int PERIOD = 63;

    /**
     * The feature id for the '<em><b>Seconds</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PERIOD__SECONDS = 0;

    /**
     * The feature id for the '<em><b>Nanoseconds</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PERIOD__NANOSECONDS = 1;

    /**
     * The number of structural features of the '<em>Period</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int PERIOD_FEATURE_COUNT = 2;

    /**
     * The meta object id for the '{@link OpenDDS.WriterDataLifecycleQosPolicyImpl <em>Writer Data Lifecycle Qos Policy</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.WriterDataLifecycleQosPolicyImpl
     * @see OpenDDS.OpenDDSPackageImpl#getWriterDataLifecycleQosPolicy()
     * @generated
     */
    int WRITER_DATA_LIFECYCLE_QOS_POLICY = 64;

    /**
     * The feature id for the '<em><b>Autodispose unregistered instances</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int WRITER_DATA_LIFECYCLE_QOS_POLICY__AUTODISPOSE_UNREGISTERED_INSTANCES = QOS_POLICY_FEATURE_COUNT + 0;

    /**
     * The number of structural features of the '<em>Writer Data Lifecycle Qos Policy</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int WRITER_DATA_LIFECYCLE_QOS_POLICY_FEATURE_COUNT = QOS_POLICY_FEATURE_COUNT + 1;

    /**
     * The meta object id for the '{@link OpenDDS.ApplicationTargetImpl <em>Application Target</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ApplicationTargetImpl
     * @see OpenDDS.OpenDDSPackageImpl#getApplicationTarget()
     * @generated
     */
    int APPLICATION_TARGET = 65;

    /**
     * The feature id for the '<em><b>Component type</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int APPLICATION_TARGET__COMPONENT_TYPE = ENTITY_FEATURE_COUNT + 0;

    /**
     * The feature id for the '<em><b>Language</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int APPLICATION_TARGET__LANGUAGE = ENTITY_FEATURE_COUNT + 1;

    /**
     * The feature id for the '<em><b>Platform</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int APPLICATION_TARGET__PLATFORM = ENTITY_FEATURE_COUNT + 2;

    /**
     * The feature id for the '<em><b>Participants</b></em>' reference list.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int APPLICATION_TARGET__PARTICIPANTS = ENTITY_FEATURE_COUNT + 3;

    /**
     * The feature id for the '<em><b>Service arguments</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int APPLICATION_TARGET__SERVICE_ARGUMENTS = ENTITY_FEATURE_COUNT + 4;

    /**
     * The number of structural features of the '<em>Application Target</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int APPLICATION_TARGET_FEATURE_COUNT = ENTITY_FEATURE_COUNT + 5;

    /**
     * The meta object id for the '{@link OpenDDS.TransportImpl <em>Transport</em>}' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.TransportImpl
     * @see OpenDDS.OpenDDSPackageImpl#getTransport()
     * @generated
     */
    int TRANSPORT = 66;

    /**
     * The feature id for the '<em><b>Transport id</b></em>' attribute.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TRANSPORT__TRANSPORT_ID = 0;

    /**
     * The number of structural features of the '<em>Transport</em>' class.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @generated
     * @ordered
     */
    int TRANSPORT_FEATURE_COUNT = 1;

    /**
     * The meta object id for the '{@link OpenDDS.DestinationOrderQosPolicyKind <em>Destination Order Qos Policy Kind</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DestinationOrderQosPolicyKind
     * @see OpenDDS.OpenDDSPackageImpl#getDestinationOrderQosPolicyKind()
     * @generated
     */
    int DESTINATION_ORDER_QOS_POLICY_KIND = 67;

    /**
     * The meta object id for the '{@link OpenDDS.DurabilityQosPolicyKind <em>Durability Qos Policy Kind</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.DurabilityQosPolicyKind
     * @see OpenDDS.OpenDDSPackageImpl#getDurabilityQosPolicyKind()
     * @generated
     */
    int DURABILITY_QOS_POLICY_KIND = 68;

    /**
     * The meta object id for the '{@link OpenDDS.HistoryQosPolicyKind <em>History Qos Policy Kind</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.HistoryQosPolicyKind
     * @see OpenDDS.OpenDDSPackageImpl#getHistoryQosPolicyKind()
     * @generated
     */
    int HISTORY_QOS_POLICY_KIND = 69;

    /**
     * The meta object id for the '{@link OpenDDS.LivelinessQosPolicyKind <em>Liveliness Qos Policy Kind</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.LivelinessQosPolicyKind
     * @see OpenDDS.OpenDDSPackageImpl#getLivelinessQosPolicyKind()
     * @generated
     */
    int LIVELINESS_QOS_POLICY_KIND = 70;

    /**
     * The meta object id for the '{@link OpenDDS.OwnershipQosPolicyKind <em>Ownership Qos Policy Kind</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.OwnershipQosPolicyKind
     * @see OpenDDS.OpenDDSPackageImpl#getOwnershipQosPolicyKind()
     * @generated
     */
    int OWNERSHIP_QOS_POLICY_KIND = 71;

    /**
     * The meta object id for the '{@link OpenDDS.PresentationQosPolicyAccessScopeKind <em>Presentation Qos Policy Access Scope Kind</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.PresentationQosPolicyAccessScopeKind
     * @see OpenDDS.OpenDDSPackageImpl#getPresentationQosPolicyAccessScopeKind()
     * @generated
     */
    int PRESENTATION_QOS_POLICY_ACCESS_SCOPE_KIND = 72;

    /**
     * The meta object id for the '{@link OpenDDS.ReliabilityQosPolicyKind <em>Reliability Qos Policy Kind</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ReliabilityQosPolicyKind
     * @see OpenDDS.OpenDDSPackageImpl#getReliabilityQosPolicyKind()
     * @generated
     */
    int RELIABILITY_QOS_POLICY_KIND = 73;

    /**
     * The meta object id for the '{@link OpenDDS.ComponentType <em>Component Type</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.ComponentType
     * @see OpenDDS.OpenDDSPackageImpl#getComponentType()
     * @generated
     */
    int COMPONENT_TYPE = 74;

    /**
     * The meta object id for the '{@link OpenDDS.LanguageType <em>Language Type</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.LanguageType
     * @see OpenDDS.OpenDDSPackageImpl#getLanguageType()
     * @generated
     */
    int LANGUAGE_TYPE = 75;

    /**
     * The meta object id for the '{@link OpenDDS.PlatformType <em>Platform Type</em>}' enum.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @see OpenDDS.PlatformType
     * @see OpenDDS.OpenDDSPackageImpl#getPlatformType()
     * @generated
     */
    int PLATFORM_TYPE = 76;

    /**
     * Returns the meta object for class '{@link OpenDDS.Entity <em>Entity</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Entity</em>'.
     * @see OpenDDS.Entity
     * @generated
     */
    EClass getEntity();

    /**
     * Returns the meta object for class '{@link OpenDDS.NamedEntity <em>Named Entity</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Named Entity</em>'.
     * @see OpenDDS.NamedEntity
     * @generated
     */
    EClass getNamedEntity();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.NamedEntity#getName <em>Name</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Name</em>'.
     * @see OpenDDS.NamedEntity#getName()
     * @see #getNamedEntity()
     * @generated
     */
    EAttribute getNamedEntity_Name();

    /**
     * Returns the meta object for class '{@link OpenDDS.Specification <em>Specification</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Specification</em>'.
     * @see OpenDDS.Specification
     * @generated
     */
    EClass getSpecification();

    /**
     * Returns the meta object for class '{@link OpenDDS.DomainEntity <em>Domain Entity</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Domain Entity</em>'.
     * @see OpenDDS.DomainEntity
     * @generated
     */
    EClass getDomainEntity();

    /**
     * Returns the meta object for class '{@link OpenDDS.ContentFilteredTopic <em>Content Filtered Topic</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Content Filtered Topic</em>'.
     * @see OpenDDS.ContentFilteredTopic
     * @generated
     */
    EClass getContentFilteredTopic();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ContentFilteredTopic#getFilter_expression <em>Filter expression</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Filter expression</em>'.
     * @see OpenDDS.ContentFilteredTopic#getFilter_expression()
     * @see #getContentFilteredTopic()
     * @generated
     */
    EAttribute getContentFilteredTopic_Filter_expression();

    /**
     * Returns the meta object for class '{@link OpenDDS.MultiTopic <em>Multi Topic</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Multi Topic</em>'.
     * @see OpenDDS.MultiTopic
     * @generated
     */
    EClass getMultiTopic();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.MultiTopic#getSubscription_expression <em>Subscription expression</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Subscription expression</em>'.
     * @see OpenDDS.MultiTopic#getSubscription_expression()
     * @see #getMultiTopic()
     * @generated
     */
    EAttribute getMultiTopic_Subscription_expression();

    /**
     * Returns the meta object for class '{@link OpenDDS.Topic <em>Topic</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Topic</em>'.
     * @see OpenDDS.Topic
     * @generated
     */
    EClass getTopic();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getDurability_service <em>Durability service</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Durability service</em>'.
     * @see OpenDDS.Topic#getDurability_service()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Durability_service();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getTransport_priority <em>Transport priority</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Transport priority</em>'.
     * @see OpenDDS.Topic#getTransport_priority()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Transport_priority();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getTopic_data <em>Topic data</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Topic data</em>'.
     * @see OpenDDS.Topic#getTopic_data()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Topic_data();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getResource_limits <em>Resource limits</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Resource limits</em>'.
     * @see OpenDDS.Topic#getResource_limits()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Resource_limits();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getReliability <em>Reliability</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Reliability</em>'.
     * @see OpenDDS.Topic#getReliability()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Reliability();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getOwnership <em>Ownership</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Ownership</em>'.
     * @see OpenDDS.Topic#getOwnership()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Ownership();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getLiveliness <em>Liveliness</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Liveliness</em>'.
     * @see OpenDDS.Topic#getLiveliness()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Liveliness();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getHistory <em>History</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>History</em>'.
     * @see OpenDDS.Topic#getHistory()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_History();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getDurability <em>Durability</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Durability</em>'.
     * @see OpenDDS.Topic#getDurability()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Durability();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getDestination_order <em>Destination order</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Destination order</em>'.
     * @see OpenDDS.Topic#getDestination_order()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Destination_order();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getDeadline <em>Deadline</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Deadline</em>'.
     * @see OpenDDS.Topic#getDeadline()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Deadline();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Topic#getLatency_budget <em>Latency budget</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Latency budget</em>'.
     * @see OpenDDS.Topic#getLatency_budget()
     * @see #getTopic()
     * @generated
     */
    EReference getTopic_Latency_budget();

    /**
     * Returns the meta object for class '{@link OpenDDS.TopicDescription <em>Topic Description</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Topic Description</em>'.
     * @see OpenDDS.TopicDescription
     * @generated
     */
    EClass getTopicDescription();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.TopicDescription#getType <em>Type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Type</em>'.
     * @see OpenDDS.TopicDescription#getType()
     * @see #getTopicDescription()
     * @generated
     */
    EReference getTopicDescription_Type();

    /**
     * Returns the meta object for class '{@link OpenDDS.Array <em>Array</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Array</em>'.
     * @see OpenDDS.Array
     * @generated
     */
    EClass getArray();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.Array#getLength <em>Length</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Length</em>'.
     * @see OpenDDS.Array#getLength()
     * @see #getArray()
     * @generated
     */
    EAttribute getArray_Length();

    /**
     * Returns the meta object for class '{@link OpenDDS.OBoolean <em>OBoolean</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OBoolean</em>'.
     * @see OpenDDS.OBoolean
     * @generated
     */
    EClass getOBoolean();

    /**
     * Returns the meta object for class '{@link OpenDDS.Case <em>Case</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Case</em>'.
     * @see OpenDDS.Case
     * @generated
     */
    EClass getCase();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.Case#getLabels <em>Labels</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Labels</em>'.
     * @see OpenDDS.Case#getLabels()
     * @see #getCase()
     * @generated
     */
    EAttribute getCase_Labels();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Case#getType <em>Type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Type</em>'.
     * @see OpenDDS.Case#getType()
     * @see #getCase()
     * @generated
     */
    EReference getCase_Type();

    /**
     * Returns the meta object for class '{@link OpenDDS.OChar <em>OChar</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OChar</em>'.
     * @see OpenDDS.OChar
     * @generated
     */
    EClass getOChar();

    /**
     * Returns the meta object for class '{@link OpenDDS.Collection <em>Collection</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Collection</em>'.
     * @see OpenDDS.Collection
     * @generated
     */
    EClass getCollection();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Collection#getType <em>Type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Type</em>'.
     * @see OpenDDS.Collection#getType()
     * @see #getCollection()
     * @generated
     */
    EReference getCollection_Type();

    /**
     * Returns the meta object for class '{@link OpenDDS.ConstructedTopicType <em>Constructed Topic Type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Constructed Topic Type</em>'.
     * @see OpenDDS.ConstructedTopicType
     * @generated
     */
    EClass getConstructedTopicType();

    /**
     * Returns the meta object for class '{@link OpenDDS.ODouble <em>ODouble</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>ODouble</em>'.
     * @see OpenDDS.ODouble
     * @generated
     */
    EClass getODouble();

    /**
     * Returns the meta object for class '{@link OpenDDS.Enum <em>Enum</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Enum</em>'.
     * @see OpenDDS.Enum
     * @generated
     */
    EClass getEnum();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.Enum#getLabels <em>Labels</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Labels</em>'.
     * @see OpenDDS.Enum#getLabels()
     * @see #getEnum()
     * @generated
     */
    EAttribute getEnum_Labels();

    /**
     * Returns the meta object for class '{@link OpenDDS.OFloat <em>OFloat</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OFloat</em>'.
     * @see OpenDDS.OFloat
     * @generated
     */
    EClass getOFloat();

    /**
     * Returns the meta object for class '{@link OpenDDS.Key <em>Key</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Key</em>'.
     * @see OpenDDS.Key
     * @generated
     */
    EClass getKey();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Key#getMember <em>Member</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Member</em>'.
     * @see OpenDDS.Key#getMember()
     * @see #getKey()
     * @generated
     */
    EReference getKey_Member();

    /**
     * Returns the meta object for class '{@link OpenDDS.KeyField <em>Key Field</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Key Field</em>'.
     * @see OpenDDS.KeyField
     * @generated
     */
    EClass getKeyField();

    /**
     * Returns the meta object for class '{@link OpenDDS.OLong <em>OLong</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OLong</em>'.
     * @see OpenDDS.OLong
     * @generated
     */
    EClass getOLong();

    /**
     * Returns the meta object for class '{@link OpenDDS.OLongLong <em>OLong Long</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OLong Long</em>'.
     * @see OpenDDS.OLongLong
     * @generated
     */
    EClass getOLongLong();

    /**
     * Returns the meta object for class '{@link OpenDDS.OOctet <em>OOctet</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OOctet</em>'.
     * @see OpenDDS.OOctet
     * @generated
     */
    EClass getOOctet();

    /**
     * Returns the meta object for class '{@link OpenDDS.Sequence <em>Sequence</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Sequence</em>'.
     * @see OpenDDS.Sequence
     * @generated
     */
    EClass getSequence();

    /**
     * Returns the meta object for class '{@link OpenDDS.OShort <em>OShort</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OShort</em>'.
     * @see OpenDDS.OShort
     * @generated
     */
    EClass getOShort();

    /**
     * Returns the meta object for class '{@link OpenDDS.Simple <em>Simple</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Simple</em>'.
     * @see OpenDDS.Simple
     * @generated
     */
    EClass getSimple();

    /**
     * Returns the meta object for class '{@link OpenDDS.OString <em>OString</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OString</em>'.
     * @see OpenDDS.OString
     * @generated
     */
    EClass getOString();

    /**
     * Returns the meta object for class '{@link OpenDDS.TopicStruct <em>Topic Struct</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Topic Struct</em>'.
     * @see OpenDDS.TopicStruct
     * @generated
     */
    EClass getTopicStruct();

    /**
     * Returns the meta object for the containment reference list '{@link OpenDDS.TopicStruct#getMembers <em>Members</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference list '<em>Members</em>'.
     * @see OpenDDS.TopicStruct#getMembers()
     * @see #getTopicStruct()
     * @generated
     */
    EReference getTopicStruct_Members();

    /**
     * Returns the meta object for the containment reference list '{@link OpenDDS.TopicStruct#getKeys <em>Keys</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference list '<em>Keys</em>'.
     * @see OpenDDS.TopicStruct#getKeys()
     * @see #getTopicStruct()
     * @generated
     */
    EReference getTopicStruct_Keys();

    /**
     * Returns the meta object for class '{@link OpenDDS.TopicField <em>Topic Field</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Topic Field</em>'.
     * @see OpenDDS.TopicField
     * @generated
     */
    EClass getTopicField();

    /**
     * Returns the meta object for class '{@link OpenDDS.Typedef <em>Typedef</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Typedef</em>'.
     * @see OpenDDS.Typedef
     * @generated
     */
    EClass getTypedef();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Typedef#getType <em>Type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Type</em>'.
     * @see OpenDDS.Typedef#getType()
     * @see #getTypedef()
     * @generated
     */
    EReference getTypedef_Type();

    /**
     * Returns the meta object for class '{@link OpenDDS.OULong <em>OU Long</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OU Long</em>'.
     * @see OpenDDS.OULong
     * @generated
     */
    EClass getOULong();

    /**
     * Returns the meta object for class '{@link OpenDDS.OULongLong <em>OU Long Long</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OU Long Long</em>'.
     * @see OpenDDS.OULongLong
     * @generated
     */
    EClass getOULongLong();

    /**
     * Returns the meta object for class '{@link OpenDDS.Union <em>Union</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Union</em>'.
     * @see OpenDDS.Union
     * @generated
     */
    EClass getUnion();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Union#getSwitch <em>Switch</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Switch</em>'.
     * @see OpenDDS.Union#getSwitch()
     * @see #getUnion()
     * @generated
     */
    EReference getUnion_Switch();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.Union#getCases <em>Cases</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Cases</em>'.
     * @see OpenDDS.Union#getCases()
     * @see #getUnion()
     * @generated
     */
    EReference getUnion_Cases();

    /**
     * Returns the meta object for class '{@link OpenDDS.OUShort <em>OU Short</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>OU Short</em>'.
     * @see OpenDDS.OUShort
     * @generated
     */
    EClass getOUShort();

    /**
     * Returns the meta object for class '{@link OpenDDS.DataReader <em>Data Reader</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Data Reader</em>'.
     * @see OpenDDS.DataReader
     * @generated
     */
    EClass getDataReader();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReader#getTopic <em>Topic</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Topic</em>'.
     * @see OpenDDS.DataReader#getTopic()
     * @see #getDataReader()
     * @generated
     */
    EReference getDataReader_Topic();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReader#getReader_data_lifecycle <em>Reader data lifecycle</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Reader data lifecycle</em>'.
     * @see OpenDDS.DataReader#getReader_data_lifecycle()
     * @see #getDataReader()
     * @generated
     */
    EReference getDataReader_Reader_data_lifecycle();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReader#getTransport_priority <em>Transport priority</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Transport priority</em>'.
     * @see OpenDDS.DataReader#getTransport_priority()
     * @see #getDataReader()
     * @generated
     */
    EReference getDataReader_Transport_priority();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReader#getDurability_service <em>Durability service</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Durability service</em>'.
     * @see OpenDDS.DataReader#getDurability_service()
     * @see #getDataReader()
     * @generated
     */
    EReference getDataReader_Durability_service();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReader#getOwnership_strength <em>Ownership strength</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Ownership strength</em>'.
     * @see OpenDDS.DataReader#getOwnership_strength()
     * @see #getDataReader()
     * @generated
     */
    EReference getDataReader_Ownership_strength();

    /**
     * Returns the meta object for class '{@link OpenDDS.DataReaderWriter <em>Data Reader Writer</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Data Reader Writer</em>'.
     * @see OpenDDS.DataReaderWriter
     * @generated
     */
    EClass getDataReaderWriter();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getDurability <em>Durability</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Durability</em>'.
     * @see OpenDDS.DataReaderWriter#getDurability()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_Durability();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getDestination_order <em>Destination order</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Destination order</em>'.
     * @see OpenDDS.DataReaderWriter#getDestination_order()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_Destination_order();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getDeadline <em>Deadline</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Deadline</em>'.
     * @see OpenDDS.DataReaderWriter#getDeadline()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_Deadline();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getHistory <em>History</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>History</em>'.
     * @see OpenDDS.DataReaderWriter#getHistory()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_History();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getUser_data <em>User data</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>User data</em>'.
     * @see OpenDDS.DataReaderWriter#getUser_data()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_User_data();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getResource_limits <em>Resource limits</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Resource limits</em>'.
     * @see OpenDDS.DataReaderWriter#getResource_limits()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_Resource_limits();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getOwnership <em>Ownership</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Ownership</em>'.
     * @see OpenDDS.DataReaderWriter#getOwnership()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_Ownership();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getLiveliness <em>Liveliness</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Liveliness</em>'.
     * @see OpenDDS.DataReaderWriter#getLiveliness()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_Liveliness();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getLatency_budget <em>Latency budget</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Latency budget</em>'.
     * @see OpenDDS.DataReaderWriter#getLatency_budget()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_Latency_budget();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataReaderWriter#getReliability <em>Reliability</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Reliability</em>'.
     * @see OpenDDS.DataReaderWriter#getReliability()
     * @see #getDataReaderWriter()
     * @generated
     */
    EReference getDataReaderWriter_Reliability();

    /**
     * Returns the meta object for class '{@link OpenDDS.DataWriter <em>Data Writer</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Data Writer</em>'.
     * @see OpenDDS.DataWriter
     * @generated
     */
    EClass getDataWriter();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataWriter#getTopic <em>Topic</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Topic</em>'.
     * @see OpenDDS.DataWriter#getTopic()
     * @see #getDataWriter()
     * @generated
     */
    EReference getDataWriter_Topic();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DataWriter#getWriter_data_lifecycle <em>Writer data lifecycle</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Writer data lifecycle</em>'.
     * @see OpenDDS.DataWriter#getWriter_data_lifecycle()
     * @see #getDataWriter()
     * @generated
     */
    EReference getDataWriter_Writer_data_lifecycle();

    /**
     * Returns the meta object for class '{@link OpenDDS.Domain <em>Domain</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Domain</em>'.
     * @see OpenDDS.Domain
     * @generated
     */
    EClass getDomain();

    /**
     * Returns the meta object for class '{@link OpenDDS.DomainParticipant <em>Domain Participant</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Domain Participant</em>'.
     * @see OpenDDS.DomainParticipant
     * @generated
     */
    EClass getDomainParticipant();

    /**
     * Returns the meta object for the containment reference list '{@link OpenDDS.DomainParticipant#getSubscribers <em>Subscribers</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference list '<em>Subscribers</em>'.
     * @see OpenDDS.DomainParticipant#getSubscribers()
     * @see #getDomainParticipant()
     * @generated
     */
    EReference getDomainParticipant_Subscribers();

    /**
     * Returns the meta object for the containment reference list '{@link OpenDDS.DomainParticipant#getPublishers <em>Publishers</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference list '<em>Publishers</em>'.
     * @see OpenDDS.DomainParticipant#getPublishers()
     * @see #getDomainParticipant()
     * @generated
     */
    EReference getDomainParticipant_Publishers();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DomainParticipant#getEntity_factory <em>Entity factory</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Entity factory</em>'.
     * @see OpenDDS.DomainParticipant#getEntity_factory()
     * @see #getDomainParticipant()
     * @generated
     */
    EReference getDomainParticipant_Entity_factory();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DomainParticipant#getUser_data <em>User data</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>User data</em>'.
     * @see OpenDDS.DomainParticipant#getUser_data()
     * @see #getDomainParticipant()
     * @generated
     */
    EReference getDomainParticipant_User_data();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.DomainParticipant#getDomain <em>Domain</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Domain</em>'.
     * @see OpenDDS.DomainParticipant#getDomain()
     * @see #getDomainParticipant()
     * @generated
     */
    EReference getDomainParticipant_Domain();

    /**
     * Returns the meta object for class '{@link OpenDDS.Publisher <em>Publisher</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Publisher</em>'.
     * @see OpenDDS.Publisher
     * @generated
     */
    EClass getPublisher();

    /**
     * Returns the meta object for the containment reference list '{@link OpenDDS.Publisher#getWriters <em>Writers</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference list '<em>Writers</em>'.
     * @see OpenDDS.Publisher#getWriters()
     * @see #getPublisher()
     * @generated
     */
    EReference getPublisher_Writers();

    /**
     * Returns the meta object for class '{@link OpenDDS.PublisherSubscriber <em>Publisher Subscriber</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Publisher Subscriber</em>'.
     * @see OpenDDS.PublisherSubscriber
     * @generated
     */
    EClass getPublisherSubscriber();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.PublisherSubscriber#getEntity_factory <em>Entity factory</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Entity factory</em>'.
     * @see OpenDDS.PublisherSubscriber#getEntity_factory()
     * @see #getPublisherSubscriber()
     * @generated
     */
    EReference getPublisherSubscriber_Entity_factory();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.PublisherSubscriber#getPresentation <em>Presentation</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Presentation</em>'.
     * @see OpenDDS.PublisherSubscriber#getPresentation()
     * @see #getPublisherSubscriber()
     * @generated
     */
    EReference getPublisherSubscriber_Presentation();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.PublisherSubscriber#getGroup_data <em>Group data</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Group data</em>'.
     * @see OpenDDS.PublisherSubscriber#getGroup_data()
     * @see #getPublisherSubscriber()
     * @generated
     */
    EReference getPublisherSubscriber_Group_data();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.PublisherSubscriber#getPartition <em>Partition</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Partition</em>'.
     * @see OpenDDS.PublisherSubscriber#getPartition()
     * @see #getPublisherSubscriber()
     * @generated
     */
    EReference getPublisherSubscriber_Partition();

    /**
     * Returns the meta object for the reference '{@link OpenDDS.PublisherSubscriber#getTransport <em>Transport</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference '<em>Transport</em>'.
     * @see OpenDDS.PublisherSubscriber#getTransport()
     * @see #getPublisherSubscriber()
     * @generated
     */
    EReference getPublisherSubscriber_Transport();

    /**
     * Returns the meta object for class '{@link OpenDDS.Subscriber <em>Subscriber</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Subscriber</em>'.
     * @see OpenDDS.Subscriber
     * @generated
     */
    EClass getSubscriber();

    /**
     * Returns the meta object for the containment reference list '{@link OpenDDS.Subscriber#getReaders <em>Readers</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference list '<em>Readers</em>'.
     * @see OpenDDS.Subscriber#getReaders()
     * @see #getSubscriber()
     * @generated
     */
    EReference getSubscriber_Readers();

    /**
     * Returns the meta object for class '{@link OpenDDS.DeadlineQosPolicy <em>Deadline Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Deadline Qos Policy</em>'.
     * @see OpenDDS.DeadlineQosPolicy
     * @generated
     */
    EClass getDeadlineQosPolicy();

    /**
     * Returns the meta object for the containment reference '{@link OpenDDS.DeadlineQosPolicy#getPeriod <em>Period</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference '<em>Period</em>'.
     * @see OpenDDS.DeadlineQosPolicy#getPeriod()
     * @see #getDeadlineQosPolicy()
     * @generated
     */
    EReference getDeadlineQosPolicy_Period();

    /**
     * Returns the meta object for class '{@link OpenDDS.DestinationOrderQosPolicy <em>Destination Order Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Destination Order Qos Policy</em>'.
     * @see OpenDDS.DestinationOrderQosPolicy
     * @generated
     */
    EClass getDestinationOrderQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.DestinationOrderQosPolicy#getKind <em>Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Kind</em>'.
     * @see OpenDDS.DestinationOrderQosPolicy#getKind()
     * @see #getDestinationOrderQosPolicy()
     * @generated
     */
    EAttribute getDestinationOrderQosPolicy_Kind();

    /**
     * Returns the meta object for class '{@link OpenDDS.DurabilityQosPolicy <em>Durability Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Durability Qos Policy</em>'.
     * @see OpenDDS.DurabilityQosPolicy
     * @generated
     */
    EClass getDurabilityQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.DurabilityQosPolicy#getKind <em>Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Kind</em>'.
     * @see OpenDDS.DurabilityQosPolicy#getKind()
     * @see #getDurabilityQosPolicy()
     * @generated
     */
    EAttribute getDurabilityQosPolicy_Kind();

    /**
     * Returns the meta object for class '{@link OpenDDS.DurabilityServiceQosPolicy <em>Durability Service Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Durability Service Qos Policy</em>'.
     * @see OpenDDS.DurabilityServiceQosPolicy
     * @generated
     */
    EClass getDurabilityServiceQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.DurabilityServiceQosPolicy#getHistory_depth <em>History depth</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>History depth</em>'.
     * @see OpenDDS.DurabilityServiceQosPolicy#getHistory_depth()
     * @see #getDurabilityServiceQosPolicy()
     * @generated
     */
    EAttribute getDurabilityServiceQosPolicy_History_depth();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.DurabilityServiceQosPolicy#getHistory_kind <em>History kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>History kind</em>'.
     * @see OpenDDS.DurabilityServiceQosPolicy#getHistory_kind()
     * @see #getDurabilityServiceQosPolicy()
     * @generated
     */
    EAttribute getDurabilityServiceQosPolicy_History_kind();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.DurabilityServiceQosPolicy#getMax_instances <em>Max instances</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Max instances</em>'.
     * @see OpenDDS.DurabilityServiceQosPolicy#getMax_instances()
     * @see #getDurabilityServiceQosPolicy()
     * @generated
     */
    EAttribute getDurabilityServiceQosPolicy_Max_instances();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.DurabilityServiceQosPolicy#getMax_samples <em>Max samples</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Max samples</em>'.
     * @see OpenDDS.DurabilityServiceQosPolicy#getMax_samples()
     * @see #getDurabilityServiceQosPolicy()
     * @generated
     */
    EAttribute getDurabilityServiceQosPolicy_Max_samples();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.DurabilityServiceQosPolicy#getMax_samples_per_instance <em>Max samples per instance</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Max samples per instance</em>'.
     * @see OpenDDS.DurabilityServiceQosPolicy#getMax_samples_per_instance()
     * @see #getDurabilityServiceQosPolicy()
     * @generated
     */
    EAttribute getDurabilityServiceQosPolicy_Max_samples_per_instance();

    /**
     * Returns the meta object for the containment reference '{@link OpenDDS.DurabilityServiceQosPolicy#getService_cleanup_delay <em>Service cleanup delay</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference '<em>Service cleanup delay</em>'.
     * @see OpenDDS.DurabilityServiceQosPolicy#getService_cleanup_delay()
     * @see #getDurabilityServiceQosPolicy()
     * @generated
     */
    EReference getDurabilityServiceQosPolicy_Service_cleanup_delay();

    /**
     * Returns the meta object for class '{@link OpenDDS.EntityFactoryQosPolicy <em>Entity Factory Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Entity Factory Qos Policy</em>'.
     * @see OpenDDS.EntityFactoryQosPolicy
     * @generated
     */
    EClass getEntityFactoryQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.EntityFactoryQosPolicy#isAutoenable_created_entities <em>Autoenable created entities</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Autoenable created entities</em>'.
     * @see OpenDDS.EntityFactoryQosPolicy#isAutoenable_created_entities()
     * @see #getEntityFactoryQosPolicy()
     * @generated
     */
    EAttribute getEntityFactoryQosPolicy_Autoenable_created_entities();

    /**
     * Returns the meta object for class '{@link OpenDDS.GroupDataQosPolicy <em>Group Data Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Group Data Qos Policy</em>'.
     * @see OpenDDS.GroupDataQosPolicy
     * @generated
     */
    EClass getGroupDataQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.GroupDataQosPolicy#getValue <em>Value</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Value</em>'.
     * @see OpenDDS.GroupDataQosPolicy#getValue()
     * @see #getGroupDataQosPolicy()
     * @generated
     */
    EAttribute getGroupDataQosPolicy_Value();

    /**
     * Returns the meta object for class '{@link OpenDDS.HistoryQosPolicy <em>History Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>History Qos Policy</em>'.
     * @see OpenDDS.HistoryQosPolicy
     * @generated
     */
    EClass getHistoryQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.HistoryQosPolicy#getDepth <em>Depth</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Depth</em>'.
     * @see OpenDDS.HistoryQosPolicy#getDepth()
     * @see #getHistoryQosPolicy()
     * @generated
     */
    EAttribute getHistoryQosPolicy_Depth();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.HistoryQosPolicy#getKind <em>Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Kind</em>'.
     * @see OpenDDS.HistoryQosPolicy#getKind()
     * @see #getHistoryQosPolicy()
     * @generated
     */
    EAttribute getHistoryQosPolicy_Kind();

    /**
     * Returns the meta object for class '{@link OpenDDS.LatencyBudgetQosPolicy <em>Latency Budget Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Latency Budget Qos Policy</em>'.
     * @see OpenDDS.LatencyBudgetQosPolicy
     * @generated
     */
    EClass getLatencyBudgetQosPolicy();

    /**
     * Returns the meta object for the containment reference '{@link OpenDDS.LatencyBudgetQosPolicy#getDuration <em>Duration</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference '<em>Duration</em>'.
     * @see OpenDDS.LatencyBudgetQosPolicy#getDuration()
     * @see #getLatencyBudgetQosPolicy()
     * @generated
     */
    EReference getLatencyBudgetQosPolicy_Duration();

    /**
     * Returns the meta object for class '{@link OpenDDS.LifespanQosPolicy <em>Lifespan Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Lifespan Qos Policy</em>'.
     * @see OpenDDS.LifespanQosPolicy
     * @generated
     */
    EClass getLifespanQosPolicy();

    /**
     * Returns the meta object for the containment reference '{@link OpenDDS.LifespanQosPolicy#getDuration <em>Duration</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference '<em>Duration</em>'.
     * @see OpenDDS.LifespanQosPolicy#getDuration()
     * @see #getLifespanQosPolicy()
     * @generated
     */
    EReference getLifespanQosPolicy_Duration();

    /**
     * Returns the meta object for class '{@link OpenDDS.LivelinessQosPolicy <em>Liveliness Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Liveliness Qos Policy</em>'.
     * @see OpenDDS.LivelinessQosPolicy
     * @generated
     */
    EClass getLivelinessQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.LivelinessQosPolicy#getKind <em>Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Kind</em>'.
     * @see OpenDDS.LivelinessQosPolicy#getKind()
     * @see #getLivelinessQosPolicy()
     * @generated
     */
    EAttribute getLivelinessQosPolicy_Kind();

    /**
     * Returns the meta object for the containment reference '{@link OpenDDS.LivelinessQosPolicy#getLease_duration <em>Lease duration</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference '<em>Lease duration</em>'.
     * @see OpenDDS.LivelinessQosPolicy#getLease_duration()
     * @see #getLivelinessQosPolicy()
     * @generated
     */
    EReference getLivelinessQosPolicy_Lease_duration();

    /**
     * Returns the meta object for class '{@link OpenDDS.OwnershipQosPolicy <em>Ownership Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Ownership Qos Policy</em>'.
     * @see OpenDDS.OwnershipQosPolicy
     * @generated
     */
    EClass getOwnershipQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.OwnershipQosPolicy#getKind <em>Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Kind</em>'.
     * @see OpenDDS.OwnershipQosPolicy#getKind()
     * @see #getOwnershipQosPolicy()
     * @generated
     */
    EAttribute getOwnershipQosPolicy_Kind();

    /**
     * Returns the meta object for class '{@link OpenDDS.OwnershipStrengthQosPolicy <em>Ownership Strength Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Ownership Strength Qos Policy</em>'.
     * @see OpenDDS.OwnershipStrengthQosPolicy
     * @generated
     */
    EClass getOwnershipStrengthQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.OwnershipStrengthQosPolicy#getValue <em>Value</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Value</em>'.
     * @see OpenDDS.OwnershipStrengthQosPolicy#getValue()
     * @see #getOwnershipStrengthQosPolicy()
     * @generated
     */
    EAttribute getOwnershipStrengthQosPolicy_Value();

    /**
     * Returns the meta object for class '{@link OpenDDS.PartitionQosPolicy <em>Partition Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Partition Qos Policy</em>'.
     * @see OpenDDS.PartitionQosPolicy
     * @generated
     */
    EClass getPartitionQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.PartitionQosPolicy#getName <em>Name</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Name</em>'.
     * @see OpenDDS.PartitionQosPolicy#getName()
     * @see #getPartitionQosPolicy()
     * @generated
     */
    EAttribute getPartitionQosPolicy_Name();

    /**
     * Returns the meta object for class '{@link OpenDDS.PresentationQosPolicy <em>Presentation Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Presentation Qos Policy</em>'.
     * @see OpenDDS.PresentationQosPolicy
     * @generated
     */
    EClass getPresentationQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.PresentationQosPolicy#getAccess_scope <em>Access scope</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Access scope</em>'.
     * @see OpenDDS.PresentationQosPolicy#getAccess_scope()
     * @see #getPresentationQosPolicy()
     * @generated
     */
    EAttribute getPresentationQosPolicy_Access_scope();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.PresentationQosPolicy#isCoherent_access <em>Coherent access</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Coherent access</em>'.
     * @see OpenDDS.PresentationQosPolicy#isCoherent_access()
     * @see #getPresentationQosPolicy()
     * @generated
     */
    EAttribute getPresentationQosPolicy_Coherent_access();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.PresentationQosPolicy#isOrdered_access <em>Ordered access</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Ordered access</em>'.
     * @see OpenDDS.PresentationQosPolicy#isOrdered_access()
     * @see #getPresentationQosPolicy()
     * @generated
     */
    EAttribute getPresentationQosPolicy_Ordered_access();

    /**
     * Returns the meta object for class '{@link OpenDDS.QosPolicy <em>Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Qos Policy</em>'.
     * @see OpenDDS.QosPolicy
     * @generated
     */
    EClass getQosPolicy();

    /**
     * Returns the meta object for class '{@link OpenDDS.ReaderDataLifecycleQosPolicy <em>Reader Data Lifecycle Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Reader Data Lifecycle Qos Policy</em>'.
     * @see OpenDDS.ReaderDataLifecycleQosPolicy
     * @generated
     */
    EClass getReaderDataLifecycleQosPolicy();

    /**
     * Returns the meta object for the containment reference '{@link OpenDDS.ReaderDataLifecycleQosPolicy#getAutopurge_nowriter_samples_delay <em>Autopurge nowriter samples delay</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference '<em>Autopurge nowriter samples delay</em>'.
     * @see OpenDDS.ReaderDataLifecycleQosPolicy#getAutopurge_nowriter_samples_delay()
     * @see #getReaderDataLifecycleQosPolicy()
     * @generated
     */
    EReference getReaderDataLifecycleQosPolicy_Autopurge_nowriter_samples_delay();

    /**
     * Returns the meta object for class '{@link OpenDDS.ReliabilityQosPolicy <em>Reliability Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Reliability Qos Policy</em>'.
     * @see OpenDDS.ReliabilityQosPolicy
     * @generated
     */
    EClass getReliabilityQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ReliabilityQosPolicy#getKind <em>Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Kind</em>'.
     * @see OpenDDS.ReliabilityQosPolicy#getKind()
     * @see #getReliabilityQosPolicy()
     * @generated
     */
    EAttribute getReliabilityQosPolicy_Kind();

    /**
     * Returns the meta object for the containment reference '{@link OpenDDS.ReliabilityQosPolicy#getMax_blocking_time <em>Max blocking time</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference '<em>Max blocking time</em>'.
     * @see OpenDDS.ReliabilityQosPolicy#getMax_blocking_time()
     * @see #getReliabilityQosPolicy()
     * @generated
     */
    EReference getReliabilityQosPolicy_Max_blocking_time();

    /**
     * Returns the meta object for class '{@link OpenDDS.ResourceLimitsQosPolicy <em>Resource Limits Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Resource Limits Qos Policy</em>'.
     * @see OpenDDS.ResourceLimitsQosPolicy
     * @generated
     */
    EClass getResourceLimitsQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ResourceLimitsQosPolicy#getMax_instances <em>Max instances</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Max instances</em>'.
     * @see OpenDDS.ResourceLimitsQosPolicy#getMax_instances()
     * @see #getResourceLimitsQosPolicy()
     * @generated
     */
    EAttribute getResourceLimitsQosPolicy_Max_instances();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ResourceLimitsQosPolicy#getMax_samples <em>Max samples</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Max samples</em>'.
     * @see OpenDDS.ResourceLimitsQosPolicy#getMax_samples()
     * @see #getResourceLimitsQosPolicy()
     * @generated
     */
    EAttribute getResourceLimitsQosPolicy_Max_samples();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ResourceLimitsQosPolicy#getMax_samples_per_instance <em>Max samples per instance</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Max samples per instance</em>'.
     * @see OpenDDS.ResourceLimitsQosPolicy#getMax_samples_per_instance()
     * @see #getResourceLimitsQosPolicy()
     * @generated
     */
    EAttribute getResourceLimitsQosPolicy_Max_samples_per_instance();

    /**
     * Returns the meta object for class '{@link OpenDDS.TimeBasedFilterQosPolicy <em>Time Based Filter Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Time Based Filter Qos Policy</em>'.
     * @see OpenDDS.TimeBasedFilterQosPolicy
     * @generated
     */
    EClass getTimeBasedFilterQosPolicy();

    /**
     * Returns the meta object for the containment reference '{@link OpenDDS.TimeBasedFilterQosPolicy#getMinimum_separation <em>Minimum separation</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the containment reference '<em>Minimum separation</em>'.
     * @see OpenDDS.TimeBasedFilterQosPolicy#getMinimum_separation()
     * @see #getTimeBasedFilterQosPolicy()
     * @generated
     */
    EReference getTimeBasedFilterQosPolicy_Minimum_separation();

    /**
     * Returns the meta object for class '{@link OpenDDS.TopicDataQosPolicy <em>Topic Data Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Topic Data Qos Policy</em>'.
     * @see OpenDDS.TopicDataQosPolicy
     * @generated
     */
    EClass getTopicDataQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.TopicDataQosPolicy#getValue <em>Value</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Value</em>'.
     * @see OpenDDS.TopicDataQosPolicy#getValue()
     * @see #getTopicDataQosPolicy()
     * @generated
     */
    EAttribute getTopicDataQosPolicy_Value();

    /**
     * Returns the meta object for class '{@link OpenDDS.TransportPriorityQosPolicy <em>Transport Priority Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Transport Priority Qos Policy</em>'.
     * @see OpenDDS.TransportPriorityQosPolicy
     * @generated
     */
    EClass getTransportPriorityQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.TransportPriorityQosPolicy#getValue <em>Value</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Value</em>'.
     * @see OpenDDS.TransportPriorityQosPolicy#getValue()
     * @see #getTransportPriorityQosPolicy()
     * @generated
     */
    EAttribute getTransportPriorityQosPolicy_Value();

    /**
     * Returns the meta object for class '{@link OpenDDS.UserDataQosPolicy <em>User Data Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>User Data Qos Policy</em>'.
     * @see OpenDDS.UserDataQosPolicy
     * @generated
     */
    EClass getUserDataQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.UserDataQosPolicy#getValue <em>Value</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Value</em>'.
     * @see OpenDDS.UserDataQosPolicy#getValue()
     * @see #getUserDataQosPolicy()
     * @generated
     */
    EAttribute getUserDataQosPolicy_Value();

    /**
     * Returns the meta object for class '{@link OpenDDS.Period <em>Period</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Period</em>'.
     * @see OpenDDS.Period
     * @generated
     */
    EClass getPeriod();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.Period#getSeconds <em>Seconds</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Seconds</em>'.
     * @see OpenDDS.Period#getSeconds()
     * @see #getPeriod()
     * @generated
     */
    EAttribute getPeriod_Seconds();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.Period#getNanoseconds <em>Nanoseconds</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Nanoseconds</em>'.
     * @see OpenDDS.Period#getNanoseconds()
     * @see #getPeriod()
     * @generated
     */
    EAttribute getPeriod_Nanoseconds();

    /**
     * Returns the meta object for class '{@link OpenDDS.WriterDataLifecycleQosPolicy <em>Writer Data Lifecycle Qos Policy</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Writer Data Lifecycle Qos Policy</em>'.
     * @see OpenDDS.WriterDataLifecycleQosPolicy
     * @generated
     */
    EClass getWriterDataLifecycleQosPolicy();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.WriterDataLifecycleQosPolicy#isAutodispose_unregistered_instances <em>Autodispose unregistered instances</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Autodispose unregistered instances</em>'.
     * @see OpenDDS.WriterDataLifecycleQosPolicy#isAutodispose_unregistered_instances()
     * @see #getWriterDataLifecycleQosPolicy()
     * @generated
     */
    EAttribute getWriterDataLifecycleQosPolicy_Autodispose_unregistered_instances();

    /**
     * Returns the meta object for class '{@link OpenDDS.ApplicationTarget <em>Application Target</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Application Target</em>'.
     * @see OpenDDS.ApplicationTarget
     * @generated
     */
    EClass getApplicationTarget();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ApplicationTarget#getComponent_type <em>Component type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Component type</em>'.
     * @see OpenDDS.ApplicationTarget#getComponent_type()
     * @see #getApplicationTarget()
     * @generated
     */
    EAttribute getApplicationTarget_Component_type();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ApplicationTarget#getLanguage <em>Language</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Language</em>'.
     * @see OpenDDS.ApplicationTarget#getLanguage()
     * @see #getApplicationTarget()
     * @generated
     */
    EAttribute getApplicationTarget_Language();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ApplicationTarget#getPlatform <em>Platform</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Platform</em>'.
     * @see OpenDDS.ApplicationTarget#getPlatform()
     * @see #getApplicationTarget()
     * @generated
     */
    EAttribute getApplicationTarget_Platform();

    /**
     * Returns the meta object for the reference list '{@link OpenDDS.ApplicationTarget#getParticipants <em>Participants</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the reference list '<em>Participants</em>'.
     * @see OpenDDS.ApplicationTarget#getParticipants()
     * @see #getApplicationTarget()
     * @generated
     */
    EReference getApplicationTarget_Participants();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.ApplicationTarget#getService_arguments <em>Service arguments</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Service arguments</em>'.
     * @see OpenDDS.ApplicationTarget#getService_arguments()
     * @see #getApplicationTarget()
     * @generated
     */
    EAttribute getApplicationTarget_Service_arguments();

    /**
     * Returns the meta object for class '{@link OpenDDS.Transport <em>Transport</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for class '<em>Transport</em>'.
     * @see OpenDDS.Transport
     * @generated
     */
    EClass getTransport();

    /**
     * Returns the meta object for the attribute '{@link OpenDDS.Transport#getTransport_id <em>Transport id</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for the attribute '<em>Transport id</em>'.
     * @see OpenDDS.Transport#getTransport_id()
     * @see #getTransport()
     * @generated
     */
    EAttribute getTransport_Transport_id();

    /**
     * Returns the meta object for enum '{@link OpenDDS.DestinationOrderQosPolicyKind <em>Destination Order Qos Policy Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Destination Order Qos Policy Kind</em>'.
     * @see OpenDDS.DestinationOrderQosPolicyKind
     * @generated
     */
    EEnum getDestinationOrderQosPolicyKind();

    /**
     * Returns the meta object for enum '{@link OpenDDS.DurabilityQosPolicyKind <em>Durability Qos Policy Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Durability Qos Policy Kind</em>'.
     * @see OpenDDS.DurabilityQosPolicyKind
     * @generated
     */
    EEnum getDurabilityQosPolicyKind();

    /**
     * Returns the meta object for enum '{@link OpenDDS.HistoryQosPolicyKind <em>History Qos Policy Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>History Qos Policy Kind</em>'.
     * @see OpenDDS.HistoryQosPolicyKind
     * @generated
     */
    EEnum getHistoryQosPolicyKind();

    /**
     * Returns the meta object for enum '{@link OpenDDS.LivelinessQosPolicyKind <em>Liveliness Qos Policy Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Liveliness Qos Policy Kind</em>'.
     * @see OpenDDS.LivelinessQosPolicyKind
     * @generated
     */
    EEnum getLivelinessQosPolicyKind();

    /**
     * Returns the meta object for enum '{@link OpenDDS.OwnershipQosPolicyKind <em>Ownership Qos Policy Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Ownership Qos Policy Kind</em>'.
     * @see OpenDDS.OwnershipQosPolicyKind
     * @generated
     */
    EEnum getOwnershipQosPolicyKind();

    /**
     * Returns the meta object for enum '{@link OpenDDS.PresentationQosPolicyAccessScopeKind <em>Presentation Qos Policy Access Scope Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Presentation Qos Policy Access Scope Kind</em>'.
     * @see OpenDDS.PresentationQosPolicyAccessScopeKind
     * @generated
     */
    EEnum getPresentationQosPolicyAccessScopeKind();

    /**
     * Returns the meta object for enum '{@link OpenDDS.ReliabilityQosPolicyKind <em>Reliability Qos Policy Kind</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Reliability Qos Policy Kind</em>'.
     * @see OpenDDS.ReliabilityQosPolicyKind
     * @generated
     */
    EEnum getReliabilityQosPolicyKind();

    /**
     * Returns the meta object for enum '{@link OpenDDS.ComponentType <em>Component Type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Component Type</em>'.
     * @see OpenDDS.ComponentType
     * @generated
     */
    EEnum getComponentType();

    /**
     * Returns the meta object for enum '{@link OpenDDS.LanguageType <em>Language Type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Language Type</em>'.
     * @see OpenDDS.LanguageType
     * @generated
     */
    EEnum getLanguageType();

    /**
     * Returns the meta object for enum '{@link OpenDDS.PlatformType <em>Platform Type</em>}'.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the meta object for enum '<em>Platform Type</em>'.
     * @see OpenDDS.PlatformType
     * @generated
     */
    EEnum getPlatformType();

    /**
     * Returns the factory that creates the instances of the model.
     * <!-- begin-user-doc -->
     * <!-- end-user-doc -->
     * @return the factory that creates the instances of the model.
     * @generated
     */
    OpenDDSFactory getOpenDDSFactory();

    /**
     * <!-- begin-user-doc -->
     * Defines literals for the meta objects that represent
     * <ul>
     *   <li>each class,</li>
     *   <li>each feature of each class,</li>
     *   <li>each enum,</li>
     *   <li>and each data type</li>
     * </ul>
     * <!-- end-user-doc -->
     * @generated
     */
    interface Literals {
        /**
         * The meta object literal for the '{@link OpenDDS.EntityImpl <em>Entity</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.EntityImpl
         * @see OpenDDS.OpenDDSPackageImpl#getEntity()
         * @generated
         */
        EClass ENTITY = eINSTANCE.getEntity();

        /**
         * The meta object literal for the '{@link OpenDDS.NamedEntityImpl <em>Named Entity</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.NamedEntityImpl
         * @see OpenDDS.OpenDDSPackageImpl#getNamedEntity()
         * @generated
         */
        EClass NAMED_ENTITY = eINSTANCE.getNamedEntity();

        /**
         * The meta object literal for the '<em><b>Name</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute NAMED_ENTITY__NAME = eINSTANCE.getNamedEntity_Name();

        /**
         * The meta object literal for the '{@link OpenDDS.SpecificationImpl <em>Specification</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.SpecificationImpl
         * @see OpenDDS.OpenDDSPackageImpl#getSpecification()
         * @generated
         */
        EClass SPECIFICATION = eINSTANCE.getSpecification();

        /**
         * The meta object literal for the '{@link OpenDDS.DomainEntityImpl <em>Domain Entity</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DomainEntityImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDomainEntity()
         * @generated
         */
        EClass DOMAIN_ENTITY = eINSTANCE.getDomainEntity();

        /**
         * The meta object literal for the '{@link OpenDDS.ContentFilteredTopicImpl <em>Content Filtered Topic</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ContentFilteredTopicImpl
         * @see OpenDDS.OpenDDSPackageImpl#getContentFilteredTopic()
         * @generated
         */
        EClass CONTENT_FILTERED_TOPIC = eINSTANCE.getContentFilteredTopic();

        /**
         * The meta object literal for the '<em><b>Filter expression</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute CONTENT_FILTERED_TOPIC__FILTER_EXPRESSION = eINSTANCE.getContentFilteredTopic_Filter_expression();

        /**
         * The meta object literal for the '{@link OpenDDS.MultiTopicImpl <em>Multi Topic</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.MultiTopicImpl
         * @see OpenDDS.OpenDDSPackageImpl#getMultiTopic()
         * @generated
         */
        EClass MULTI_TOPIC = eINSTANCE.getMultiTopic();

        /**
         * The meta object literal for the '<em><b>Subscription expression</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute MULTI_TOPIC__SUBSCRIPTION_EXPRESSION = eINSTANCE.getMultiTopic_Subscription_expression();

        /**
         * The meta object literal for the '{@link OpenDDS.TopicImpl <em>Topic</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TopicImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTopic()
         * @generated
         */
        EClass TOPIC = eINSTANCE.getTopic();

        /**
         * The meta object literal for the '<em><b>Durability service</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__DURABILITY_SERVICE = eINSTANCE.getTopic_Durability_service();

        /**
         * The meta object literal for the '<em><b>Transport priority</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__TRANSPORT_PRIORITY = eINSTANCE.getTopic_Transport_priority();

        /**
         * The meta object literal for the '<em><b>Topic data</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__TOPIC_DATA = eINSTANCE.getTopic_Topic_data();

        /**
         * The meta object literal for the '<em><b>Resource limits</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__RESOURCE_LIMITS = eINSTANCE.getTopic_Resource_limits();

        /**
         * The meta object literal for the '<em><b>Reliability</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__RELIABILITY = eINSTANCE.getTopic_Reliability();

        /**
         * The meta object literal for the '<em><b>Ownership</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__OWNERSHIP = eINSTANCE.getTopic_Ownership();

        /**
         * The meta object literal for the '<em><b>Liveliness</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__LIVELINESS = eINSTANCE.getTopic_Liveliness();

        /**
         * The meta object literal for the '<em><b>History</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__HISTORY = eINSTANCE.getTopic_History();

        /**
         * The meta object literal for the '<em><b>Durability</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__DURABILITY = eINSTANCE.getTopic_Durability();

        /**
         * The meta object literal for the '<em><b>Destination order</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__DESTINATION_ORDER = eINSTANCE.getTopic_Destination_order();

        /**
         * The meta object literal for the '<em><b>Deadline</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__DEADLINE = eINSTANCE.getTopic_Deadline();

        /**
         * The meta object literal for the '<em><b>Latency budget</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC__LATENCY_BUDGET = eINSTANCE.getTopic_Latency_budget();

        /**
         * The meta object literal for the '{@link OpenDDS.TopicDescriptionImpl <em>Topic Description</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TopicDescriptionImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTopicDescription()
         * @generated
         */
        EClass TOPIC_DESCRIPTION = eINSTANCE.getTopicDescription();

        /**
         * The meta object literal for the '<em><b>Type</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC_DESCRIPTION__TYPE = eINSTANCE.getTopicDescription_Type();

        /**
         * The meta object literal for the '{@link OpenDDS.ArrayImpl <em>Array</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ArrayImpl
         * @see OpenDDS.OpenDDSPackageImpl#getArray()
         * @generated
         */
        EClass ARRAY = eINSTANCE.getArray();

        /**
         * The meta object literal for the '<em><b>Length</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute ARRAY__LENGTH = eINSTANCE.getArray_Length();

        /**
         * The meta object literal for the '{@link OpenDDS.OBooleanImpl <em>OBoolean</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OBooleanImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOBoolean()
         * @generated
         */
        EClass OBOOLEAN = eINSTANCE.getOBoolean();

        /**
         * The meta object literal for the '{@link OpenDDS.CaseImpl <em>Case</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.CaseImpl
         * @see OpenDDS.OpenDDSPackageImpl#getCase()
         * @generated
         */
        EClass CASE = eINSTANCE.getCase();

        /**
         * The meta object literal for the '<em><b>Labels</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute CASE__LABELS = eINSTANCE.getCase_Labels();

        /**
         * The meta object literal for the '<em><b>Type</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference CASE__TYPE = eINSTANCE.getCase_Type();

        /**
         * The meta object literal for the '{@link OpenDDS.OCharImpl <em>OChar</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OCharImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOChar()
         * @generated
         */
        EClass OCHAR = eINSTANCE.getOChar();

        /**
         * The meta object literal for the '{@link OpenDDS.CollectionImpl <em>Collection</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.CollectionImpl
         * @see OpenDDS.OpenDDSPackageImpl#getCollection()
         * @generated
         */
        EClass COLLECTION = eINSTANCE.getCollection();

        /**
         * The meta object literal for the '<em><b>Type</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference COLLECTION__TYPE = eINSTANCE.getCollection_Type();

        /**
         * The meta object literal for the '{@link OpenDDS.ConstructedTopicTypeImpl <em>Constructed Topic Type</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ConstructedTopicTypeImpl
         * @see OpenDDS.OpenDDSPackageImpl#getConstructedTopicType()
         * @generated
         */
        EClass CONSTRUCTED_TOPIC_TYPE = eINSTANCE.getConstructedTopicType();

        /**
         * The meta object literal for the '{@link OpenDDS.ODoubleImpl <em>ODouble</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ODoubleImpl
         * @see OpenDDS.OpenDDSPackageImpl#getODouble()
         * @generated
         */
        EClass ODOUBLE = eINSTANCE.getODouble();

        /**
         * The meta object literal for the '{@link OpenDDS.EnumImpl <em>Enum</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.EnumImpl
         * @see OpenDDS.OpenDDSPackageImpl#getEnum()
         * @generated
         */
        EClass ENUM = eINSTANCE.getEnum();

        /**
         * The meta object literal for the '<em><b>Labels</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute ENUM__LABELS = eINSTANCE.getEnum_Labels();

        /**
         * The meta object literal for the '{@link OpenDDS.OFloatImpl <em>OFloat</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OFloatImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOFloat()
         * @generated
         */
        EClass OFLOAT = eINSTANCE.getOFloat();

        /**
         * The meta object literal for the '{@link OpenDDS.KeyImpl <em>Key</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.KeyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getKey()
         * @generated
         */
        EClass KEY = eINSTANCE.getKey();

        /**
         * The meta object literal for the '<em><b>Member</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference KEY__MEMBER = eINSTANCE.getKey_Member();

        /**
         * The meta object literal for the '{@link OpenDDS.KeyFieldImpl <em>Key Field</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.KeyFieldImpl
         * @see OpenDDS.OpenDDSPackageImpl#getKeyField()
         * @generated
         */
        EClass KEY_FIELD = eINSTANCE.getKeyField();

        /**
         * The meta object literal for the '{@link OpenDDS.OLongImpl <em>OLong</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OLongImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOLong()
         * @generated
         */
        EClass OLONG = eINSTANCE.getOLong();

        /**
         * The meta object literal for the '{@link OpenDDS.OLongLongImpl <em>OLong Long</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OLongLongImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOLongLong()
         * @generated
         */
        EClass OLONG_LONG = eINSTANCE.getOLongLong();

        /**
         * The meta object literal for the '{@link OpenDDS.OOctetImpl <em>OOctet</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OOctetImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOOctet()
         * @generated
         */
        EClass OOCTET = eINSTANCE.getOOctet();

        /**
         * The meta object literal for the '{@link OpenDDS.SequenceImpl <em>Sequence</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.SequenceImpl
         * @see OpenDDS.OpenDDSPackageImpl#getSequence()
         * @generated
         */
        EClass SEQUENCE = eINSTANCE.getSequence();

        /**
         * The meta object literal for the '{@link OpenDDS.OShortImpl <em>OShort</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OShortImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOShort()
         * @generated
         */
        EClass OSHORT = eINSTANCE.getOShort();

        /**
         * The meta object literal for the '{@link OpenDDS.SimpleImpl <em>Simple</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.SimpleImpl
         * @see OpenDDS.OpenDDSPackageImpl#getSimple()
         * @generated
         */
        EClass SIMPLE = eINSTANCE.getSimple();

        /**
         * The meta object literal for the '{@link OpenDDS.OStringImpl <em>OString</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OStringImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOString()
         * @generated
         */
        EClass OSTRING = eINSTANCE.getOString();

        /**
         * The meta object literal for the '{@link OpenDDS.TopicStructImpl <em>Topic Struct</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TopicStructImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTopicStruct()
         * @generated
         */
        EClass TOPIC_STRUCT = eINSTANCE.getTopicStruct();

        /**
         * The meta object literal for the '<em><b>Members</b></em>' containment reference list feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC_STRUCT__MEMBERS = eINSTANCE.getTopicStruct_Members();

        /**
         * The meta object literal for the '<em><b>Keys</b></em>' containment reference list feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TOPIC_STRUCT__KEYS = eINSTANCE.getTopicStruct_Keys();

        /**
         * The meta object literal for the '{@link OpenDDS.TopicFieldImpl <em>Topic Field</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TopicFieldImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTopicField()
         * @generated
         */
        EClass TOPIC_FIELD = eINSTANCE.getTopicField();

        /**
         * The meta object literal for the '{@link OpenDDS.TypedefImpl <em>Typedef</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TypedefImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTypedef()
         * @generated
         */
        EClass TYPEDEF = eINSTANCE.getTypedef();

        /**
         * The meta object literal for the '<em><b>Type</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TYPEDEF__TYPE = eINSTANCE.getTypedef_Type();

        /**
         * The meta object literal for the '{@link OpenDDS.OULongImpl <em>OU Long</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OULongImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOULong()
         * @generated
         */
        EClass OU_LONG = eINSTANCE.getOULong();

        /**
         * The meta object literal for the '{@link OpenDDS.OULongLongImpl <em>OU Long Long</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OULongLongImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOULongLong()
         * @generated
         */
        EClass OU_LONG_LONG = eINSTANCE.getOULongLong();

        /**
         * The meta object literal for the '{@link OpenDDS.UnionImpl <em>Union</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.UnionImpl
         * @see OpenDDS.OpenDDSPackageImpl#getUnion()
         * @generated
         */
        EClass UNION = eINSTANCE.getUnion();

        /**
         * The meta object literal for the '<em><b>Switch</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference UNION__SWITCH = eINSTANCE.getUnion_Switch();

        /**
         * The meta object literal for the '<em><b>Cases</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference UNION__CASES = eINSTANCE.getUnion_Cases();

        /**
         * The meta object literal for the '{@link OpenDDS.OUShortImpl <em>OU Short</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OUShortImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOUShort()
         * @generated
         */
        EClass OU_SHORT = eINSTANCE.getOUShort();

        /**
         * The meta object literal for the '{@link OpenDDS.DataReaderImpl <em>Data Reader</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DataReaderImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDataReader()
         * @generated
         */
        EClass DATA_READER = eINSTANCE.getDataReader();

        /**
         * The meta object literal for the '<em><b>Topic</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER__TOPIC = eINSTANCE.getDataReader_Topic();

        /**
         * The meta object literal for the '<em><b>Reader data lifecycle</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER__READER_DATA_LIFECYCLE = eINSTANCE.getDataReader_Reader_data_lifecycle();

        /**
         * The meta object literal for the '<em><b>Transport priority</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER__TRANSPORT_PRIORITY = eINSTANCE.getDataReader_Transport_priority();

        /**
         * The meta object literal for the '<em><b>Durability service</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER__DURABILITY_SERVICE = eINSTANCE.getDataReader_Durability_service();

        /**
         * The meta object literal for the '<em><b>Ownership strength</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER__OWNERSHIP_STRENGTH = eINSTANCE.getDataReader_Ownership_strength();

        /**
         * The meta object literal for the '{@link OpenDDS.DataReaderWriterImpl <em>Data Reader Writer</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DataReaderWriterImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDataReaderWriter()
         * @generated
         */
        EClass DATA_READER_WRITER = eINSTANCE.getDataReaderWriter();

        /**
         * The meta object literal for the '<em><b>Durability</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__DURABILITY = eINSTANCE.getDataReaderWriter_Durability();

        /**
         * The meta object literal for the '<em><b>Destination order</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__DESTINATION_ORDER = eINSTANCE.getDataReaderWriter_Destination_order();

        /**
         * The meta object literal for the '<em><b>Deadline</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__DEADLINE = eINSTANCE.getDataReaderWriter_Deadline();

        /**
         * The meta object literal for the '<em><b>History</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__HISTORY = eINSTANCE.getDataReaderWriter_History();

        /**
         * The meta object literal for the '<em><b>User data</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__USER_DATA = eINSTANCE.getDataReaderWriter_User_data();

        /**
         * The meta object literal for the '<em><b>Resource limits</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__RESOURCE_LIMITS = eINSTANCE.getDataReaderWriter_Resource_limits();

        /**
         * The meta object literal for the '<em><b>Ownership</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__OWNERSHIP = eINSTANCE.getDataReaderWriter_Ownership();

        /**
         * The meta object literal for the '<em><b>Liveliness</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__LIVELINESS = eINSTANCE.getDataReaderWriter_Liveliness();

        /**
         * The meta object literal for the '<em><b>Latency budget</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__LATENCY_BUDGET = eINSTANCE.getDataReaderWriter_Latency_budget();

        /**
         * The meta object literal for the '<em><b>Reliability</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_READER_WRITER__RELIABILITY = eINSTANCE.getDataReaderWriter_Reliability();

        /**
         * The meta object literal for the '{@link OpenDDS.DataWriterImpl <em>Data Writer</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DataWriterImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDataWriter()
         * @generated
         */
        EClass DATA_WRITER = eINSTANCE.getDataWriter();

        /**
         * The meta object literal for the '<em><b>Topic</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_WRITER__TOPIC = eINSTANCE.getDataWriter_Topic();

        /**
         * The meta object literal for the '<em><b>Writer data lifecycle</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DATA_WRITER__WRITER_DATA_LIFECYCLE = eINSTANCE.getDataWriter_Writer_data_lifecycle();

        /**
         * The meta object literal for the '{@link OpenDDS.DomainImpl <em>Domain</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DomainImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDomain()
         * @generated
         */
        EClass DOMAIN = eINSTANCE.getDomain();

        /**
         * The meta object literal for the '{@link OpenDDS.DomainParticipantImpl <em>Domain Participant</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DomainParticipantImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDomainParticipant()
         * @generated
         */
        EClass DOMAIN_PARTICIPANT = eINSTANCE.getDomainParticipant();

        /**
         * The meta object literal for the '<em><b>Subscribers</b></em>' containment reference list feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DOMAIN_PARTICIPANT__SUBSCRIBERS = eINSTANCE.getDomainParticipant_Subscribers();

        /**
         * The meta object literal for the '<em><b>Publishers</b></em>' containment reference list feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DOMAIN_PARTICIPANT__PUBLISHERS = eINSTANCE.getDomainParticipant_Publishers();

        /**
         * The meta object literal for the '<em><b>Entity factory</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DOMAIN_PARTICIPANT__ENTITY_FACTORY = eINSTANCE.getDomainParticipant_Entity_factory();

        /**
         * The meta object literal for the '<em><b>User data</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DOMAIN_PARTICIPANT__USER_DATA = eINSTANCE.getDomainParticipant_User_data();

        /**
         * The meta object literal for the '<em><b>Domain</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DOMAIN_PARTICIPANT__DOMAIN = eINSTANCE.getDomainParticipant_Domain();

        /**
         * The meta object literal for the '{@link OpenDDS.PublisherImpl <em>Publisher</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.PublisherImpl
         * @see OpenDDS.OpenDDSPackageImpl#getPublisher()
         * @generated
         */
        EClass PUBLISHER = eINSTANCE.getPublisher();

        /**
         * The meta object literal for the '<em><b>Writers</b></em>' containment reference list feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference PUBLISHER__WRITERS = eINSTANCE.getPublisher_Writers();

        /**
         * The meta object literal for the '{@link OpenDDS.PublisherSubscriberImpl <em>Publisher Subscriber</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.PublisherSubscriberImpl
         * @see OpenDDS.OpenDDSPackageImpl#getPublisherSubscriber()
         * @generated
         */
        EClass PUBLISHER_SUBSCRIBER = eINSTANCE.getPublisherSubscriber();

        /**
         * The meta object literal for the '<em><b>Entity factory</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference PUBLISHER_SUBSCRIBER__ENTITY_FACTORY = eINSTANCE.getPublisherSubscriber_Entity_factory();

        /**
         * The meta object literal for the '<em><b>Presentation</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference PUBLISHER_SUBSCRIBER__PRESENTATION = eINSTANCE.getPublisherSubscriber_Presentation();

        /**
         * The meta object literal for the '<em><b>Group data</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference PUBLISHER_SUBSCRIBER__GROUP_DATA = eINSTANCE.getPublisherSubscriber_Group_data();

        /**
         * The meta object literal for the '<em><b>Partition</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference PUBLISHER_SUBSCRIBER__PARTITION = eINSTANCE.getPublisherSubscriber_Partition();

        /**
         * The meta object literal for the '<em><b>Transport</b></em>' reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference PUBLISHER_SUBSCRIBER__TRANSPORT = eINSTANCE.getPublisherSubscriber_Transport();

        /**
         * The meta object literal for the '{@link OpenDDS.SubscriberImpl <em>Subscriber</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.SubscriberImpl
         * @see OpenDDS.OpenDDSPackageImpl#getSubscriber()
         * @generated
         */
        EClass SUBSCRIBER = eINSTANCE.getSubscriber();

        /**
         * The meta object literal for the '<em><b>Readers</b></em>' containment reference list feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference SUBSCRIBER__READERS = eINSTANCE.getSubscriber_Readers();

        /**
         * The meta object literal for the '{@link OpenDDS.DeadlineQosPolicyImpl <em>Deadline Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DeadlineQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDeadlineQosPolicy()
         * @generated
         */
        EClass DEADLINE_QOS_POLICY = eINSTANCE.getDeadlineQosPolicy();

        /**
         * The meta object literal for the '<em><b>Period</b></em>' containment reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DEADLINE_QOS_POLICY__PERIOD = eINSTANCE.getDeadlineQosPolicy_Period();

        /**
         * The meta object literal for the '{@link OpenDDS.DestinationOrderQosPolicyImpl <em>Destination Order Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DestinationOrderQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDestinationOrderQosPolicy()
         * @generated
         */
        EClass DESTINATION_ORDER_QOS_POLICY = eINSTANCE.getDestinationOrderQosPolicy();

        /**
         * The meta object literal for the '<em><b>Kind</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute DESTINATION_ORDER_QOS_POLICY__KIND = eINSTANCE.getDestinationOrderQosPolicy_Kind();

        /**
         * The meta object literal for the '{@link OpenDDS.DurabilityQosPolicyImpl <em>Durability Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DurabilityQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDurabilityQosPolicy()
         * @generated
         */
        EClass DURABILITY_QOS_POLICY = eINSTANCE.getDurabilityQosPolicy();

        /**
         * The meta object literal for the '<em><b>Kind</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute DURABILITY_QOS_POLICY__KIND = eINSTANCE.getDurabilityQosPolicy_Kind();

        /**
         * The meta object literal for the '{@link OpenDDS.DurabilityServiceQosPolicyImpl <em>Durability Service Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DurabilityServiceQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getDurabilityServiceQosPolicy()
         * @generated
         */
        EClass DURABILITY_SERVICE_QOS_POLICY = eINSTANCE.getDurabilityServiceQosPolicy();

        /**
         * The meta object literal for the '<em><b>History depth</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute DURABILITY_SERVICE_QOS_POLICY__HISTORY_DEPTH = eINSTANCE
                .getDurabilityServiceQosPolicy_History_depth();

        /**
         * The meta object literal for the '<em><b>History kind</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute DURABILITY_SERVICE_QOS_POLICY__HISTORY_KIND = eINSTANCE.getDurabilityServiceQosPolicy_History_kind();

        /**
         * The meta object literal for the '<em><b>Max instances</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute DURABILITY_SERVICE_QOS_POLICY__MAX_INSTANCES = eINSTANCE
                .getDurabilityServiceQosPolicy_Max_instances();

        /**
         * The meta object literal for the '<em><b>Max samples</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES = eINSTANCE.getDurabilityServiceQosPolicy_Max_samples();

        /**
         * The meta object literal for the '<em><b>Max samples per instance</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute DURABILITY_SERVICE_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE = eINSTANCE
                .getDurabilityServiceQosPolicy_Max_samples_per_instance();

        /**
         * The meta object literal for the '<em><b>Service cleanup delay</b></em>' containment reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference DURABILITY_SERVICE_QOS_POLICY__SERVICE_CLEANUP_DELAY = eINSTANCE
                .getDurabilityServiceQosPolicy_Service_cleanup_delay();

        /**
         * The meta object literal for the '{@link OpenDDS.EntityFactoryQosPolicyImpl <em>Entity Factory Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.EntityFactoryQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getEntityFactoryQosPolicy()
         * @generated
         */
        EClass ENTITY_FACTORY_QOS_POLICY = eINSTANCE.getEntityFactoryQosPolicy();

        /**
         * The meta object literal for the '<em><b>Autoenable created entities</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute ENTITY_FACTORY_QOS_POLICY__AUTOENABLE_CREATED_ENTITIES = eINSTANCE
                .getEntityFactoryQosPolicy_Autoenable_created_entities();

        /**
         * The meta object literal for the '{@link OpenDDS.GroupDataQosPolicyImpl <em>Group Data Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.GroupDataQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getGroupDataQosPolicy()
         * @generated
         */
        EClass GROUP_DATA_QOS_POLICY = eINSTANCE.getGroupDataQosPolicy();

        /**
         * The meta object literal for the '<em><b>Value</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute GROUP_DATA_QOS_POLICY__VALUE = eINSTANCE.getGroupDataQosPolicy_Value();

        /**
         * The meta object literal for the '{@link OpenDDS.HistoryQosPolicyImpl <em>History Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.HistoryQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getHistoryQosPolicy()
         * @generated
         */
        EClass HISTORY_QOS_POLICY = eINSTANCE.getHistoryQosPolicy();

        /**
         * The meta object literal for the '<em><b>Depth</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute HISTORY_QOS_POLICY__DEPTH = eINSTANCE.getHistoryQosPolicy_Depth();

        /**
         * The meta object literal for the '<em><b>Kind</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute HISTORY_QOS_POLICY__KIND = eINSTANCE.getHistoryQosPolicy_Kind();

        /**
         * The meta object literal for the '{@link OpenDDS.LatencyBudgetQosPolicyImpl <em>Latency Budget Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.LatencyBudgetQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getLatencyBudgetQosPolicy()
         * @generated
         */
        EClass LATENCY_BUDGET_QOS_POLICY = eINSTANCE.getLatencyBudgetQosPolicy();

        /**
         * The meta object literal for the '<em><b>Duration</b></em>' containment reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference LATENCY_BUDGET_QOS_POLICY__DURATION = eINSTANCE.getLatencyBudgetQosPolicy_Duration();

        /**
         * The meta object literal for the '{@link OpenDDS.LifespanQosPolicyImpl <em>Lifespan Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.LifespanQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getLifespanQosPolicy()
         * @generated
         */
        EClass LIFESPAN_QOS_POLICY = eINSTANCE.getLifespanQosPolicy();

        /**
         * The meta object literal for the '<em><b>Duration</b></em>' containment reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference LIFESPAN_QOS_POLICY__DURATION = eINSTANCE.getLifespanQosPolicy_Duration();

        /**
         * The meta object literal for the '{@link OpenDDS.LivelinessQosPolicyImpl <em>Liveliness Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.LivelinessQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getLivelinessQosPolicy()
         * @generated
         */
        EClass LIVELINESS_QOS_POLICY = eINSTANCE.getLivelinessQosPolicy();

        /**
         * The meta object literal for the '<em><b>Kind</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute LIVELINESS_QOS_POLICY__KIND = eINSTANCE.getLivelinessQosPolicy_Kind();

        /**
         * The meta object literal for the '<em><b>Lease duration</b></em>' containment reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference LIVELINESS_QOS_POLICY__LEASE_DURATION = eINSTANCE.getLivelinessQosPolicy_Lease_duration();

        /**
         * The meta object literal for the '{@link OpenDDS.OwnershipQosPolicyImpl <em>Ownership Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OwnershipQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOwnershipQosPolicy()
         * @generated
         */
        EClass OWNERSHIP_QOS_POLICY = eINSTANCE.getOwnershipQosPolicy();

        /**
         * The meta object literal for the '<em><b>Kind</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute OWNERSHIP_QOS_POLICY__KIND = eINSTANCE.getOwnershipQosPolicy_Kind();

        /**
         * The meta object literal for the '{@link OpenDDS.OwnershipStrengthQosPolicyImpl <em>Ownership Strength Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OwnershipStrengthQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getOwnershipStrengthQosPolicy()
         * @generated
         */
        EClass OWNERSHIP_STRENGTH_QOS_POLICY = eINSTANCE.getOwnershipStrengthQosPolicy();

        /**
         * The meta object literal for the '<em><b>Value</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute OWNERSHIP_STRENGTH_QOS_POLICY__VALUE = eINSTANCE.getOwnershipStrengthQosPolicy_Value();

        /**
         * The meta object literal for the '{@link OpenDDS.PartitionQosPolicyImpl <em>Partition Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.PartitionQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getPartitionQosPolicy()
         * @generated
         */
        EClass PARTITION_QOS_POLICY = eINSTANCE.getPartitionQosPolicy();

        /**
         * The meta object literal for the '<em><b>Name</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute PARTITION_QOS_POLICY__NAME = eINSTANCE.getPartitionQosPolicy_Name();

        /**
         * The meta object literal for the '{@link OpenDDS.PresentationQosPolicyImpl <em>Presentation Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.PresentationQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getPresentationQosPolicy()
         * @generated
         */
        EClass PRESENTATION_QOS_POLICY = eINSTANCE.getPresentationQosPolicy();

        /**
         * The meta object literal for the '<em><b>Access scope</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute PRESENTATION_QOS_POLICY__ACCESS_SCOPE = eINSTANCE.getPresentationQosPolicy_Access_scope();

        /**
         * The meta object literal for the '<em><b>Coherent access</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute PRESENTATION_QOS_POLICY__COHERENT_ACCESS = eINSTANCE.getPresentationQosPolicy_Coherent_access();

        /**
         * The meta object literal for the '<em><b>Ordered access</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute PRESENTATION_QOS_POLICY__ORDERED_ACCESS = eINSTANCE.getPresentationQosPolicy_Ordered_access();

        /**
         * The meta object literal for the '{@link OpenDDS.QosPolicyImpl <em>Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.QosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getQosPolicy()
         * @generated
         */
        EClass QOS_POLICY = eINSTANCE.getQosPolicy();

        /**
         * The meta object literal for the '{@link OpenDDS.ReaderDataLifecycleQosPolicyImpl <em>Reader Data Lifecycle Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ReaderDataLifecycleQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getReaderDataLifecycleQosPolicy()
         * @generated
         */
        EClass READER_DATA_LIFECYCLE_QOS_POLICY = eINSTANCE.getReaderDataLifecycleQosPolicy();

        /**
         * The meta object literal for the '<em><b>Autopurge nowriter samples delay</b></em>' containment reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference READER_DATA_LIFECYCLE_QOS_POLICY__AUTOPURGE_NOWRITER_SAMPLES_DELAY = eINSTANCE
                .getReaderDataLifecycleQosPolicy_Autopurge_nowriter_samples_delay();

        /**
         * The meta object literal for the '{@link OpenDDS.ReliabilityQosPolicyImpl <em>Reliability Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ReliabilityQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getReliabilityQosPolicy()
         * @generated
         */
        EClass RELIABILITY_QOS_POLICY = eINSTANCE.getReliabilityQosPolicy();

        /**
         * The meta object literal for the '<em><b>Kind</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute RELIABILITY_QOS_POLICY__KIND = eINSTANCE.getReliabilityQosPolicy_Kind();

        /**
         * The meta object literal for the '<em><b>Max blocking time</b></em>' containment reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference RELIABILITY_QOS_POLICY__MAX_BLOCKING_TIME = eINSTANCE.getReliabilityQosPolicy_Max_blocking_time();

        /**
         * The meta object literal for the '{@link OpenDDS.ResourceLimitsQosPolicyImpl <em>Resource Limits Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ResourceLimitsQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getResourceLimitsQosPolicy()
         * @generated
         */
        EClass RESOURCE_LIMITS_QOS_POLICY = eINSTANCE.getResourceLimitsQosPolicy();

        /**
         * The meta object literal for the '<em><b>Max instances</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute RESOURCE_LIMITS_QOS_POLICY__MAX_INSTANCES = eINSTANCE.getResourceLimitsQosPolicy_Max_instances();

        /**
         * The meta object literal for the '<em><b>Max samples</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES = eINSTANCE.getResourceLimitsQosPolicy_Max_samples();

        /**
         * The meta object literal for the '<em><b>Max samples per instance</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute RESOURCE_LIMITS_QOS_POLICY__MAX_SAMPLES_PER_INSTANCE = eINSTANCE
                .getResourceLimitsQosPolicy_Max_samples_per_instance();

        /**
         * The meta object literal for the '{@link OpenDDS.TimeBasedFilterQosPolicyImpl <em>Time Based Filter Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TimeBasedFilterQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTimeBasedFilterQosPolicy()
         * @generated
         */
        EClass TIME_BASED_FILTER_QOS_POLICY = eINSTANCE.getTimeBasedFilterQosPolicy();

        /**
         * The meta object literal for the '<em><b>Minimum separation</b></em>' containment reference feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference TIME_BASED_FILTER_QOS_POLICY__MINIMUM_SEPARATION = eINSTANCE
                .getTimeBasedFilterQosPolicy_Minimum_separation();

        /**
         * The meta object literal for the '{@link OpenDDS.TopicDataQosPolicyImpl <em>Topic Data Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TopicDataQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTopicDataQosPolicy()
         * @generated
         */
        EClass TOPIC_DATA_QOS_POLICY = eINSTANCE.getTopicDataQosPolicy();

        /**
         * The meta object literal for the '<em><b>Value</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute TOPIC_DATA_QOS_POLICY__VALUE = eINSTANCE.getTopicDataQosPolicy_Value();

        /**
         * The meta object literal for the '{@link OpenDDS.TransportPriorityQosPolicyImpl <em>Transport Priority Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TransportPriorityQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTransportPriorityQosPolicy()
         * @generated
         */
        EClass TRANSPORT_PRIORITY_QOS_POLICY = eINSTANCE.getTransportPriorityQosPolicy();

        /**
         * The meta object literal for the '<em><b>Value</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute TRANSPORT_PRIORITY_QOS_POLICY__VALUE = eINSTANCE.getTransportPriorityQosPolicy_Value();

        /**
         * The meta object literal for the '{@link OpenDDS.UserDataQosPolicyImpl <em>User Data Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.UserDataQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getUserDataQosPolicy()
         * @generated
         */
        EClass USER_DATA_QOS_POLICY = eINSTANCE.getUserDataQosPolicy();

        /**
         * The meta object literal for the '<em><b>Value</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute USER_DATA_QOS_POLICY__VALUE = eINSTANCE.getUserDataQosPolicy_Value();

        /**
         * The meta object literal for the '{@link OpenDDS.PeriodImpl <em>Period</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.PeriodImpl
         * @see OpenDDS.OpenDDSPackageImpl#getPeriod()
         * @generated
         */
        EClass PERIOD = eINSTANCE.getPeriod();

        /**
         * The meta object literal for the '<em><b>Seconds</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute PERIOD__SECONDS = eINSTANCE.getPeriod_Seconds();

        /**
         * The meta object literal for the '<em><b>Nanoseconds</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute PERIOD__NANOSECONDS = eINSTANCE.getPeriod_Nanoseconds();

        /**
         * The meta object literal for the '{@link OpenDDS.WriterDataLifecycleQosPolicyImpl <em>Writer Data Lifecycle Qos Policy</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.WriterDataLifecycleQosPolicyImpl
         * @see OpenDDS.OpenDDSPackageImpl#getWriterDataLifecycleQosPolicy()
         * @generated
         */
        EClass WRITER_DATA_LIFECYCLE_QOS_POLICY = eINSTANCE.getWriterDataLifecycleQosPolicy();

        /**
         * The meta object literal for the '<em><b>Autodispose unregistered instances</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute WRITER_DATA_LIFECYCLE_QOS_POLICY__AUTODISPOSE_UNREGISTERED_INSTANCES = eINSTANCE
                .getWriterDataLifecycleQosPolicy_Autodispose_unregistered_instances();

        /**
         * The meta object literal for the '{@link OpenDDS.ApplicationTargetImpl <em>Application Target</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ApplicationTargetImpl
         * @see OpenDDS.OpenDDSPackageImpl#getApplicationTarget()
         * @generated
         */
        EClass APPLICATION_TARGET = eINSTANCE.getApplicationTarget();

        /**
         * The meta object literal for the '<em><b>Component type</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute APPLICATION_TARGET__COMPONENT_TYPE = eINSTANCE.getApplicationTarget_Component_type();

        /**
         * The meta object literal for the '<em><b>Language</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute APPLICATION_TARGET__LANGUAGE = eINSTANCE.getApplicationTarget_Language();

        /**
         * The meta object literal for the '<em><b>Platform</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute APPLICATION_TARGET__PLATFORM = eINSTANCE.getApplicationTarget_Platform();

        /**
         * The meta object literal for the '<em><b>Participants</b></em>' reference list feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EReference APPLICATION_TARGET__PARTICIPANTS = eINSTANCE.getApplicationTarget_Participants();

        /**
         * The meta object literal for the '<em><b>Service arguments</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute APPLICATION_TARGET__SERVICE_ARGUMENTS = eINSTANCE.getApplicationTarget_Service_arguments();

        /**
         * The meta object literal for the '{@link OpenDDS.TransportImpl <em>Transport</em>}' class.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.TransportImpl
         * @see OpenDDS.OpenDDSPackageImpl#getTransport()
         * @generated
         */
        EClass TRANSPORT = eINSTANCE.getTransport();

        /**
         * The meta object literal for the '<em><b>Transport id</b></em>' attribute feature.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @generated
         */
        EAttribute TRANSPORT__TRANSPORT_ID = eINSTANCE.getTransport_Transport_id();

        /**
         * The meta object literal for the '{@link OpenDDS.DestinationOrderQosPolicyKind <em>Destination Order Qos Policy Kind</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DestinationOrderQosPolicyKind
         * @see OpenDDS.OpenDDSPackageImpl#getDestinationOrderQosPolicyKind()
         * @generated
         */
        EEnum DESTINATION_ORDER_QOS_POLICY_KIND = eINSTANCE.getDestinationOrderQosPolicyKind();

        /**
         * The meta object literal for the '{@link OpenDDS.DurabilityQosPolicyKind <em>Durability Qos Policy Kind</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.DurabilityQosPolicyKind
         * @see OpenDDS.OpenDDSPackageImpl#getDurabilityQosPolicyKind()
         * @generated
         */
        EEnum DURABILITY_QOS_POLICY_KIND = eINSTANCE.getDurabilityQosPolicyKind();

        /**
         * The meta object literal for the '{@link OpenDDS.HistoryQosPolicyKind <em>History Qos Policy Kind</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.HistoryQosPolicyKind
         * @see OpenDDS.OpenDDSPackageImpl#getHistoryQosPolicyKind()
         * @generated
         */
        EEnum HISTORY_QOS_POLICY_KIND = eINSTANCE.getHistoryQosPolicyKind();

        /**
         * The meta object literal for the '{@link OpenDDS.LivelinessQosPolicyKind <em>Liveliness Qos Policy Kind</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.LivelinessQosPolicyKind
         * @see OpenDDS.OpenDDSPackageImpl#getLivelinessQosPolicyKind()
         * @generated
         */
        EEnum LIVELINESS_QOS_POLICY_KIND = eINSTANCE.getLivelinessQosPolicyKind();

        /**
         * The meta object literal for the '{@link OpenDDS.OwnershipQosPolicyKind <em>Ownership Qos Policy Kind</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.OwnershipQosPolicyKind
         * @see OpenDDS.OpenDDSPackageImpl#getOwnershipQosPolicyKind()
         * @generated
         */
        EEnum OWNERSHIP_QOS_POLICY_KIND = eINSTANCE.getOwnershipQosPolicyKind();

        /**
         * The meta object literal for the '{@link OpenDDS.PresentationQosPolicyAccessScopeKind <em>Presentation Qos Policy Access Scope Kind</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.PresentationQosPolicyAccessScopeKind
         * @see OpenDDS.OpenDDSPackageImpl#getPresentationQosPolicyAccessScopeKind()
         * @generated
         */
        EEnum PRESENTATION_QOS_POLICY_ACCESS_SCOPE_KIND = eINSTANCE.getPresentationQosPolicyAccessScopeKind();

        /**
         * The meta object literal for the '{@link OpenDDS.ReliabilityQosPolicyKind <em>Reliability Qos Policy Kind</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ReliabilityQosPolicyKind
         * @see OpenDDS.OpenDDSPackageImpl#getReliabilityQosPolicyKind()
         * @generated
         */
        EEnum RELIABILITY_QOS_POLICY_KIND = eINSTANCE.getReliabilityQosPolicyKind();

        /**
         * The meta object literal for the '{@link OpenDDS.ComponentType <em>Component Type</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.ComponentType
         * @see OpenDDS.OpenDDSPackageImpl#getComponentType()
         * @generated
         */
        EEnum COMPONENT_TYPE = eINSTANCE.getComponentType();

        /**
         * The meta object literal for the '{@link OpenDDS.LanguageType <em>Language Type</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.LanguageType
         * @see OpenDDS.OpenDDSPackageImpl#getLanguageType()
         * @generated
         */
        EEnum LANGUAGE_TYPE = eINSTANCE.getLanguageType();

        /**
         * The meta object literal for the '{@link OpenDDS.PlatformType <em>Platform Type</em>}' enum.
         * <!-- begin-user-doc -->
         * <!-- end-user-doc -->
         * @see OpenDDS.PlatformType
         * @see OpenDDS.OpenDDSPackageImpl#getPlatformType()
         * @generated
         */
        EEnum PLATFORM_TYPE = eINSTANCE.getPlatformType();

    }

} //OpenDDSPackage
