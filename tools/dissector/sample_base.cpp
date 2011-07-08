/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tools/dissector/sample_base.h"

#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/Message_Block.h>
#include <ace/Log_Msg.h>
#include <ace/ACE.h>

#include <cstring>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace OpenDDS
{
  namespace DCPS
  {

    Sample_Dissector_Manager
    Sample_Dissector_Manager::instance_;

    Sample_Dissector_Manager &
    Sample_Dissector_Manager::instance ()
    {
      return instance_;
    }

    void
    Sample_Dissector_Manager::init ()
    {
      Sample_Base *dummy = new LocationInfo_Dissector;
      dummy = new PlanInfo_Dissector;
      dummy = new MoreInfo_Dissector;
      dummy = new UnrelatedInfo_Dissector;
      dummy = new Resulting_Dissector;

      dummy = new Message_Dissector;
      dummy = new Message2_Dissector;

      ACE_UNUSED_ARG (dummy);
    };

    void
    Sample_Dissector_Manager::add (Sample_Base &d)
    {
      const char *key = d.typeId();
      ACE_DEBUG ((LM_DEBUG,"Adding new dissector for %s\n",key));

      dissectors_.bind(key,&d);
    }

    Sample_Base *
    Sample_Dissector_Manager::find (const char *data_name)
    {
      Sample_Base *result = 0;
      dissectors_.find (data_name, result);
      return result;
    }

    //--------------------------------------------------------------------


    Sample_Field::Sample_Field (const char *f, gint l)
      : format_(f),
        len_(l),
        nested_ (0),
        next_(0)
    {
    }

    Sample_Field::Sample_Field (Sample_Base *n)
      : format_(""),
        len_(0),
        nested_ (n),
        next_(0)
    {
    }

    Sample_Field::~Sample_Field ()
    {
      delete nested_;
      delete next_;
    }

    Sample_Field *
    Sample_Field::chain (Sample_Field *f)
    {
      if (next_ != 0)
        next_->chain (f);
      else
        next_ = f;
      return f;
    }

    Sample_Field *
    Sample_Field::get_link (size_t index)
    {
      if (index == 0)
        return this;

      return (next_ == 0) ? 0 : next_->get_link (index - 1);
    }

    size_t
    Sample_Field::dissect_i (tvbuff_t *tvb, packet_info* pinfo,
                             proto_tree *tree,
                             gint offset, guint8 *data)
    {
      size_t len = 0;
      if (nested_ == 0)
        {
          proto_tree_add_text (tree, tvb, offset, len_, format_, *(data));
          len = this->len_;
        }
      else
        {
          len = this->nested_->dissect_i (tvb, pinfo, tree, offset);
        }

      offset += len;
      data += len;
      if (next_ != 0)
        len += next_->dissect_i (tvb, pinfo, tree, offset, data);
      return len;
    }

    //------------------------------------------------------------------------

    Sample_Base::Sample_Base ()
      :ett_payload_ (-1),
       label_ (0),
       typeId_ (0),
       field_ (0)
    {
    }

    Sample_Base::~Sample_Base ()
    {
      delete [] this->typeId_;
      delete field_;
    }

    const char *
    Sample_Base::typeId() const
    {
      return this->typeId_;
    }

    void
    Sample_Base::init (const char *type_id,
                       const char *subtree_label)
    {
      size_t len = ACE_OS::strlen(type_id);
      if (len > 0)
        {
          this->typeId_ = new char[len+1];
          ACE_OS::strcpy (this->typeId_, type_id);
          Sample_Dissector_Manager::instance().add (*this);
        }

      len = ACE_OS::strlen(subtree_label);
      if (len > 0)
        {
          this->label_ = new char[len+1];
          ACE_OS::strcpy (this->label_, subtree_label);

          gint *ett[] = {
            &ett_payload_
          };
          proto_register_subtree_array(ett, array_length(ett));
        }
    }

    Sample_Field *
    Sample_Base::add_field (Sample_Field *f)
    {
      if (this->field_ == 0)
        {
          this->field_ = f;
        }
      else
        {
          this->field_->chain (f);
        }
      return f;
    }

    size_t
    Sample_Base::dissect_i (tvbuff_t *tvb,
                            packet_info *pinfo,
                            proto_tree *tree,
                            gint offset)
    {
      size_t data_pos = 0;

      gint remainder = tvb->length - offset;
      guint8* data = (guint8*)ep_tvb_memdup(tvb, offset, remainder);

      if (this->label_ != 0)
        {
          proto_item *item =
            proto_tree_add_text (tree, tvb, offset, -1,
                                 "Sample Payload: %s", label_);
          tree = proto_item_add_subtree (item, ett_payload_);
        }

      if (field_ != 0)
        data_pos += field_->dissect_i (tvb, pinfo, tree, offset, data);

      return data_pos;
    }

    void
    Sample_Base::dissect (tvbuff_t *tvb,
                          packet_info *pinfo,
                          proto_tree *tree,
                          gint &offset)
    {
      offset += this->dissect_i (tvb, pinfo, tree, offset);
    }

    //----------------------------------------------------------------------

    Sample_String::Sample_String (const char *label)
    {
      size_t len = label != 0 ? ACE_OS::strlen(label) : 0;
      this->label_ = new char[len+1];
      if (len > 0)
        ACE_OS::strcpy (this->label_, label);
    }

    size_t
    Sample_String::dissect_i (tvbuff_t *tvb,
                              packet_info *,
                              proto_tree *tree,
                              gint offset)
    {
      gint remainder = tvb->length - offset;
      char * data = (char *)ep_tvb_memdup(tvb, offset, remainder);
      guint32 len = *(reinterpret_cast< guint32 * >(data));

      char * clone = new char[len + 1];
      ACE_OS::memcpy (clone, data+4, len);
      clone[len] = '\0';
      proto_tree_add_text (tree, tvb, offset, (len) + 4,
                           "%s: %s", this->label_, clone);
      delete [] clone;

      return len + 4;
    }

    //----------------------------------------------------------------------

    Sample_WString::Sample_WString (const char *label)
    {
      size_t len = label != 0 ? ACE_OS::strlen(label) : 0;
      this->label_ = new char[len+1];
      if (len > 0)
        ACE_OS::strcpy (this->label_, label);
    }

    size_t
    Sample_WString::dissect_i (tvbuff_t *tvb,
                               packet_info *,
                               proto_tree *tree,
                               gint offset)
    {
      gint remainder = tvb->length - offset;
      char * data = (char *)ep_tvb_memdup(tvb, offset, remainder);
      guint32 len = *(reinterpret_cast< guint32 * >(data)) * sizeof (wchar_t);

      wchar_t * clone = new wchar_t[len + 1];
      ACE_OS::memcpy (clone, data+4, len);
      clone[len] = '\0';
      proto_tree_add_text (tree, tvb, offset, (len) + 4,
                           "%s: %ls", this->label_, clone);
      delete [] clone;

      return len + 4;
    }

    //----------------------------------------------------------------------

    Sample_Sequence::Sample_Sequence (const char *label)
      : element_ (0)
    {
      this->init ("",label);
      element_ = new Sample_Base;
    }

    Sample_Sequence::~Sample_Sequence ()
    {
      delete element_;
    }

    Sample_Base *
    Sample_Sequence::element ()
    {
      return element_;
    }

    size_t
    Sample_Sequence::dissect_i (tvbuff_t *tvb,
                                packet_info *pinfo,
                                proto_tree *tree,
                                gint offset)
    {
      gint remainder = tvb->length - offset;
      char * data = (char *)ep_tvb_memdup(tvb, offset, remainder);
      guint32 count = *(reinterpret_cast< guint32 * >(data));

      if (this->label_ != 0)
        {
          proto_item *item =
            proto_tree_add_text (tree, tvb, offset, -1,
                                 "%s [%d]", label_, count);
          tree = proto_item_add_subtree (item, ett_payload_);
        }

      size_t data_pos = 4;
      for (guint32 i = 0; i < count; i++)
        {
          data_pos += element_->dissect_i (tvb, pinfo, tree,
                                           offset + data_pos);
        }
      return data_pos;
    }

    //----------------------------------------------------------------------

    Sample_Array::Sample_Array (const char *label,size_t count)
      : Sample_Sequence (label),
        count_(count)
    {
    }

    size_t
    Sample_Array::dissect_i (tvbuff_t *tvb,
                             packet_info *pinfo,
                             proto_tree *tree,
                             gint offset)
    {
      if (this->label_ != 0)
        {
          proto_item *item =
            proto_tree_add_text (tree, tvb, offset, -1, "%s", label_);
          tree = proto_item_add_subtree (item, ett_payload_);
        }

      size_t data_pos = 0;
      for (size_t i = 0; i < count_; i++)
        {
          data_pos += element_->dissect_i (tvb, pinfo, tree,
                                           offset + data_pos );
        }
      return data_pos;
    }

    //----------------------------------------------------------------------

    Sample_Enum::Sample_Enum (const char *label)
      : name_ (0)
    {
      size_t len = ACE_OS::strlen(label);
      if (len > 0)
        {
          this->label_ = new char[len+1];
          ACE_OS::strcpy (this->label_, label);
        }
    }

    Sample_Enum::~Sample_Enum ()
    {
      delete name_;
    }

    Sample_Field *
    Sample_Enum::add_name (const char *name)
    {
      return (name_ == 0) ?
        (name_ = new Sample_Field (name, 4)) :
        name_->chain (new Sample_Field (name, 4));
    }

    size_t
    Sample_Enum::dissect_i (tvbuff_t *tvb,
                            packet_info *,
                            proto_tree *tree,
                            gint offset)
    {
      size_t len = 4;
      char * data = (char *)ep_tvb_memdup(tvb, offset, len);
      guint32 value = *(reinterpret_cast< guint32 * >(data));

      Sample_Field *sf = name_->get_link (value);

      proto_tree_add_text (tree, tvb, offset, len,
                           "%s: %s", label_, sf->format_);
      return len;
    }

    //----------------------------------------------------------------------

    LocationInfo_Dissector::LocationInfo_Dissector ()
    {

      this->init ("IDL:LocationInfoTypeSupport:1.0",
                  "LocationInfo");
#if 0
      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field ("flight_id2: %u", 4));
      this->add_field (new Sample_Field ("x: %d", 4));
      this->add_field (new Sample_Field ("y: %d", 4));
      this->add_field (new Sample_Field ("z: %d", 4));
#else
      Sample_Field *f =
        this->add_field (new Sample_Field ("flight_id1: %u", 4));
      f = f->chain (new Sample_Field ("flight_id2: %u", 4));
      f = f->chain (new Sample_Field ("x: %d", 4));
      f = f->chain (new Sample_Field ("y: %d", 4));
      f = f->chain (new Sample_Field ("z: %d", 4));
#endif

    }

    PlanInfo_Dissector::PlanInfo_Dissector ()
    {
      this->init ("IDL:PlanInfoTypeSupport:1.0",
                  "PlanInfo");

      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field ("flight_id2: %u", 4));
      this->add_field (new Sample_Field (new Sample_String ("flight_name")));
      this->add_field (new Sample_Field (new Sample_String ("tailno")));
    }

    MoreInfo_Dissector::MoreInfo_Dissector ()
    {

      this->init ("IDL:MoreInfoTypeSupport:1.0",
                  "MoreInfo");

      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field (new Sample_String ("more")));
    }

    UnrelatedInfo_Dissector::UnrelatedInfo_Dissector ()
    {


      this->init ("IDL:UnrelatedInfoTypeSupport:1.0",
                  "UnrelatedInfo");

     this->add_field (new Sample_Field (new Sample_String ("misc")));

    }

    Resulting_Dissector::Resulting_Dissector ()
    {
      this->init ("IDL:ResultingTypeSupport:1.0",
                  "Resulting");

      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field ("flight_id2: %u", 4));
      this->add_field (new Sample_Field (new Sample_String ("flight_name")));
      this->add_field (new Sample_Field ("x: %d", 4));
      this->add_field (new Sample_Field ("y: %d", 4));
      this->add_field (new Sample_Field ("height: %d", 4));
      this->add_field (new Sample_Field (new Sample_String ("more")));
      this->add_field (new Sample_Field (new Sample_String ("misc")));

    }

    //---------------------------------------------------------------------


    Message_Dissector::Message_Dissector ()
    {
      this->init ("IDL:Messenger/MessageTypeSupport:1.0",
                  "Messenger::Message");
//   struct Message {
//     string from;
//     string subject;
//     long subject_id;
//     string text;
//     long   count;
//     CORBA::BooleanSeq    bool_seq;
//     CORBA::LongDoubleSeq longdouble_seq;
//     CORBA::ShortSeq      short_seq;
//     CORBA::UShortSeq     ushort_seq;
//     CORBA::CharSeq       char_seq;
//     CORBA::LongLongSeq   longlong_seq;
//     CORBA::StringSeq     string_seq;
//     CORBA::WCharSeq      wchar_seq;
//     CORBA::DoubleSeq     double_seq;
//     CORBA::LongSeq       long_seq;
//     CORBA::ULongLongSeq  ulonglong_seq;
//     CORBA::WStringSeq    wstring_seq;
//     CORBA::FloatSeq      float_seq;
//     CORBA::OctetSeq      octet_seq;
//     CORBA::ULongSeq      ulong_seq;
//     Messenger2::LongSeq  outside_long_seq;
//   };
      Sample_Field *f =
        this->add_field (new Sample_Field (new Sample_String ("from")));
      f = f->chain (new Sample_Field (new Sample_String ("subject")));
      f = f->chain (new Sample_Field ("subject_id: %d", 4));
      f = f->chain (new Sample_Field (new Sample_String ("text")));
      f = f->chain (new Sample_Field ("count: %d", 4));

      Sample_Sequence *seq = new Sample_Sequence("bool_seq");
      seq->element()->add_field (new Sample_Field ("%u", sizeof (bool)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("longdouble_seq");
      seq->element()->add_field (new Sample_Field ("%24.16LG", sizeof (long double)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("short_seq");
      seq->element()->add_field (new Sample_Field ("%d", sizeof (short)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("ushort_seq");
      seq->element()->add_field (new Sample_Field ("%u", sizeof (short)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("char_seq");
      seq->element()->add_field (new Sample_Field ("%c", sizeof (char)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("longlong_seq");
      seq->element()->add_field (new Sample_Field ("%lld", sizeof (long long)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("string_seq");
      seq->element()->add_field (new Sample_Field (new Sample_String ("")));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("wchar_seq");
      seq->element()->add_field (new Sample_Field ("%lc", sizeof (wchar_t)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("double_seq");
      seq->element()->add_field (new Sample_Field ("%g", sizeof (double)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("long_seq");
      seq->element()->add_field (new Sample_Field ("%ld", sizeof (long)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("ulonglong_seq");
      seq->element()->add_field (new Sample_Field ("%llu", sizeof (long long)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("wstring_seq");
      seq->element()->add_field (new Sample_Field (new Sample_WString ("")));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("float_seq");
      seq->element()->add_field (new Sample_Field ("%f", sizeof (float)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("octet_seq");
      seq->element()->add_field (new Sample_Field ("%x", sizeof (char)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("ulong_seq");
      seq->element()->add_field (new Sample_Field ("%lu", sizeof (long)));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("outside_long_seq");
      seq->element()->add_field (new Sample_Field ("%ld", sizeof (long)));
      f = f->chain (new Sample_Field (seq));
    }

    Message2_Dissector::Message2_Dissector ()
    {

      this->init ("IDL:Messenger/MessageTypeSupport:1.0",
                  "Messenger::Message");


//     typedef sequence <long> LongSeq;

//   struct Message2 {
//     string from;
//     string subject;
//     long subject_id;
//     string text;
//     long   count;
//     CORBA::BooleanSeq    bool_seq;
//     CORBA::LongDoubleSeq longdouble_seq;
//     CORBA::ShortSeq      short_seq;
//     CORBA::UShortSeq     ushort_seq;
//     CORBA::CharSeq       char_seq;
//     CORBA::LongLongSeq   longlong_seq;
//     CORBA::StringSeq     string_seq;
//     CORBA::WCharSeq      wchar_seq;
//     CORBA::DoubleSeq     double_seq;
//     CORBA::LongSeq       long_seq;
//     CORBA::ULongLongSeq  ulonglong_seq;
//     CORBA::WStringSeq    wstring_seq;
//     CORBA::FloatSeq      float_seq;
//     CORBA::OctetSeq      octet_seq;
//     CORBA::ULongSeq      ulong_seq;
//   };

      Sample_Field *f =
        this->add_field (new Sample_Field (new Sample_String ("from")));
      f = f->chain (new Sample_Field (new Sample_String ("subject")));
      f = f->chain (new Sample_Field ("subject_id: %d", 4));
      f = f->chain (new Sample_Field (new Sample_String ("text")));
      f = f->chain (new Sample_Field ("count: %d", 4));

      Sample_Sequence *seq = new Sample_Sequence("bool_seq");
      seq->element()->add_field (new Sample_Field ("%b", 1));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("longdouble_seq");
      seq->element()->add_field (new Sample_Field ("%24.16LG", 16));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("short_seq");
      seq->element()->add_field (new Sample_Field ("%d", 2));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("ushort_seq");
      seq->element()->add_field (new Sample_Field ("%u", 2));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("char_seq");
      seq->element()->add_field (new Sample_Field ("%c", 1));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("longlong_seq");
      seq->element()->add_field (new Sample_Field ("%lld", 8));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("string_seq");
      seq->element()->add_field (new Sample_Field (new Sample_String ("")));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("wchar_seq");
      seq->element()->add_field (new Sample_Field ("%lc", 2));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("double_seq");
      seq->element()->add_field (new Sample_Field ("%g", 8));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("long_seq");
      seq->element()->add_field (new Sample_Field ("%ld", 4));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("ulonglong_seq");
      seq->element()->add_field (new Sample_Field ("%llu", 8));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("wstring_seq");
      seq->element()->add_field (new Sample_Field (new Sample_WString ("")));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("float_seq");
      seq->element()->add_field (new Sample_Field ("%f", 4));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("octet_seq");
      seq->element()->add_field (new Sample_Field ("%x", 1));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("ulong_seq");
      seq->element()->add_field (new Sample_Field ("%lu", 4));
      f = f->chain (new Sample_Field (seq));

      seq = new Sample_Sequence("outside_long_seq");
      seq->element()->add_field (new Sample_Field ("%ld", 4));
      f = f->chain (new Sample_Field (seq));
    }

  }
}
