/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <QtGui/QtGui>
#include "RepoSelect.h"

namespace Monitor {

RepoSelect::RepoSelect( QWidget* parent)
 : QDialog( parent)
{
  ui.setupUi( this);
  connect( ui.fileButton, SIGNAL(clicked()), this, SLOT(fileIOR()));
}

QString
RepoSelect::getRepoIOR( QWidget* parent, bool* status)
{
  RepoSelect dialog( parent);
  switch( dialog.exec()) {
    case Accepted:
      dialog.ior_ = dialog.ui.iorLineEdit->displayText();
      *status = true;
      break;

    case Rejected:
    default:
      *status = false;
      break;
  }

  return dialog.ior_;
}

void
RepoSelect::fileIOR()
{
  QString fileName = QFileDialog::getOpenFileName( this);
  if( !fileName.isEmpty()) {
    QString fileIor( "file://");
    fileIor.append( fileName);
    this->ui.iorLineEdit->setText( fileIor);
  }
}

} // End of namespace Monitor

