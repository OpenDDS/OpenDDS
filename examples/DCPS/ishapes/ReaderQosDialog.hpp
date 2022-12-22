#ifndef DDS_DEMO_ISHAPES_READER_QOS_HPP_
#define DDS_DEMO_ISHAPES_READER_QOS_HPP_

#include <dds/DdsDcpsSubscriptionC.h>

// Tell GCC to ignore implicitly declared copy methods as long as
// Qt is not compliant.
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#endif

#include <QtGui/QtGui>
#include <ui_readerQosForm.h>

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

class ReaderQosDialog : public QDialog
{
  Q_OBJECT;
public:
  ReaderQosDialog();
  virtual ~ReaderQosDialog();

public:
  DDS::DataReaderQos get_qos();

  void setSubscriber(DDS::Subscriber_var subscriber);

public slots:
  virtual void accept();
  virtual void reject();

private:
  Ui::ReaderQos qosForm_;
  DDS::DataReaderQos qos_;
  DDS::Subscriber_var subscriber_;
};

#endif /* DDS_DEMO_ISHAPES_READER_QOS_HPP_ */
