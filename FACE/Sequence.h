#ifndef OPENDDS_FACE_SEQUENCE_HEADER
#define OPENDDS_FACE_SEQUENCE_HEADER

#include "FACE/types.hpp"
#include "FACE/StringManager.h"

#include "dds/DCPS/SafetyProfilePool.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/PoolAllocationBase.h"

#include <tao/Array_VarOut_T.h> // Array_Traits

#include <algorithm>
#include <memory>
#include <utility>
#include <cstddef>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace FaceTypes {

  typedef FACE::UnsignedLong seq_size_type;
  typedef FACE::Boolean seq_flag_type;

  template <seq_size_type N>
  struct Bounded {
    static const seq_size_type Bounds = N;
  };

  struct Unbounded {
    static const seq_size_type Bounds = INT_MAX;
  };

  template <typename T, typename Sequence, typename Bounds>
  struct AllocPolicy;

  template <typename T, typename Sequence, seq_size_type N>
  struct AllocPolicy<T, Sequence, Bounded<N> > {
    static T* allocbuf();
    seq_size_type maximum() const { return N; }
    seq_size_type max_size() const { return N; }
    void replace(seq_size_type length, T* data, seq_flag_type release = false)
    {
      static_cast<Sequence&>(*this).replace_i(N, length, data, release);
    }
  protected:
    explicit AllocPolicy(seq_size_type = N) {}
    T* allocate(seq_size_type = N) const { return allocbuf(); }
    void swap(AllocPolicy&) throw() {}
  };

  template <typename T, typename Sequence>
  struct AllocPolicy<T, Sequence, Unbounded> {
    static T* allocbuf(seq_size_type n);
    seq_size_type maximum() const { return maximum_; }
    seq_size_type max_size() const { return Unbounded::Bounds; }
    void replace(seq_size_type maximum, seq_size_type length,
                 T* data, seq_flag_type release = false)
    {
      static_cast<Sequence&>(*this).replace_i(maximum, length, data, release);
    }
  protected:
    explicit AllocPolicy(seq_size_type n = 0) : maximum_(n) {}
    T* allocate(seq_size_type request = 0) const
    {
      return allocbuf(request ? request : maximum_);
    }
    void swap(AllocPolicy& rhs) throw() { std::swap(maximum_, rhs.maximum_); }
    seq_size_type maximum_;
  };

  /// Element Policy for sequence elements that are IDL "fixed-length" types.
  /// These types don't need initialization or destruction of elements in their
  /// allocbuf()/freebuf() functions.
  /// @tparam T element type of the sequence
  template <typename T>
  struct DefaultEltPolicy {
    typedef T& Element;
    typedef const T& ConstElement;
    typedef T ConstRawElement;
    static const seq_size_type extra = 0;
    static T& make_element(T& elt, seq_flag_type) { return elt; }
    static void construct(T*, seq_size_type, seq_flag_type) {}
    static void copy_n(const T* input, seq_size_type n, T* output);
    static void move_n(T* in, seq_size_type n, T* out) { copy_n(in, n, out); }
    static void reset_n(T*, seq_size_type) {}
    static T* destroy(T* buffer, seq_size_type) { return buffer; }
  };

  /// Element Policy for sequence elements that are IDL "variable-length" types
  /// except for strings and arrays, which are handled separately.
  /// @tparam T element type of the sequence
  template <typename T>
  struct VariEltPolicy {
    typedef T& Element;
    typedef const T& ConstElement;
    typedef T ConstRawElement;
    static const seq_size_type extra = 1;
    static T& make_element(T& elt, seq_flag_type) { return elt; }
    static void construct(T* buffer, seq_size_type n, seq_flag_type cookie);
    static void copy_n(const T* input, seq_size_type n, T* output);
    static void move_n(T* in, seq_size_type n, T* out);
    static void reset_n(T* buffer, seq_size_type n);
    static T* destroy(T* buffer, seq_size_type n);
  };

  /// Element Policy for sequences of strings.
  /// @tparam CharT FACE::Char or FACE::WChar
  template <typename CharT>
  struct StringEltPolicy {

    /// Indexing a non-const string sequence yields an object of this class.
    /// This allows string memory management duing assignment.
    struct Element {
      Element(CharT*& element, seq_flag_type release)
        : element_(element), release_(release) {}

      Element(const Element& elt)
        : element_(elt.element_), release_(elt.release_) {}

      Element& operator=(const CharT* rhs)
      {
        String_var<CharT> tmp(rhs);
        return move_from(tmp);
      }

      Element& operator=(CharT* rhs)
      {
        String_var<CharT> tmp(rhs);
        return move_from(tmp);
      }

      Element& operator=(const String_var<CharT>& rhs)
      {
        String_var<CharT> tmp(rhs);
        return move_from(tmp);
      }

      Element& operator=(const StringManager<CharT>& rhs)
      {
        String_var<CharT> tmp(rhs);
        return move_from(tmp);
      }

      operator const CharT*() const { return element_; }
      const CharT* in() const { return element_; }
      CharT*& inout() { return element_; }

      String_out<CharT> out() const
      {
        if (release_) StringTraits<CharT>::free(element_);
        return element_;
      }

      CharT* _retn()
      {
        CharT* const tmp = element_;
        element_ = StringTraits<CharT>::empty();
        return tmp;
      }

    private:
      Element& move_from(String_var<CharT>& rhs)
      {
        if (release_) StringTraits<CharT>::free(element_);
        element_ = rhs._retn();
        return *this;
      }

      CharT*& element_;
      seq_flag_type release_;

      inline friend bool operator>>(DCPS::Serializer& ser, Element elt)
      {
        ser.read_string(elt.out(), StringTraits<CharT>::alloc,
          StringTraits<CharT>::free);
        return ser.good_bit();
      }
    };

    static Element make_element(CharT*& elt, seq_flag_type release)
    {
      return Element(elt, release);
    }

    typedef const CharT* ConstElement;
    typedef const CharT* ConstRawElement;
    static const seq_size_type extra = 1;
    static void construct(CharT** buf, seq_size_type n, seq_flag_type cookie);
    static void copy_n(const CharT* const* in, seq_size_type n, CharT** out);
    static void move_n(CharT** in, seq_size_type n, CharT** out);
    static void reset_n(CharT**, seq_size_type);
    static CharT** destroy(CharT** buffer, seq_size_type n);
  };

  /// Element Policy for sequences of arrays.
  /// Currently arrays of fixed-length and variable-length elements are both
  /// handled the same way, but optimizing the fixed-length element types could
  /// be done here (they don't need construction, destruction, or cookies).
  /// @tparam Forany the array's *_forany type generated by the IDL compiler
  template <typename Forany, typename T = typename Forany::_array_type>
  struct ArrayEltPolicy {
    typedef T& Element;
    typedef const T& ConstElement;
    typedef const T ConstRawElement;
    static const seq_size_type extra =
      (sizeof(seq_size_type) - 1) / sizeof(T) + 1;
    static T& make_element(T& elt, seq_flag_type) { return elt; }
    static void construct(T* buffer, seq_size_type n, seq_flag_type use_cookie);
    static void copy_n(const T* input, seq_size_type n, T* output);
    static void move_n(T* in, seq_size_type n, T* out) { copy_n(in, n, out); }
    static void reset_n(T* buffer, seq_size_type n);
    static T* destroy(T* buffer, seq_size_type n);
  };

  /// Generic base class for all IDL-defined sequences accepted by opendds_idl.
  /// Derived classes (generated by opendds_idl) need to provide the following
  /// methods to be compliant with the IDL-to-C++ specification:
  /// If bounded:
  /// - Constructors: default, copy, 3-arg
  /// If unbounded:
  /// - Constructors: default, copy, 1-arg (maximum), 4-arg
  /// Both bounded and unbounded:
  /// - Copy assignment
  /// - non-member swap(), while not in spec this is useful for copy assignment
  /// @tparam T element type of the sequence
  /// @tparam Bounds either Bounded<N> or Unbounded
  /// @tparam Elts element handling policy
  template <typename T, typename Bounds, typename Elts = DefaultEltPolicy<T> >
  class Sequence
    : public AllocPolicy<T, Sequence<T, Bounds, Elts>, Bounds>
    , public ::OpenDDS::DCPS::PoolAllocationBase {
  public:
    typedef seq_size_type size_type;  // from std C++ Container concept
    typedef seq_size_type _size_type; // from IDL-to-C++ specification
    typedef Elts ElementPolicy;

  protected:
    explicit Sequence(size_type maximum = 0, size_type length = 0,
                      T* data = 0, seq_flag_type release = false);
    Sequence(const Sequence& seq);
    ~Sequence();
    Sequence& operator=(const Sequence& seq);

    void swap(Sequence& rhs) throw();
  public:
    using AllocPolicy<T, Sequence, Bounds>::maximum;
    void length(size_type len);
    size_type length() const { return length_; }

    typedef typename Elts::Element Element;
    typedef typename Elts::ConstElement ConstElement;
    typedef typename Elts::ConstRawElement ConstRawElement;

    typedef ConstElement const_subscript_type; // sequence _var compatibility
    typedef Element subscript_type; // sequence _var compatibility

    ConstElement operator[](size_type idx) const;
    Element operator[](size_type idx);

    seq_flag_type release() const { return release_; }

    T* get_buffer(seq_flag_type orphan = false);
    const ConstRawElement* get_buffer() const;

    // allocbuf() inherited from AllocPolicy
    static void freebuf(T* data);


    // The public members below provide C++ standard library container
    // compatibility for convenience.
    // Iterators are always T* so be careful with string sequences,
    // the caller needs to use FACE::string_free() and FACE::string_alloc()
    // or FACE::string_dup() to replace a string in the sequence.
    // These are the same semantics as get_buffer(bool) in the IDL-to-C++
    // mapping.

    typedef T value_type;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef std::ptrdiff_t difference_type;

    const T* begin() const { return buffer_; }
    T* begin() { return buffer_; }

    const T* end() const { return buffer_ + length_; }
    T* end() { return buffer_ + length_; }

    bool operator==(const Sequence& rhs) const;
    bool operator!=(const Sequence& rhs) const;

    size_type size() const { return length_; }
    // max_size() inherited from AllocPolicy
    bool empty() const { return !length_; }

#ifndef __SUNPRO_CC
  private:
    friend struct AllocPolicy<T, Sequence, Bounds>;
#endif
    void replace_i(size_type maximum, size_type length,
                   T* data, seq_flag_type release);

  private:
    using AllocPolicy<T, Sequence, Bounds>::allocate;
    void lazy_alloc() const;

    size_type length_;
    mutable seq_flag_type release_;
    mutable T* buffer_;
  };


  // Allocation Policies:

  template <typename T, typename Sequence, seq_size_type N>
  inline T* AllocPolicy<T, Sequence, Bounded<N> >::allocbuf()
  {
    void* const raw =
      ACE_Allocator::instance()->malloc(N * sizeof(T));
    T* const mem = static_cast<T*>(raw);
    Sequence::ElementPolicy::construct(mem, N, false);
    return mem;
  }

  template <typename T, typename Sequence>
  inline T* AllocPolicy<T, Sequence, Unbounded>::allocbuf(seq_size_type n)
  {
    const size_t bytes = (n + Sequence::ElementPolicy::extra) * sizeof(T);
    void* const raw = ACE_Allocator::instance()->malloc(bytes);
    T* const mem = static_cast<T*>(raw);
    Sequence::ElementPolicy::construct(mem, n, true);
    return mem + Sequence::ElementPolicy::extra;
  }


  // Default Element Policy:

  template <typename T>
  inline void DefaultEltPolicy<T>::copy_n(const T* in, seq_size_type n, T* out)
  {
    std::memcpy(out, in, n * sizeof(T));
  }


  // String Element Policy:

  template <typename CharT>
  inline void StringEltPolicy<CharT>::construct(CharT** buffer, seq_size_type n,
                                                seq_flag_type use_cookie)
  {
    for (seq_size_type i = use_cookie; i < n + use_cookie; ++i) {
      buffer[i] = StringTraits<CharT>::empty();
    }
    if (use_cookie) {
      *reinterpret_cast<seq_size_type*>(buffer) = n;
    }
  }

  template <typename CharT>
  inline void StringEltPolicy<CharT>::copy_n(const CharT* const* in,
                                             seq_size_type n, CharT** out)
  {
    for (seq_size_type i = 0; i < n; ++i) {
      StringTraits<CharT>::free(out[i]);
      out[i] = StringTraits<CharT>::dup(in[i]);
    }
  }

  template <typename CharT>
  inline void StringEltPolicy<CharT>::move_n(CharT** in, seq_size_type n,
                                             CharT** out)
  {
    for (seq_size_type i = 0; i < n; ++i) {
      std::swap(in[i], out[i]);
    }
  }

  template <typename CharT>
  inline void StringEltPolicy<CharT>::reset_n(CharT** buffer, seq_size_type n)
  {
    for (seq_size_type i = 0; i < n; ++i) {
      StringTraits<CharT>::free(buffer[i]);
      buffer[i] = StringTraits<CharT>::empty();
    }
  }

  template <typename CharT>
  inline CharT** StringEltPolicy<CharT>::destroy(CharT** buffer,
                                                 seq_size_type n_or_int_max)
  {
    seq_size_type n = n_or_int_max;
    CharT** allocated = buffer;

    if (n_or_int_max == INT_MAX) {
      allocated = buffer - 1;
      n = *reinterpret_cast<seq_size_type*>(allocated);
    }

    for (seq_size_type i = 0; i < n; ++i) {
      StringTraits<CharT>::free(buffer[i]);
    }

    return allocated;
  }


  // Variable-length Element Policy:

  template <typename T>
  inline void VariEltPolicy<T>::construct(T* buffer, seq_size_type n,
                                          seq_flag_type use_cookie)
  {
    std::uninitialized_fill_n(buffer + use_cookie, n, T());
    if (use_cookie) {
      *reinterpret_cast<seq_size_type*>(buffer) = n;
    }
  }

  template <typename T>
  inline void VariEltPolicy<T>::copy_n(const T* in, seq_size_type n, T* out)
  {
    std::copy(in, in + n, out);
  }

  template <typename T>
  inline void VariEltPolicy<T>::move_n(T* in, seq_size_type n, T* out)
  {
    std::swap_ranges(in, in + n, out);
  }

  template <typename T>
  inline void VariEltPolicy<T>::reset_n(T* buffer, seq_size_type n)
  {
    std::fill_n(buffer, n, T());
  }

  template <typename T>
  inline T* VariEltPolicy<T>::destroy(T* buffer, seq_size_type n_or_int_max)
  {
    seq_size_type n = n_or_int_max;
    T* allocated = buffer;

    if (n_or_int_max == INT_MAX) {
      allocated = buffer - 1;
      n = *reinterpret_cast<seq_size_type*>(allocated);
    }

    for (seq_size_type i = 0; i < n; ++i) {
      buffer[i].~T();
    }

    return allocated;
  }


  // Array Element Policy:

  template <typename Forany, typename T>
  inline void ArrayEltPolicy<Forany, T>::construct(T* buffer,
                                                   seq_size_type n,
                                                   seq_flag_type use_cookie)
  {
    const seq_size_type start = use_cookie ? extra : 0;
    for (seq_size_type i = start; i < n + start; ++i) {
      TAO::Array_Traits<Forany>::construct(buffer[i]);
    }
    if (use_cookie) {
      *reinterpret_cast<seq_size_type*>(buffer) = n;
    }
  }

  template <typename Forany, typename T>
  inline void ArrayEltPolicy<Forany, T>::copy_n(const T* in, seq_size_type n,
                                                T* out)
  {
    for (seq_size_type i = 0; i < n; ++i) {
      TAO::Array_Traits<Forany>::copy(out[i], in[i]);
    }
  }

  template <typename Forany, typename T>
  inline void ArrayEltPolicy<Forany, T>::reset_n(T* buffer, seq_size_type n)
  {
    for (seq_size_type i = 0; i < n; ++i) {
      TAO::Array_Traits<Forany>::zero(buffer[i]);
    }
  }

  template <typename Forany, typename T>
  inline T* ArrayEltPolicy<Forany, T>::destroy(T* buffer, seq_size_type n)
  {
    seq_size_type num = n;
    T* alloc = buffer;

    if (n == INT_MAX) {
      alloc = buffer - extra;
      num = *reinterpret_cast<seq_size_type*>(alloc);
    }

    for (seq_size_type i = 0; i < num; ++i) {
      TAO::Array_Traits<Forany>::destroy(buffer[i]);
    }

    return alloc;
  }


  // Members of the Sequence template itself:

  template <typename T, typename Bounds, typename Elts>
  inline Sequence<T, Bounds, Elts>::Sequence(size_type maximum,
                                             size_type length,
                                             T* data, seq_flag_type release)
    : AllocPolicy<T, Sequence, Bounds>(maximum)
    , length_(length)
    , release_(release)
    , buffer_(data)
  {
  }

  template <typename T, typename Bounds, typename Elts>
  inline Sequence<T, Bounds, Elts>::Sequence(const Sequence& seq)
    : AllocPolicy<T, Sequence, Bounds>(seq.maximum())
    , length_(seq.length_)
    , release_(true)
    , buffer_((seq.maximum() && seq.buffer_) ? allocate() : 0)
  {
    if (buffer_) {
      Elts::copy_n(seq.buffer_, length_, buffer_);
    }
  }

  template <typename T, typename Bounds, typename Elts>
  inline Sequence<T, Bounds, Elts>::~Sequence()
  {
    if (release_) {
      freebuf(buffer_);
    }
  }

  template <typename T, typename Bounds, typename Elts>
  inline Sequence<T, Bounds, Elts>&
  Sequence<T, Bounds, Elts>::operator=(const Sequence& seq)
  {
    Sequence cpy(seq);
    swap(cpy);
    return *this;
  }

  template <typename T, typename Bounds, typename Elts>
  inline void Sequence<T, Bounds, Elts>::swap(Sequence& rhs) throw()
  {
    AllocPolicy<T, Sequence, Bounds>::swap(rhs);
    std::swap(length_, rhs.length_);
    std::swap(release_, rhs.release_);
    std::swap(buffer_, rhs.buffer_);
  }

  template <typename T, typename Bounds, typename Elts>
  inline void Sequence<T, Bounds, Elts>::replace_i(size_type maximum,
                                                   size_type length, T* data,
                                                   seq_flag_type release)
  {
    Sequence tmp(maximum, length, data, release);
    swap(tmp);
  }

  template <typename T, typename Bounds, typename Elts>
  inline void Sequence<T, Bounds, Elts>::lazy_alloc() const
  {
    if (!buffer_) {
      buffer_ = allocate();
      release_ = true;
    }
  }

  template <typename T, typename Bounds, typename Elts>
  inline void Sequence<T, Bounds, Elts>::length(size_type len)
  {
    if (len <= maximum()) {
      if (len && !buffer_) {
        lazy_alloc();
      }
      else if (release_ && len < length_) {
        Elts::reset_n(buffer_ + len, length_ - len);
      }
      length_ = len;
      return;
    }

    Sequence tmp(len, len, allocate(len), true);
    Elts::move_n(buffer_, length_, tmp.buffer_);
    swap(tmp);
  }

  template <typename T, typename Bounds, typename Elts>
  inline typename Sequence<T, Bounds, Elts>::ConstElement
  Sequence<T, Bounds, Elts>::operator[](size_type idx) const
  {
    return buffer_[idx];
  }

  template <typename T, typename Bounds, typename Elts>
  inline typename Sequence<T, Bounds, Elts>::Element
  Sequence<T, Bounds, Elts>::operator[](size_type idx)
  {
    return Elts::make_element(buffer_[idx], release_);
  }

  template <typename T, typename Bounds, typename Elts>
  inline T* Sequence<T, Bounds, Elts>::get_buffer(seq_flag_type orphan)
  {
    if (orphan && !release_) {
      return 0;
    }

    lazy_alloc();

    if (orphan) {
      Sequence tmp;
      swap(tmp);
      tmp.release_ = false;
      return tmp.buffer_;
    }

    return buffer_;
  }

  template <typename T, typename Bounds, typename Elts>
  inline const typename Sequence<T, Bounds, Elts>::ConstRawElement*
  Sequence<T, Bounds, Elts>::get_buffer() const
  {
    lazy_alloc();
    return buffer_;
  }

  template <typename T, typename Bounds, typename Elts>
  inline void Sequence<T, Bounds, Elts>::freebuf(T* data)
  {
    if (!data) return;
    T* const allocated = Elts::destroy(data, Bounds::Bounds);
    ACE_Allocator::instance()->free(allocated);
  }

  template <typename T, typename Bounds, typename Elts>
  inline bool Sequence<T, Bounds, Elts>::operator==(const Sequence& rhs) const
  {
    const size_type sz = size();
    if (sz != rhs.size()) {
      return false;
    }
    for (size_type i = 0; i < sz; ++i) {
      if (!((*this)[i] == rhs[i])) {
        return false;
      }
    }
    return true;
  }

  template <typename T, typename Bounds, typename Elts>
  inline bool Sequence<T, Bounds, Elts>::operator!=(const Sequence& rhs) const
  {
    return !(*this == rhs);
  }
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
