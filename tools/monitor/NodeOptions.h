/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef NODEOPTIONS_H
#define NODEOPTIONS_H

#include "ui_NodeOptions.h"
#include <map>

namespace Monitor {

// forward decl
class NodeOptionsDialog;

class NodeOptionsData {
  public:
    bool hideParentChild() { return parentChild_; };
    bool hidePubSub() { return pubSub_; };
    bool abbrGUIDs() { return abbrGUIDs_; };
    bool ignoreBuiltinTopics() { return ignoreBuiltinTopics_; };
    bool hideTopics() { return hideTopics_; };
    bool ignoreHosts() { return ignoreHosts_; };
    bool ignoreProcs() { return ignoreProcs_; };
    bool ignorePubs() { return ignorePubs_; };
    bool ignoreSubs() { return ignoreSubs_; };
    bool ignoreTransports() { return ignoreTransports_; };
    bool ignoreQos() { return ignoreQos_; };

    NodeOptionsData();

  private:
    friend class NodeOptionsDialog;

    bool parentChild_;
    bool pubSub_;
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


class NodeOptionsDialog : public QDialog {
  Q_OBJECT

  public:
      static void dialogAction(QWidget *parent, NodeOptionsData &nodeOpt, bool* status);

      static void getNodeData(QWidget *parent, NodeOptionsData &nodeOpt);

  protected:
    NodeOptionsDialog( QWidget* parent = 0);

  private:
    /// The dialog user interface.
    Ui::NodeOptionsDialog ui;
};


} // End of namespace Monitor

#endif
