#ifndef FLEXTS_COMMON_H
#define FLEXTS_COMMON_H

namespace HelloWorld {
  const int HELLO_WORLD_DOMAIN = 1;

  const char STATUS_TOPIC_NAME[] = "Status";
  const char COMMAND_TOPIC_NAME[] = "Command";

  // for use with DistributedConditionSet
  const char OLDDEV[] = "OLDDEV";
  const char NEWDEV[] = "NEWDEV";
  const char CONTROLLER[] = "CONTROLLER";
  const char CONTROLLER_READY[] = "CONTROLLER_READY";
  const char OLDDEV_DONE[] = "OLDDEV_DONE";
  const char NEWDEV_DONE[] = "NEWDEV_DONE";
}

#endif
