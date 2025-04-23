/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef GVOPTIONS_H
#define GVOPTIONS_H

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include "ui_GvOptions.h"

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

#include <map>

namespace Monitor {

// forward decl
class GvOptionsDialog;

class GvOptionsData {
  public:
    std::string getDot() { return dotFile_; };
    std::string getPng() { return pngFile_; };
    std::string getBin() { return graphvizBin_; };

    bool lrLayout() { return lrLayout_; };
    bool abbrGUIDs() { return abbrGUIDs_; };
    bool ignoreBuiltinTopics() { return ignoreBuiltinTopics_; };
    bool hideTopics() { return hideTopics_; };
    bool ignoreHosts() { return ignoreHosts_; };
    bool ignoreProcs() { return ignoreProcs_; };
    bool ignorePubs() { return ignorePubs_; };
    bool ignoreSubs() { return ignoreSubs_; };
    bool ignoreTransports() { return ignoreTransports_; };
    bool ignoreQos() { return ignoreQos_; };

    GvOptionsData();

  private:
    friend class GvOptionsDialog;

    std::string dotFile_;
    std::string pngFile_;
    std::string graphvizBin_;

    bool lrLayout_;  // left right layout
    bool abbrGUIDs_;
    bool ignoreBuiltinTopics_;
    bool hideTopics_;
    bool ignoreHosts_;
    bool ignoreProcs_;
    bool ignorePubs_;
    bool ignoreSubs_;
    bool ignoreTransports_;
    bool ignoreQos_;
};


class GvOptionsDialog : public QDialog {
  Q_OBJECT

  public:
      static void dialogAction(QWidget *parent, GvOptionsData &gvOpt, bool* status);

      static void getGvData(QWidget *parent, GvOptionsData &gvOpt);

  protected:
    GvOptionsDialog( QWidget* parent = 0);

  private:

    /// The dialog user interface.
    Ui::GvOptionsDialog ui;
};


} // End of namespace Monitor

#endif
