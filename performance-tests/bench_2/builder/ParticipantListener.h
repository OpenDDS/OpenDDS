#pragma once

#include "Participant.h"

namespace Builder {

class ParticipantListener : public DDS::DomainParticipantListener {
public:
  virtual void set_participant(Participant& participant) = 0;
  virtual void unset_participant(Participant& participant) = 0;
};

}
