#ifndef DDS_DEMO_ISHAPES_WRITER_QOS_HPP_
#define DDS_DEMO_ISHAPES_WRITER_QOS_HPP_

#include <dds/DdsDcpsPublicationC.h>
#include <QtGui/QtGui>
#include <ui_writerQosForm.h>

class WriterQosDialog : public QDialog
{
  Q_OBJECT;
public:
  WriterQosDialog();
  virtual ~WriterQosDialog();

public:
  DDS::DataWriterQos get_qos();

  void setPublisher(DDS::Publisher_var publisher);

public slots:
  virtual void accept();
  virtual void reject();
private:
  Ui::WriterQoS qosForm_;
  DDS::DataWriterQos qos_;
  DDS::Publisher_var publisher_;
};

#endif /* DDS_DEMO_ISHAPES_WRITER_QOS_HPP_ */
