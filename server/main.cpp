#include <bits/stdc++.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

/*
 * Protocol:
 *
 * Client sends REGISTER <api_key> <nickname>
 * Server responds either
 *   REG_OKAY <id> WELCOME <nickname>
 *   REG_FAIL <error reason>
 *
 *  Client sends REQUEST <api_key> <data_set>
 *  Server responds either
 *    ACESS_OKAY <nickname> ACCESSING <data_set>
 *    ACESS_FAIL <error reason>
 *
 * Server responds with this if bad command
 *   INVAL_CMD <message>
 */

void message_to_client(int fd, std::string message) {
  int size = send(fd, message.c_str(), message.size(), 0);
  if (size < 0) {
    perror("failed to send to client");
  }
}

const std::unordered_set<std::string> API_KEYS{{"something_professional"}};
const std::unordered_set<std::string> DATA_SETS{{"TEMP","AQI","HUMIDITY"}};

void invalid_command(int fd, std::string extra_message) {
  std::stringstream m;
  m << "INVAL_CMD " << extra_message << "\n";
  std::string message{m.str()};
  message_to_client(fd, message);
}

void key_exist_in_set(const std::unordered_set<std::string> set_to_search, std::string key_to_find) {
  if(set_to_search.find(key_to_find) != set_to_search.end()) {
    std::cout << "VALID KEY" << std::endl;
    return true;
  } else {
    std::cout << "INVALID KEY" << std::endl;
    return true;
  }
}

void handle_register(int fd, const std::string &command,
                     std::size_t begin_pos) {
  std::size_t next_pos = command.find(' ', begin_pos + 1);
  if (next_pos == std::string::npos) {
    invalid_command(fd, "USAGE REGISTER <API_KEY> <NICKNAME>");
    return;
  }
  std::string api_key =
      command.substr(begin_pos + 1, next_pos - (begin_pos + 1));
  std::cout << "API KEY: \"" << api_key << "\"\n";
  std::string nickname =
      command.substr(next_pos + 1, command.size() - (next_pos + 1) - 1);
  std::cout << "NICKNAME: \"" << nickname << "\"\n";

  key_exist_in_set(API_KEYS, api_key);
}

void handle_access(int fd, const std::string &command,
                   std::size_t begin_pos) {
  std::size_t next_pos = command.find(' ', begin_pos + 1);
  if (next_pos == std::string::npos) {
    invalid_command(fd, "USAGE ACCES <API_KEY> <DATA_SET>");
    return;
  }
  std::string api_key =
    command_substr(begin_pos + 1, next_pos - (begin_pos + 1));
  std::cout << "API KEY: \"" << api_key << "\"\n";
  std::string data_set =
      command.substr(next_pos + 1, command.size() - (next_pos + 1) - 1);
  std::cout << "DATA SET: \"" << data_set << "\"\n";

  key_exist_in_set(DATA_SETS, data_set);
}

void handle_connection(int fd) {
  char buffer[BUFSIZ];

  while (true) {
    int size = recv(fd, buffer, BUFSIZ - 1, 0);
    if (size < 0) {
      perror("failed to read from client");
      return;
    }

    buffer[BUFSIZ - 1] = 0;
    std::string command{buffer};

    std::cout << "[RECV:" << fd << "] " << command;
    std::size_t pos = command.find(' ');
    if (pos == std::string::npos) {
      invalid_command(fd, "INVALID REQ_TYPE; POSSIBLE: REGISTER, ...");
      continue;
    }
    std::string cmd = command.substr(0, pos);
    if (cmd == "REGISTER") {
      handle_register(fd, command, pos);
    } else {
      invalid_command(fd, "UNDEFINED REQ_TYPE");
    }
    if (cmd == "ACCESS") {
      handle_access(fd, command, pos);
    } else {
      invalid_command(fd, "UNDEFINED REQ_TYPE");
    }
  }
}

int main(int argc, char **argv) {
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("Failed to create socket");
    return 1;
  }

  int yes = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) {
    perror("setsockopt");
    return 1;
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port_number>\n", argv[0]);
    return 1;
  }

  int portno = atoi(argv[1]);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("error on binding");
    return 1;
  }

  listen(fd, 5);

  clilen = sizeof(cli_addr);

  std::vector<std::thread> connections;
  while (true) {
    // TODO: handle shutdown somehow? signal handler?
    int newfd = accept(fd, (struct sockaddr *)&cli_addr, &clilen);
    if (newfd < 0) {
      perror("error on accept");
      // TODO: gotta clean up all handlers maybe?
      return 1;
    }

    std::thread connection{handle_connection, newfd};
    connections.push_back(std::move(connection));
  }

  return 0;
}
