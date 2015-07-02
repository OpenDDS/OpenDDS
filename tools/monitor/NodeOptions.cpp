/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <QtGui/QtGui>
#include "NodeOptions.h"

namespace Monitor {

NodeOptionsDialog::NodeOptionsDialog( QWidget* parent)
 : QDialog( parent)
{
  ui.setupUi( this);

}

void
NodeOptionsDialog::dialogAction(QWidget *parent, NodeOptionsData &nodeOpt, bool* status)
{
  NodeOptionsDialog dialog( parent);

  // set current gvOptions
  dialog.ui.parentChild->setChecked(nodeOpt.parentChild_);
  dialog.ui.pubSub->setChecked(nodeOpt.pubSub_);
  dialog.ui.abbrGUIDs->setChecked(nodeOpt.abbrGUIDs_);
  dialog.ui.ignoreBuiltinTopics->setChecked(nodeOpt.ignoreBuiltinTopics_);
  dialog.ui.hideTopics->setChecked(nodeOpt.hideTopics_);
  dialog.ui.ignoreHosts->setChecked(nodeOpt.ignoreHosts_);
  dialog.ui.ignoreProcs->setChecked(nodeOpt.ignoreProcs_);
  dialog.ui.ignorePubs->setChecked(nodeOpt.ignorePubs_);
  dialog.ui.ignoreSubs->setChecked(nodeOpt.ignoreSubs_);
  dialog.ui.ignoreTransports->setChecked(nodeOpt.ignoreTransports_);
  dialog.ui.ignoreQos->setChecked(nodeOpt.ignoreQos_);

  switch( dialog.exec()) {
    case Accepted:
      nodeOpt.parentChild_ = dialog.ui.parentChild->isChecked();
      nodeOpt.pubSub_ = dialog.ui.pubSub->isChecked();
      nodeOpt.abbrGUIDs_ = dialog.ui.abbrGUIDs->isChecked();
      nodeOpt.ignoreBuiltinTopics_ = dialog.ui.ignoreBuiltinTopics->isChecked();
      nodeOpt.hideTopics_ = dialog.ui.hideTopics->isChecked();
      nodeOpt.ignoreHosts_ = dialog.ui.ignoreHosts->isChecked();
      nodeOpt.ignoreProcs_ = dialog.ui.ignoreProcs->isChecked();
      nodeOpt.ignorePubs_ = dialog.ui.ignorePubs->isChecked();
      nodeOpt.ignoreSubs_ = dialog.ui.ignoreSubs->isChecked();
      nodeOpt.ignoreTransports_ = dialog.ui.ignoreTransports->isChecked();
      nodeOpt.ignoreQos_ = dialog.ui.ignoreQos->isChecked();

      *status = true;
      break;

    case Rejected:
      default:
      *status = false;
      break;
  }

}

void
NodeOptionsDialog::getNodeData(QWidget *parent, NodeOptionsData &nodeOpt)
{
  NodeOptionsDialog dialog( parent);

  nodeOpt.parentChild_ = dialog.ui.parentChild->isChecked();
  nodeOpt.pubSub_ = dialog.ui.pubSub->isChecked();

  nodeOpt.abbrGUIDs_ = dialog.ui.abbrGUIDs->isChecked();
  nodeOpt.ignoreBuiltinTopics_ = dialog.ui.ignoreBuiltinTopics->isChecked();
  nodeOpt.hideTopics_ = dialog.ui.hideTopics->isChecked();

  nodeOpt.ignoreHosts_ = dialog.ui.ignoreHosts->isChecked();
  nodeOpt.ignoreProcs_ = dialog.ui.ignoreProcs->isChecked();
  nodeOpt.ignorePubs_ = dialog.ui.ignorePubs->isChecked();
  nodeOpt.ignoreSubs_ = dialog.ui.ignoreSubs->isChecked();

}

NodeOptionsData::NodeOptionsData() : parentChild_(false)
, pubSub_(false)
, abbrGUIDs_(false)
, ignoreBuiltinTopics_(false)
, hideTopics_(false)
, ignoreHosts_(false)
, ignoreProcs_(false)
, ignorePubs_(false)
, ignoreSubs_(false)
, ignoreTransports_(false)
, ignoreQos_(false)
{
}


} // End of namespace Monitor

