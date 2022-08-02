/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/SOCK_Connector.h>
#include <ace/SOCK_Stream.h>

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  ACE_SOCK_Connector connector;
  ACE_SOCK_Stream stream;
  ACE_INET_Addr remote("127.0.0.1:8081");

  {
    if (connector.connect(stream, remote) != 0) {
      ACE_ERROR((LM_ERROR, "Could not connect\n"));
      return EXIT_FAILURE;
    }

    const char request[] = "GET / HTTP/1.1\r\n\r\n";
    const ssize_t size = sizeof(request) - 1;
    if (stream.send(request, size) != size) {
      ACE_ERROR((LM_ERROR, "Could not send request\n"));
      return EXIT_FAILURE;
    }

    char response[64 * 1024];
    ssize_t bytes = stream.recv(response, sizeof(response));
    if (bytes <= 0) {
      ACE_ERROR((LM_ERROR, "Could not read response\n"));
      return EXIT_FAILURE;
    }
    response[bytes] = 0;

    ACE_DEBUG((LM_DEBUG, "Response: %C\n", response));

    stream.close();

    const char expected_response[]=
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 2\r\n"
      "\r\n"
      "{}";

    if (strcmp(response, expected_response) != 0) {
      ACE_ERROR((LM_ERROR, "Did not get expected response\n"));
      return EXIT_FAILURE;
    }
  }

  {
    if (connector.connect(stream, remote) != 0) {
      ACE_ERROR((LM_ERROR, "Could not connect\n"));
      return EXIT_FAILURE;
    }

    const char request[] = "GET /healthcheck HTTP/1.1\r\n\r\n";
    const ssize_t size = sizeof(request) - 1;
    if (stream.send(request, size) != size) {
      ACE_ERROR((LM_ERROR, "Could not send request\n"));
      return EXIT_FAILURE;
    }

    char response[64 * 1024];
    ssize_t bytes = stream.recv(response, sizeof(response));
    if (bytes <= 0) {
      ACE_ERROR((LM_ERROR, "Could not read response\n"));
      return EXIT_FAILURE;
    }
    response[bytes] = 0;

    ACE_DEBUG((LM_DEBUG, "Response: %C\n", response));

    stream.close();

    const char expected_response[]=
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 2\r\n"
      "\r\n"
      "OK";

    if (strcmp(response, expected_response) != 0) {
      ACE_ERROR((LM_ERROR, "Did not get expected response\n"));
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
