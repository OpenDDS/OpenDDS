#include "SyncExt_i.h"

#include <iostream>

SyncExt_i::SyncExt_i (size_t pub_count, size_t sub_count
                        , CORBA::ORB_ptr orb)
  : SyncServer_i (pub_count, sub_count, orb)
    , entities_len (SyncExt::Subscriber+1)
{
  for (int count = 0; count < entities_len; count++)
    {
      entities_data[count].instances =
        entities_data[count].msecs = 0;
    }
}

SyncExt_i::~SyncExt_i (void)
{
}

void
SyncExt_i::publish (SyncExt::Role role
                  , ::CORBA::Long instances
                  , ::CORBA::Long msecs)
{
  if (role > SyncExt::Subscriber) {
    return;
  }

  entities_data[role].instances += instances;
  entities_data[role].msecs += msecs;
}

void
SyncExt_i::print_results (void)
{
  std::cout << std::endl;

  bool valid = false;
  for (int count = 0; count < entities_len; count++)
    {
      valid = false;
      switch (count)
        {
        case SyncExt::Topic:
          std::cout << "Role: Topic,  ";
          valid = true;
          break;
        case SyncExt::Participant:
          std::cout << "Role: Participant,  ";
          valid = true;
          break;
        case SyncExt::Publisher:
          std::cout << "Role: Publisher,  ";
          valid = true;
          break;
        case SyncExt::Subscriber:
          std::cout << "Role: Subscriber,  ";
          valid = true;
          break;
        }

      if (valid)
        {
          std::cout << entities_data[count].instances << " instances in "
                    << entities_data[count].msecs << " milliseconds." << std::endl;
        }
    }
}
