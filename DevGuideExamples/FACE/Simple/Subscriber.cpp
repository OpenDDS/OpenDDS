#include <iostream>
#include <vector>
#include "build/idlTestFace/FaceMessage_TS.hpp"
#include "ace/Log_Msg.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

std::atomic_bool should_exit(false);

void handler(int signum) {
    (void) signum;
    should_exit = true;
}

int main(int, char *[]) {
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    FACE::RETURN_CODE_TYPE status;
    FACE::TS::Initialize("face_config.ini", status);

    if (status != FACE::RC_NO_ERROR) {
        return static_cast<int>(status);
    }

    FACE::CONNECTION_ID_TYPE connId;
    FACE::CONNECTION_DIRECTION_TYPE dir;
    FACE::MESSAGE_SIZE_TYPE max_msg_size;
    FACE::TS::Create_Connection("sub", FACE::PUB_SUB, connId, dir, max_msg_size, FACE::INF_TIME_VALUE, status);

    if (status != FACE::RC_NO_ERROR) {
        return static_cast<int>(status);
    }

    const FACE::TIMEOUT_TYPE timeout = 2 * 1000000000; // 2 second
    FACE::TRANSACTION_ID_TYPE txn;
    Messenger::Message msg;

    ACE_DEBUG((LM_INFO, "Subscriber: about to Receive_Message()\n"));

    while (not should_exit) {
        FACE::TS::Receive_Message(connId, timeout, txn, msg, max_msg_size, status);
        std::cout << "localStatus: " << status << "\n";
        if (status != FACE::RC_NO_ERROR and status != FACE::TIMED_OUT) {
            return static_cast<int>(status);
        }

        if (status == FACE::TIMED_OUT) {
            continue;
        }

        ACE_DEBUG((LM_INFO, "%C\t%d\n", msg.text.in(), msg.count));
    }

    FACE::TS::Destroy_Connection(connId, status);

    if (status != FACE::RC_NO_ERROR) {
        return static_cast<int>(status);
    }

    return EXIT_SUCCESS;
}