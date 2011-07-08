// -*- C++ -*-
/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _SAMPLE_BASE_H_
#define _SAMPLE_BASE_H_


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
    class dissector_Export Sample_Base;

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
      /// @ctor for fixed length fields, supply a format string and a data
      /// length value
      Sample_Field (const char *f, gint l);

      /// @ctor for complex fields, supply a nested Sample_Base to handle
      /// the dissection. The Sample_Field takes ownership of the nested
      /// sample base value.
      Sample_Field (Sample_Base *n);

      ~Sample_Field ();

      /// Add a new field to the chain of fields describing the sample.
      /// returns the argument pointer to facilitate iterative construction
      /// of chains
      Sample_Field *chain (Sample_Field *n);

      /// Iterate over the list to return the n'th link returns null if n
      /// exceeds the number of links in the chain
      Sample_Field *get_link (size_t n);

      /// Add the field to the tree, either directly using the configured
      /// format and supplied data pointer, or by handing off to the attached
      /// Sample_Base object for further evaluation.
      size_t dissect_i (tvbuff_t *,
                        packet_info *,
                        proto_tree *,
                        gint,
                        guint8 *);

      /// Fixed length fields supply a printf style format string, typically
      /// of the form "name: %x" where x is the appropriate format identifer
      /// for the native type. The default sample_base dissector uses only a
      /// a single data value to be formatted.
      const char   *format_;

      /// Fixed length fields need to supply their actual length for buffer
      /// display and offset computation.
      guint         len_;

      /// Variable length fields use a nested sample base value to do their
      /// dissection. This can be another configured sample_base instance,
      /// or it can be a string, enum, union, or sequence dissector.
      Sample_Base  *nested_;

      /// A simple linked-list is used to chain fields together. When the
      /// top-level field is deleted, it will iterate over this list deleting
      /// each subsequent field.
      Sample_Field *next_;
    };

    /*
     * A Sample_Base is the base type dissector for samples. A sample base
     * instance is initialized with a list of fields that is used to walk
     * through a data buffer to compose a tree of named values.
     *
     * Sample_Bases register with a manager that associate an identifier
     * with an instance of a sample base. A base could conceivable be
     * registered with more than one name to allow for aliases, but that
     * would require reference counting, which currently isn't done.
     */

    class dissector_Export Sample_Base
    {
    public:
      Sample_Base ();
      virtual ~Sample_Base ();

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
      void dissect (tvbuff_t *buffer,
                    packet_info *info,
                    proto_tree *tree,
                    gint& offset);

      /// Accessor to the typeId value. Used for registration.
      const char * typeId() const;

      /// Adds a new sample field to the list. The Sample_Base instance
      /// takes ownership of this field instance. As a pass-thru to
      /// Sample_Field::chain(), it returns the supplied field pointer
      /// to facilitate simple chain construction.
      Sample_Field* add_field (Sample_Field *field);

      /// The actual dissector metheod. Since a sample can be composed of
      /// complex fields which need to do their own dissection, this method
      /// does not directly modify the supplied offset value, rather it
      /// returns the actual number of octets consumed for this dissection,
      /// however complicated it may be.
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);

    protected:
      /// Values used by the wireshark framework for rendering a sub-tree.
      /// These get set in init(), a subtree is defined only if label_ is not
      /// empty.
      gint ett_payload_;
      char *label_;

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
    class dissector_Export Sample_String : public Sample_Base
    {
    public:
      /// *ctor. The supplied label is used as the field identifier in the
      /// dissector, which uses "%s: %s" as the format for this string. The
      /// label supplied here will be printed as the first part, and the
      /// actual data string as the second part.
      Sample_String (const char * label);

    protected:
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);

    };

    class dissector_Export Sample_WString : public Sample_Base
    {
    public:
      /// @ctor. The supplied label is used as the field identifier in the
      /// dissector, which uses "%s: %ls" as the format for this string. The
      /// label supplied here will be printed as the first part, and the
      /// actual data string as the second part.
      Sample_WString (const char * label);

    protected:
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);

    };

    /*
     * A specialized sample dissector for rendering sequences. Sequences
     * may contain any other type of field, which are rendered separately.
     */
    class dissector_Export Sample_Sequence : public Sample_Base
    {
    public:
      /// @ctor. Creates an empty base element to be initialized after this
      /// sample sequence holder is created
      Sample_Sequence (const char *label);
      ~Sample_Sequence ();

      Sample_Base *element();

    protected:
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);

      Sample_Base *element_;
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
      Sample_Array (const char *label, size_t count);

    protected:
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);
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
    class dissector_Export Sample_Enum : public Sample_Base
    {
    public:
      Sample_Enum (const char *label);
      ~Sample_Enum ();

      Sample_Field *add_name (const char *name);

    protected:
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);

      Sample_Field *name_;
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
    class dissector_Export Sample_Union : public Sample_Base
    {

    };


    //------------------------------------------------------------------
    typedef ACE_Hash_Map_Manager <const char *, Sample_Base *, ACE_Null_Mutex> SampleDissectorMap;

    /*
     * The Sample_Dessector_Manager is a singleton which contains a hash map
     * of Sample Base instances keyed to type identifiers. This singleton is
     * used by the greater packet-opendds dissector to find type-specific
     * dissectors.
     */
    class dissector_Export Sample_Dissector_Manager
    {
    public:
      static Sample_Dissector_Manager &instance();
      void init ();

      void add (Sample_Base &dissector);
      Sample_Base *find (const char *data_name);

    private:
      static Sample_Dissector_Manager instance_;
      SampleDissectorMap dissectors_;

    };

    //-------------------------------------------------------
    // Temporary type-specific dissectors for testing

    class dissector_Export LocationInfo_Dissector : public Sample_Base
    {
    public:
      LocationInfo_Dissector ();
    };

    class dissector_Export PlanInfo_Dissector : public Sample_Base
    {
    public:
      PlanInfo_Dissector ();
    };

    class dissector_Export MoreInfo_Dissector : public Sample_Base
    {
    public:
      MoreInfo_Dissector ();
    };

    class dissector_Export UnrelatedInfo_Dissector : public Sample_Base
    {
    public:
      UnrelatedInfo_Dissector ();
    };

    class dissector_Export Resulting_Dissector : public Sample_Base
    {
    public:
      Resulting_Dissector ();
    };

    class dissector_Export Message_Dissector : public Sample_Base
    {
    public:
      Message_Dissector ();
    };

    class dissector_Export Message2_Dissector : public Sample_Base
    {
    public:
      Message2_Dissector ();
    };

  }
}

#endif //  _SAMPLE_BASE_H_
