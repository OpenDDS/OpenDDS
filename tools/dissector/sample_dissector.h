// -*- C++ -*-
/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _SAMPLE_DISSECTOR_H_
#define _SAMPLE_DISSECTOR_H_


extern "C" {

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include <epan/value_string.h>
#include <epan/ipproto.h>
#include <epan/packet.h>
#include <epan/dissectors/packet-tcp.h>
  //#include <epan/dissectors/packet-giop.h>
} // extern "C"

#include "tools/dissector/dissector_export.h"

#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportHeader.h"
#include "ace/Hash_Map_Manager.h"

namespace OpenDDS
{
  namespace DCPS
  {

    struct Wireshark_Bundle
    {
      tvbuff_t *tvb;
      packet_info *info;
      proto_tree *tree;
      gint offset;

      guint8 *get_remainder ();
    };

    struct Wireshark_Bundle_i : public Wireshark_Bundle
    {
      bool use_index;
      guint32 index;
    };

    struct Wireshark_Bundle_Field : public Wireshark_Bundle_i
    {
      guint8 *data;
    };

    class dissector_Export Sample_Dissector;
    /*
     * Sample_Field decribes a single element of the sample. This can be
     * a fixed length field, such as a short, long, etc. or variable length
     * such as a string, enum, union, sequence, or another struct.
     *
     * Fields are chained together to describe the layout of the sample in
     * the data buffer
     */

    class Sample_Field
    {
    public:

      enum FixedTypeID {
        Char,
        Bool,
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
        Enumeration,
        Undefined
      };

      /// @ctor for fixed length fields, supply a type identifier and a label
      Sample_Field (FixedTypeID type_id, const char *label);

      /// @ctor for complex fields, supply a nested Sample_Dissector to handle
      /// the dissection. The Sample_Field takes ownership of the nested
      /// sample base value.
      Sample_Field (Sample_Dissector *n, const char *label);

      ~Sample_Field ();

      /// Add a new field to the chain of fields describing the sample.
      /// returns the argument pointer to facilitate iterative construction
      /// of chains
      Sample_Field *chain (Sample_Field *n);
      Sample_Field *chain (FixedTypeID ti, const char *l);
      Sample_Field *chain (Sample_Dissector *n, const char *l);

      /// Iterate over the list to return the n'th link returns null if n
      /// exceeds the number of links in the chain
      Sample_Field *get_link (size_t n);

      /// Add the field to the tree, either directly using the configured
      /// format and supplied data pointer, or by handing off to the attached
      /// Sample_Dissector object for further evaluation.
      size_t dissect_i (Wireshark_Bundle_Field &params);

      size_t compute_length (guint8 *data);

      /// Fixed length fields supply a label for identifying the field along
      /// with a type identifier. Fields that are members of an array or
      /// sequence will have an index value adjusted as the owning dissector
      /// iterates over the members.
      const char   *label_;
      FixedTypeID   type_id_;

      /// Variable length fields use a nested sample base value to do their
      /// dissection. This can be another configured sample_dissector instance,
      /// or it can be a string, enum, union, or sequence dissector.
      Sample_Dissector  *nested_;

      /// A simple linked-list is used to chain fields together. When the
      /// top-level field is deleted, it will iterate over this list deleting
      /// each subsequent field.
      Sample_Field *next_;
    };

    /*
     * A Sample_Dissector is the base type dissector for samples. A sample base
     * instance is initialized with a list of fields that is used to walk
     * through a data buffer to compose a tree of named values.
     *
     * Sample_Dissectors register with a manager that associate an identifier
     * with an instance of a sample base. A base could conceivable be
     * registered with more than one name to allow for aliases, but that
     * would require reference counting, which currently isn't done.
     */

    class dissector_Export Sample_Dissector
    {
    public:
      Sample_Dissector ();
      virtual ~Sample_Dissector ();

      /// Initialize the instance with a type_id used for registration
      /// and a label to be used to identify a sub-tree for presentation.
      /// The type_id may be empty in the case where only a subtree needs
      /// to be defined for complex samples that are not otherwise top-level
      /// types.
      virtual void init (const char *type_id,
                         const char *subtree_label = "");

      /// Dissect is the hook function called from an owning dissector.
      /// The signature matches that required by wireshark. The offset
      /// is incremented as a side-effect of calling dissect. The real
      /// work of dissecting a sample is delegated to dissect_i.
      gint dissect (Wireshark_Bundle &params);

      virtual size_t compute_length (guint8 *data);

      /// Accessor to the typeId value. Used for registration.
      const char *typeId() const;

      /// Adds a new sample field to the list. The Sample_Dissector instance
      /// takes ownership of this field instance. As a pass-thru to
      /// Sample_Field::chain(), it returns the supplied field pointer
      /// to facilitate simple chain construction.
      Sample_Field *add_field (Sample_Field *field);
      Sample_Field *add_field (Sample_Field::FixedTypeID, const char *l);
      Sample_Field *add_field (Sample_Dissector *n, const char *l);

