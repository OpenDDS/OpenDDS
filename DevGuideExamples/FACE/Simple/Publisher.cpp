#include <iostream>
#include <fstream>
#include <vector>
#include "build/idlTestFace/FaceMessage_TS.hpp"
#include "ace/Log_Msg.h"

#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

void fillOctet(Messenger::Message &msg) {
    std::ifstream file("model.h5", std::ios::binary);
    if (!file) {
        std::cerr << "Error on opening file" << std::endl;
        return;
    }

    std::vector<unsigned char> input(std::istreambuf_iterator<char>(file), {});
    unsigned long int inputSize = input.size();

    Messenger::DataField dataField;
    dataField.name = "input";

    dataField.value.length(inputSize);
    for (unsigned long int i = 0; i < inputSize; ++i) {
        dataField.value[i] = input[i];
    }

    CORBA::ULong len = msg.data.length();
    msg.data.length(len + 1);
    msg.data[len] = dataField;
}

// FUZZ: disable check_for_improper_main_declaration
int main(int, char *[]) {
    // Initialize the TS interface
    FACE::RETURN_CODE_TYPE status;
    FACE::TS::Initialize("face_config.ini", status);
    if (status != FACE::RC_NO_ERROR) {
        return static_cast<int>(status);
    }

    // Create the pub connection
    FACE::CONNECTION_ID_TYPE connId;
    FACE::CONNECTION_DIRECTION_TYPE dir;
    FACE::MESSAGE_SIZE_TYPE max_msg_size;
    FACE::TS::Create_Connection(
            "pub", FACE::PUB_SUB, connId, dir,
            max_msg_size, FACE::INF_TIME_VALUE, status);
    if (status != FACE::RC_NO_ERROR) {
        return static_cast<int>(status);
    }

    const FACE::TIMEOUT_TYPE timeout = 2 * 1000000000; // 2 second
    // Message to send
    Messenger::Message msg;
    msg.text = "Hello, World!";
    msg.subject_id = 14;
    msg.count = 1;
    fillOctet(msg);

    // Send message
    FACE::TRANSACTION_ID_TYPE txn;
    ACE_DEBUG((LM_INFO, "Publisher: about to Send_Message()\n"));
    FACE::TS::Send_Message(
            connId, timeout, txn, msg,
            max_msg_size, status);
    std::cout << "Status: " << status << std::endl;
    if (status != FACE::RC_NO_ERROR) {
        return static_cast<int>(status);
    }

    // Give message time to be processed before exiting
//    ACE_OS::sleep(15);

    // Destroy the pub connection
    FACE::TS::Destroy_Connection(connId, status);
    if (status != FACE::RC_NO_ERROR) {
        return static_cast<int>(status);
    }

    return EXIT_SUCCESS;
}
// FUZZ: enable check_for_improper_main_declaration
