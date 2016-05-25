#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <iostream>

int main(int argc, char* argv[])
{
  int socket_fd;
  struct sockaddr_un server_address; 

  if((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
  {
    perror("client: socket");
    return 1;
  }

  // struct sockaddr_un client_address; 
  // memset(&client_address, 0, sizeof(struct sockaddr_un));
  // client_address.sun_family = AF_UNIX;
  // strcpy(client_address.sun_path, "./UDSDGCLNT");

  // unlink("./UDSDGCLNT");
  // if(bind(socket_fd, (const struct sockaddr *) &client_address, 
  //       sizeof(struct sockaddr_un)) < 0)
  // {
  //   perror("client: bind");
  //   return 1;
  // }

  memset(&server_address, 0, sizeof(struct sockaddr_un));
  server_address.sun_family = AF_UNIX;
  strcpy(server_address.sun_path, argv[1]);

  std::string line;
  while(std::getline(std::cin,line)) {
    int bytes_sent = sendto(socket_fd, line.c_str(), line.size(), 0,
        (struct sockaddr *) &server_address, 
        sizeof(struct sockaddr_un));
    if(bytes_sent<0) {
      perror("send failed.");
      break;
    }
  }

  close(socket_fd);

  return 0;
}
