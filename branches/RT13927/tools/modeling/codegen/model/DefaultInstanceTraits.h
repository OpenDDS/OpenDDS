#ifndef DEFAULTINSTANCETRAITS_H
#define DEFAULTINSTANCETRAITS_H

namespace OpenDDS { namespace Model {

  /// Define default values and behaviors so instance traits only need to
  /// implement those that are different from the default.
  struct DefaultInstanceTraits {
    /// Transport key values in the model are relative to this base.
    enum { transport_key_base = 0 };
  };

} } // End of namespace OpenDDS::Model

#endif /* DEFAULTINSTANCETRAITS_H */

