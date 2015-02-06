#include <ReaderQosDialog.hpp>
#include <iostream>

ReaderQosDialog::ReaderQosDialog()
{
  qosForm_.setupUi(this);
  this->setVisible(false);
}

ReaderQosDialog::~ReaderQosDialog() { }

void
ReaderQosDialog::setSubscriber(DDS::Subscriber_var subscriber)
{
  subscriber_ = subscriber;
}

void
ReaderQosDialog::accept()
{
  this->setVisible(false);
}

void
ReaderQosDialog::reject()
{
  this->setVisible(false);
}

DDS::DataReaderQos
ReaderQosDialog::get_qos()
{
  subscriber_->get_default_datareader_qos(qos_);

  if (qosForm_.reliableRButt->isChecked()) {
    qos_.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    qos_.reliability.max_blocking_time.sec = 0;
    qos_.reliability.max_blocking_time.nanosec = 0;
  }
  switch (qosForm_.durabilityComboBox->currentIndex()) {
  case 0:
    qos_.durability.kind = DDS::VOLATILE_DURABILITY_QOS;
    break;
  case 1:
    qos_.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    break;
  case 2:
    qos_.durability.kind = DDS::TRANSIENT_DURABILITY_QOS;
    break;
  case 3:
    qos_.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
    break;
  }

  if (qosForm_.exclusiveRButt->isChecked())
    qos_.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;

  if (qosForm_.keepLastRButton->isChecked())
  {
    qos_.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
    qos_.history.depth = qosForm_.depthSpinBox->value();
  }
  else
    qos_.history.kind = DDS::KEEP_ALL_HISTORY_QOS;

  qos_.time_based_filter.minimum_separation.sec =
    std::atoi(qosForm_.tbFilterInput->text().toAscii().constData());

  return qos_;
}
