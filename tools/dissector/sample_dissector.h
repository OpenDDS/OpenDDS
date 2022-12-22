/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DISSECTOR_SAMPLE_DISSECTOR_H_
#define OPENDDS_DISSECTOR_SAMPLE_DISSECTOR_H_

// Wireshark headers need to come before OpenDDS and ACE headers.
// On Windows, config.h from the Wireshark build directory assumes that
// the macros for Windows SDK versions (NTDDI_VERSION, _WIN32_WINNT) are not
// yet defined.  That won't be the case if ACE comes first.

#include "ws_common.h"
#include "ws-wrapper-headers/packet-tcp.h"

#include <epan/value_string.h>
#include <epan/ipproto.h>

// OpenDDS and ACE headers, and others that include OpenDDS/ACE headers start here:

#include "dissector_export.h"
#include <FACE/Fixed.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/DataSampleHeader.h>

#include <ace/Message_Block.h>

#include <glib.h>
#include <gmodule.h>

#include <string>
#include <map>
#include <exception>
#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {

    /**
     * Thrown when there is a inconsistency in the sample dissection or
     * initialization of the fields.
     * Packet will be marked by Wireshark, if thrown during dissection.
     */
    class Sample_Dissector_Error : public std::exception {
    public:
      explicit Sample_Dissector_Error(const std::string & message)
      : message_(message)
      {
      }

      ~Sample_Dissector_Error() throw() {}

      const char* what() const throw() {
        return message_.c_str();
      }

    private:
      std::string message_;

    };

    /**
     * Bundle of Parameters passed between the recursive dissection calls
     * that represents most of the state of the dissector.
     */
    class Wireshark_Bundle {
    public:
      /// Used by the serializer
      ACE_Message_Block block;

      /**
       * OpenDDS Serializer, used to convert packet buffer into ACE/OpenDDS
       * datatypes.
       */
      Serializer serializer;

      /**
       * In order to use the DCPS Serializer, we must pass over the packet
       * twice, once to just get the size from Serializer and ignore the data
       * and again so we can pass the field size to Wireshark for trees before
       * we pass the rest of the field data to Wireshark.
       */
      bool get_size_only;

      /*
       * Information used by us to communicate with Wireshark
       */
      tvbuff_t* tvb;
      packet_info* info;
      proto_tree* tree;
      size_t offset;
#ifndef NO_EXPERT
      expert_field* warning_ef;
#endif

      /// Defines if we are in a indexed structure (Array or Sequence)
      bool use_index;
      /// Defines the index if use_index is true.
      guint32 index;

      Wireshark_Bundle(
        char* data, size_t size, bool swap_bytes, Encoding::Alignment align
      );
      Wireshark_Bundle(const Wireshark_Bundle & other);

      size_t buffer_pos();
      guint8* get_remainder();
    };

    /// Holds Wireshark field information in relationship to a namespace
    struct Field_Context {
    public:
      Field_Context(
        const std::string& short_name, const std::string& long_name,
        ftenum ft, field_display_e fd
      );

      /// Default Wireshark Field Registration Values
      static const hf_register_info default_hf_register_info;

      /// Field's Display/Short Name
      const std::string short_name_;
      /// Field's Namespace/Long Name
      const std::string long_name_;
      /// Wireshark API Field Index
      int hf_;
      /// Wireshark API Field Structure
      hf_register_info hf_info_;
    };
    typedef std::map<std::string, Field_Context*> Field_Contexts;

    /**
     * Base Class for Sample_Dissector, Sample_Field, and all the other
     * elements that make up the dissector tree. Contains global namespace
     * tracking and any information to be passed to Wireshark about the
     * field.
     *
     * The dissector tree represents all the possible valid formats the
     * sample payload dissector can dissect. Its structure and contents are
     * defined by the ITL files read when the plugin is loaded.
     *
     * Simplified Example of the Dissector Tree Structure:
     *
     * Sample_Manager.dissectors_["opendds.sample.payload.Messenger.Message"]
     *             |
     *             V
     * +----------------+
     * |Sample_Dissector|
     * +----------------+  +--------------------+
     * |field_ ----------->|Sample_Field        |
     * |is_root_ = true |  +--------------------+
     * |is_struct = true|  |type_id_ = Undefined|
     * +----------------+  |label_ = "from"     |  +------------------+
     *                     |nested_ -------------->|Sample_Dissector  |
     *                     |next_ ----+         |  +------------------+
     *                     +----------|---------+  |is_root_ = false  |
     *                                |            |is_struct_ = false|
     *                                V            |field_ ---+       |
     *                     +--------------------+  +----------|-------+
     *                     |Sample_Field        |             |
     *                     +--------------------+             V
     *                     |type_id_ = Undefined|  +------------------+
     *                     |label_ = "subject"  |  |Sample_Field      |
     *                     |        ...         |  +------------------+
     *                                             |type_id_ = String |
     *                                             |label_ = ""       |
     *                                             |nested_ = 0       |
     *                                             |next_ = 0         |
     *                                             +------------------+
     */
    class Sample_Base {
    private:
      /// Associated Wireshark Header Field Contexts
      Field_Contexts field_contexts_;

      /// Keep track of the Wireshark namespace to use
      static std::list<std::string> ns_stack_;
      static std::string ns_;

      /// Make ns_ from ns_stack_
      static void rebuild_ns();

    protected:
      /**
       * Create Field_Context for the current dissection element and
       * namespace.
       */
      Field_Context* create_context(ftenum ft, field_display_e fd = BASE_NONE);

      /**
       * Returns context for the current namespace, else null if no such
       * context exists.
       */
      Field_Context* get_context();

      /// Get hf for Wireshark, returns -1 if there is no such context
      int get_hf();

      /// Have the current context's field info registered later
      void add_protocol_field();

    public:
      virtual ~Sample_Base();

      /**
       * Traverse the Dissector Tree nodes to create the protocol information
       * to be passed to Wireshark.
       * This occurs in two passes:
       *   - The first pass creates the field information and evaluates
       *     if the information is valid. If not a Sample_Dissector_Error
       *     will be thrown.
       *   - The second pass will collect the hf_register_info structs
       *     from the Dissectors and Fields.
       */
      virtual void init_ws_fields(bool first_pass) = 0;

      /// Get the full namespace including the "opendds.sample.payload..."
      static std::string get_ns();

      /// Append a name to the end of the namespace
      static void push_ns(const std::string & name);

      /// Remove the last name from the namepspace
      static std::string pop_ns();

      /// Reset the namespace
      static void clear_ns();

      /// Get the last name in the namespace
      static std::string get_label();

      /// Push IDL name "IDL:a/b/c" on to the namespace stack as "a.b.c"
      static void push_idl_name(std::string name);

    };


    class dissector_Export Sample_Dissector;
    typedef std::map<std::string, Sample_Dissector*> Dissector_Map;

    /// Sample_Field describes a single element of the sample. This can be
    /// a fixed length field, such as a short, long, etc. or variable length
    /// such as a string, enum, union, sequence, or another struct.
    ///
    /// Fields are chained together to describe the layout of the sample in
    /// the data buffer
    class dissector_Export Sample_Field : public Sample_Base {
    public:

      enum IDLTypeID {
        Boolean,
        Char,
        Octet,
        WChar,
        Short,
        UShort,
        Long,
        ULong,
        LongLong,
        ULongLong,
        Float,
        Double,
        LongDouble,
        String,  // not fixed, but pre-defined in IDL
        WString,
        Enumeration,
        Undefined
      };

      /// @ctor for fixed length fields, supply a type identifier and a label
      Sample_Field(IDLTypeID type_id, const std::string& label);

      /// @ctor for complex fields, supply a nested Sample_Dissector to handle
      /// the dissection.
      Sample_Field(Sample_Dissector* n, const std::string &label);

      virtual ~Sample_Field();

      /// Add a new field to the chain of fields describing the sample.
      /// returns the argument pointer to facilitate iterative construction
      /// of chains
      Sample_Field* chain(Sample_Field* n);
      Sample_Field* chain(IDLTypeID ti, const std::string& l);
      Sample_Field* chain(Sample_Dissector* n, const std::string& l);

      /// Iterate over the list to return the n'th link returns null if n
      /// exceeds the number of links in the chain
      Sample_Field* get_link(size_t n);

      /// Add the field to the tree, either directly using the configured
      /// format and supplied data pointer, or by handing off to the attached
      /// Sample_Dissector object for further evaluation.
      size_t dissect_i(Wireshark_Bundle& params, bool recur = true);

      size_t compute_length(const Wireshark_Bundle& p);

      void to_stream(std::stringstream& s, Wireshark_Bundle& p);

      void init_ws_fields(bool first_pass);

      /**
       * Get Name of an IDL Basic Type
       */
      static const char* IDLTypeID_string(IDLTypeID type) {
        switch (type) {
          case Boolean:
            return "Boolean";
          case Char:
            return "Char";
          case Octet:
            return "Octet";
          case WChar:
            return "WChar";
          case Short:
            return "Short";
          case UShort:
            return "UShort";
          case Long:
            return "Long";
          case ULong:
            return "ULong";
          case LongLong:
            return "LongLong";
          case ULongLong:
            return "ULongLong";
          case Float:
            return "Float";
          case Double:
            return "Double";
          case LongDouble:
            return "LongDouble";
          case String:
            return "String";
          case WString:
            return "WString";
          case Enumeration:
            return "Enumeration";
          default:
            return "Undefined";
        }
      }

      /// Fixed length fields supply a label for identifying the field along
      /// with a type identifier. Fields that are members of an array or
      /// sequence will have an index value adjusted as the owning dissector
      /// iterates over the members.
      std::string label_;
      IDLTypeID type_id_;

      /// Variable length fields use a nested sample base value to do their
      /// dissection. This can be another configured sample_dissector instance,
      /// or it can be a string, enum, union, or sequence dissector.
      Sample_Dissector* nested_;

      /// A simple linked-list is used to chain fields together. When the
      /// top-level field is deleted, it will iterate over this list deleting
      /// each subsequent field.
      Sample_Field* next_;
    };

    /**
     * A Sample_Dissector is the base type dissector for samples. A sample base
     * instance is initialized with a list of fields that is used to walk
     * through a data buffer to compose a tree of named values.
     *
     * Sample_Dissectors register with a manager that associate an identifier
     * with an instance of a sample base. A base could conceivable be
     * registered with more than one name to allow for aliases, but that
     * would require reference counting, which currently isn't done.
     */
    class dissector_Export Sample_Dissector :
      public Sample_Base
    {
    public:
      Sample_Dissector(const std::string& subtree_label = "");

      virtual ~Sample_Dissector();

      /// Initialize the instance with a type_id used for registration
      /// and a label to be used to identify a sub-tree for presentation.
      /// The type_id may be empty in the case where only a subtree needs
      /// to be defined for complex samples that are not otherwise top-level
      /// types.
      virtual void init(const std::string& subtree_label = "");

      /// Dissect is the hook function called from an owning dissector.
      /// The signature matches that required by Wireshark. The offset
      /// is incremented as a side-effect of calling dissect. The real
      /// work of dissecting a sample is delegated to dissect_i.
      gint dissect(Wireshark_Bundle& params);

      size_t compute_length(const Wireshark_Bundle& p);

      /// Adds a new sample field to the list. The Sample_Dissector instance
      /// takes ownership of this field instance. As a pass-thru to
      /// Sample_Field::chain(), it returns the supplied field pointer
      /// to facilitate simple chain construction.
      Sample_Field* add_field(Sample_Field* field);
      Sample_Field* add_field(Sample_Field::IDLTypeID, const std::string& l);
      Sample_Field* add_field(Sample_Dissector* n, const std::string& l);

      /// Run init_ws_fields on the children of the Sample_Dissector
      void init_ws_proto_tree(bool first_pass);

      virtual void init_ws_fields(bool first_pass);

      /// The actual dissector method. Since a sample can be composed of
      /// complex fields which need to do their own dissection, this method
      /// does not directly modify the supplied offset value, rather it
      /// returns the actual number of octets consumed for this dissection,
      /// however complicated it may be.
      virtual size_t dissect_i(Wireshark_Bundle& p);

      /// Used primarily for constructing unions, returns the typeid for the
      /// first field in the list. Returns Undefined if there is not exactly
      /// one field in the list.
      Sample_Field::IDLTypeID get_field_type();

      virtual std::string stringify(Wireshark_Bundle& p);

      void mark_struct();
      bool is_struct();

    protected:
      /// The list of fields defining the sample.
      Sample_Field* field_;

      /// Values used by the Wireshark framework for rendering a sub-tree.
      /// These get set in init(), a subtree is defined only if label_ is not
      /// empty.
      gint ett_;
      std::string subtree_label_;

      bool is_struct_;
      bool is_root_;
    };

    /**
     * A specialized sample dissector for rendering sequences. Sequences
     * may contain any other type of field, which are rendered separately.
     */
    class dissector_Export Sample_Sequence : public Sample_Dissector {
    public:
      /// @ctor. Creates an empty base element to be initialized after this
      /// sample sequence holder is created
      Sample_Sequence(Sample_Dissector* sub);

      Sample_Dissector* element();

      virtual void init_ws_fields(bool first_pass);

    protected:
      /// Common Dissection Code for Arrays and Sequences
      void dissect_elements(
        Wireshark_Bundle& params, size_t& len, size_t count, size_t count_size
      );

      virtual size_t dissect_i(Wireshark_Bundle &params);

      Sample_Dissector* element_;
    };

    /**
     * A specialized sample dissector for rendering Arrays. Arrays may
     * contain any other type of field, which are rendered separately.
     *
     * Arrays differ from sequences in that the size is fixed at compile
     * time and not carried in the payload.
     */
    class dissector_Export Sample_Array : public Sample_Sequence
    {
    public:
      Sample_Array (size_t count, Sample_Dissector* sub);

    protected:
      virtual size_t dissect_i(Wireshark_Bundle& params);
      size_t count_;
    };

    /**
     * A specialized sample dissector for rendering Enumerations. An enum
     * is marshaled as a 4-byte value, but is rendered using a name. A chain
     * of sample fields is used hold the enumeration labels.
     *
     * TBD: determine if the number of enumerations can be determined before
     * inserting labels, allowing for an array of strings to be used rather
     * than a chain of fields.
     */
    class dissector_Export Sample_Enum : public Sample_Dissector
    {
    public:
      Sample_Enum();
      virtual ~Sample_Enum();

      Sample_Field* add_value(const std::string& val);
      bool index_of(const std::string& value, size_t& result);
      virtual std::string stringify(Wireshark_Bundle& p);
      virtual void init_ws_fields(bool first_pass);

    protected:
      virtual size_t dissect_i(Wireshark_Bundle& p);

      Sample_Field* value_;
    };

    /**
     * A specialized sample dissector for rendering Unions. A union is
     * marshaled as discriminator of some type, and a value of some type
     * determined by the discriminator.
     */
    class dissector_Export Sample_Union : public Sample_Dissector
    {
    public:
      Sample_Union ();
      virtual ~Sample_Union ();

      void discriminator(Sample_Dissector* d);
      void add_label(const std::string& label, Sample_Field* field);
      void add_default(Sample_Field* value);
      virtual void init_ws_fields(bool first_pass);

    protected:
      virtual size_t dissect_i(Wireshark_Bundle& p);

      Sample_Dissector* discriminator_;
      typedef std::map<std::string, Sample_Field*> MapType;
      MapType map_;
      Sample_Field* default_;
    };

    /**
     * A sample dissector for types that are aliases of other types
     */
    class dissector_Export Sample_Alias : public Sample_Dissector
    {
    public:
      Sample_Alias(Sample_Dissector* base);

      virtual void init_ws_fields(bool first_pass);

    protected:
      virtual size_t dissect_i(Wireshark_Bundle& p);

      Sample_Dissector* base_;
    };

    /**
     * A Dissector for Fixed Point Types (FACE/Fixed.h)
     */
    class dissector_Export Sample_Fixed : public Sample_Dissector
    {
    public:
      Sample_Fixed(unsigned digits, unsigned scale);

      unsigned digits() { return digits_; }
      unsigned scale() { return scale_; }

    protected:
      virtual void init_ws_fields(bool first_pass);
      virtual size_t dissect_i(Wireshark_Bundle &params);

    private:
      unsigned digits_, scale_;
    };
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
