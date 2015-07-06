/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <QtGui/QtGui>
#include <QtCore/QDir>
#include "GvOptions.h"

// Note calls to QObject::toStdString() have been replaced with QObject::toLocal8Bit().constData()
// to avoid QString-related aborts under windows.

namespace Monitor {

GvOptionsDialog::GvOptionsDialog( QWidget* parent)
 : QDialog( parent)
{
  ui.setupUi( this);

  // tmp dirs are platform specific.
  ui.dotLocEdit->setText(QDir::tempPath() + QDir::separator() + "monitor.dot");
  ui.pngLocEdit->setText(QDir::tempPath() + QDir::separator() + "monitor.png");

}

void
GvOptionsDialog::dialogAction(QWidget *parent, GvOptionsData &gvOpt, bool* status)
{
  GvOptionsDialog dialog( parent);

  // set current gvOptions
  dialog.ui.dotLocEdit->setText(gvOpt.dotFile_.c_str());
  dialog.ui.pngLocEdit->setText(gvOpt.pngFile_.c_str());
  dialog.ui.gvLocEdit->setText(gvOpt.graphvizBin_.c_str());

  dialog.ui.lrLayout->setChecked(gvOpt.lrLayout_);
  dialog.ui.abbrGUIDs->setChecked(gvOpt.abbrGUIDs_);
  dialog.ui.ignoreBuiltinTopics->setChecked(gvOpt.ignoreBuiltinTopics_);
  dialog.ui.hideTopics->setChecked(gvOpt.hideTopics_);
  dialog.ui.ignoreHosts->setChecked(gvOpt.ignoreHosts_);
  dialog.ui.ignoreProcs->setChecked(gvOpt.ignoreProcs_);
  dialog.ui.ignorePubs->setChecked(gvOpt.ignorePubs_);
  dialog.ui.ignoreSubs->setChecked(gvOpt.ignoreSubs_);
  dialog.ui.ignoreTransports->setChecked(gvOpt.ignoreTransports_);
  dialog.ui.ignoreQos->setChecked(gvOpt.ignoreQos_);



  switch( dialog.exec()) {
    case Accepted:
      gvOpt.dotFile_ = dialog.ui.dotLocEdit->displayText().toLocal8Bit().constData();
      gvOpt.pngFile_ = dialog.ui.pngLocEdit->displayText().toLocal8Bit().constData();
      gvOpt.graphvizBin_ = dialog.ui.gvLocEdit->displayText().toLocal8Bit().constData();

      gvOpt.lrLayout_ = dialog.ui.lrLayout->isChecked();
      gvOpt.abbrGUIDs_ = dialog.ui.abbrGUIDs->isChecked();
      gvOpt.ignoreBuiltinTopics_ = dialog.ui.ignoreBuiltinTopics->isChecked();
      gvOpt.hideTopics_ = dialog.ui.hideTopics->isChecked();
      gvOpt.ignoreHosts_ = dialog.ui.ignoreHosts->isChecked();
      gvOpt.ignoreProcs_ = dialog.ui.ignoreProcs->isChecked();
      gvOpt.ignorePubs_ = dialog.ui.ignorePubs->isChecked();
      gvOpt.ignoreSubs_ = dialog.ui.ignoreSubs->isChecked();
      gvOpt.ignoreTransports_ = dialog.ui.ignoreTransports->isChecked();
      gvOpt.ignoreQos_ = dialog.ui.ignoreQos->isChecked();

      *status = true;
      break;

    case Rejected:
      default:
      *status = false;
      break;
  }

}

void
GvOptionsDialog::getGvData(QWidget *parent, GvOptionsData &gvOpt)
{
  GvOptionsDialog dialog( parent);

  gvOpt.dotFile_ = dialog.ui.dotLocEdit->displayText().toLocal8Bit().constData();
  gvOpt.pngFile_ = dialog.ui.pngLocEdit->displayText().toLocal8Bit().constData();
  gvOpt.graphvizBin_ = dialog.ui.gvLocEdit->displayText().toLocal8Bit().constData();

  gvOpt.lrLayout_ = dialog.ui.lrLayout->isChecked();
  gvOpt.abbrGUIDs_ = dialog.ui.abbrGUIDs->isChecked();
  gvOpt.ignoreBuiltinTopics_ = dialog.ui.ignoreBuiltinTopics->isChecked();
  gvOpt.hideTopics_ = dialog.ui.hideTopics->isChecked();

  gvOpt.ignoreHosts_ = dialog.ui.ignoreHosts->isChecked();
  gvOpt.ignoreProcs_ = dialog.ui.ignoreProcs->isChecked();
  gvOpt.ignorePubs_ = dialog.ui.ignorePubs->isChecked();
  gvOpt.ignoreSubs_ = dialog.ui.ignoreSubs->isChecked();
  gvOpt.ignoreTransports_ = dialog.ui.ignoreTransports->isChecked();
  gvOpt.ignoreQos_ = dialog.ui.ignoreQos->isChecked();
}

GvOptionsData::GvOptionsData() : lrLayout_(false)
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

