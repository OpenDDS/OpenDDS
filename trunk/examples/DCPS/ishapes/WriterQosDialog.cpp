#include <WriterQosDialog.hpp>
#include <iostream>

WriterQosDialog::WriterQosDialog()
{
  qosForm_.setupUi(this);
  this->setVisible(false);
}

WriterQosDialog::~WriterQosDialog() { }

void
WriterQosDialog::setPublisher(DDS::Publisher_var publisher)
{
  publisher_ = publisher;
}

void
WriterQosDialog::accept()
{
  this->setVisible(false);
}

void
WriterQosDialog::reject()
{
  this->setVisible(false);
}

DDS::DataWriterQos
WriterQosDialog::get_qos()
{
  publisher_->get_default_datawriter_qos(qos_);

  if (!qosForm_.reliableRButt->isChecked()) {
    qos_.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
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
  };

  qos_.transport_priority.value = qosForm_.prioritySpinBox->value();

  if (qosForm_.ownershipExclusiveRButt->isChecked()) {
    qos_.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
    qos_.ownership_strength.value = qosForm_.strengthSpinBox->value();
  }

  qos_.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  qos_.history.depth = 100;
  return qos_;
}