      /// The actual dissector metheod. Since a sample can be composed of
      /// complex fields which need to do their own dissection, this method
      /// does not directly modify the supplied offset value, rather it
      /// returns the actual number of octets consumed for this dissection,
      /// however complicated it may be.
      virtual size_t dissect_i (Wireshark_Bundle_i &p, const char *l);

    protected:
      /// Values used by the wireshark framework for rendering a sub-tree.
      /// These get set in init(), a subtree is defined only if label_ is not
      /// empty.
      gint ett_payload_;
      char *subtree_label_;

    private:
      char *typeId_;

      /// The list of fields defining the sample.
      Sample_Field *field_;

    };

    /*
     * A specialized sample dissector for rendering strings. Strings are
     * composed of a 4-byte length followed by that number of characters
     * and no terminating 0. Sample_String instances should only be used
     * as a field in some other sample.
     */
    class dissector_Export Sample_String : public Sample_Dissector
    {
    public:
      /// *ctor. The supplied label is used as the field identifier in the
      /// dissector, which uses "%s: %s" as the format for this string. The
      /// label supplied here will be printed as the first part, and the
      /// actual data string as the second part.
      Sample_String ();

      virtual size_t compute_length (guint8 *data);


    protected:
      virtual size_t dissect_i (Wireshark_Bundle_i &p, const char *l);

    };

    class dissector_Export Sample_WString : public Sample_Dissector
    {
    public:
      /// @ctor. The supplied label is used as the field identifier in the
      /// dissector, which uses "%s: %ls" as the format for this string. The
      /// label supplied here will be printed as the first part, and the
      /// actual data string as the second part.
      Sample_WString ();

      virtual size_t compute_length (guint8 *data);

    protected:
      virtual size_t dissect_i (Wireshark_Bundle_i &p, const char *l);

    };

    /*
     * A specialized sample dissector for rendering sequences. Sequences
     * may contain any other type of field, which are rendered separately.
     */
    class dissector_Export Sample_Sequence : public Sample_Dissector
    {
    public:
      /// @ctor. Creates an empty base element to be initialized after this
      /// sample sequence holder is created
      Sample_Sequence (Sample_Field *field = 0);
      Sample_Sequence (Sample_Dissector *sub);
      Sample_Sequence (Sample_Field::FixedTypeID type_id);
      ~Sample_Sequence ();

      Sample_Dissector *element();

      virtual size_t compute_length (guint8 *data);

    protected:
      virtual size_t dissect_i (Wireshark_Bundle_i &p, const char *l);

      Sample_Dissector *element_;
    };

    /*
     * A specialized sample dissector for rendering Arrays. Arrays may
     * contain any other type of field, which are rendered separately.
     *
     * Arrays differ from sequences in that the size is fixed at compile
     * time and not carried in the payload.
     */
    class dissector_Export Sample_Array : public Sample_Sequence
    {
    public:
      Sample_Array (size_t count, Sample_Field *field = 0);
      Sample_Array (size_t count, Sample_Dissector *sub);
      Sample_Array (size_t count, Sample_Field::FixedTypeID type_id);

      virtual size_t compute_length (guint8 *data);

    protected:
      virtual size_t dissect_i (Wireshark_Bundle_i &p, const char *l);
      size_t count_;
    };

    /*
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
      Sample_Enum ();
      ~Sample_Enum ();

      Sample_Field *add_value (const char *val);

      virtual size_t compute_length (guint8 *data);


    protected:
      virtual size_t dissect_i (Wireshark_Bundle_i &p, const char *l);

      Sample_Field *value_;
    };

    /*
     * A specialized sample dissector for rendering Unions. A union is
     * marshaled as discriminator of some type, and a value of some type
     * determined by the discriminator.
     *
     * TBD: have to figure out how to evaluate the descriminator to select
     * the appropriate field instance. Since each discriminant field may
     * itself be complex, they have to be held in separate chains.
     */
    class dissector_Export Sample_Union : public Sample_Dissector
    {
    public:
      Sample_Union ();
      ~Sample_Union ();

      void discriminator (Sample_Dissector *d);
      Sample_Field *add_case (Sample_Field *key, Sample_Field *value);
      Sample_Field *add_case_range (Sample_Field *key_low,
                                    Sample_Field *key_high,
                                    Sample_Field *value);
      void add_default (Sample_Field *value);

      virtual size_t compute_length (guint8 *data);

    protected:
      virtual size_t dissect_i (Wireshark_Bundle_i &p, const char *l);

      Sample_Dissector *_d;
      Sample_Field *case_;
    };

    /*
     * A sample dissector for types that are aliases of other types
     */

    class dissector_Export Sample_Alias : public Sample_Dissector
    {
    };

  }
}

#endif //  _SAMPLE_DISSECTOR_H_
