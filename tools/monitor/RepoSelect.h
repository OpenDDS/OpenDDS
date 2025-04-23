/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef REPOSELECT_H
#define REPOSELECT_H

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include "ui_RepoDialog.h"

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

namespace Monitor {

class RepoSelect : public QDialog {
  Q_OBJECT

  public:
    static QString getRepoIOR( QWidget* parent, bool* status);

  protected:
    RepoSelect( QWidget* parent = 0);

  private slots:
    void fileIOR();

  private:
    /// The IOR string value produced by this dialog, if any.
    QString ior_;

    /// The dialog user interface.
    Ui::RepoSelectDialog ui;
};

} // End of namespace Monitor

#endif /* REPOSELECT_H */
