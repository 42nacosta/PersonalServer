#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define PORT "3490"
#define MAXDATASIZE 100

void *get_in_addr(struct sockaddr *);
void get_msg_text(DWORD, char **);

int main(int argc, char *argv[]) {
    struct addrinfo *ps_address;
    int i_addrlen;
    char ac_buf[MAXDATASIZE];
    char *nc_error;
    DWORD dw_error;
    struct addrinfo s_hints;
    int i_numbytes;
    char ac_server[INET6_ADDRSTRLEN];
    struct addrinfo * ps_servinfo;
    SOCKET sockfd;
    int i_status;
    WSADATA s_wsaData;

    if (argc != 2) {
        fprintf(stderr, "usage: WSclient hostname\n");
        return 1;
    }

    i_status = WSAStartup(MAKEWORD(2, 2), &s_wsaData);

    if (i_status != 0) {
        dw_error = (DWORD)i_status;
        get_msg_text(dw_error, &nc_error);
        fprintf(stderr, "WSAStartup failed with code %d \n", i_status);
        fprintf(stderr, "%s\n", nc_error);
        LocalFree(nc_error);
        return 2;
    }

    if (HIBYTE(s_wsaData.wVersion) != 2 || LOBYTE(s_wsaData.wVersion) != 2) {
        fprintf(stderr, "Version 2.2 of Winsock is not available\n");
        WSACleanup();
        return 3;
    }
    
    memset(&s_hints, 0, sizeof(s_hints));

    s_hints.ai_family = AF_UNSPEC;
    s_hints.ai_socktype = SOCK_STREAM;

    i_status = getaddrinfo(argv[1], PORT, &s_hints, &ps_servinfo);

    if (i_status != 0) {
        dw_error = (DWORD) i_status;
        get_msg_text(dw_error, &nc_error);
        fprintf(stderr, "getaddrinfo failed with code %ld", dw_error);
        fprintf(stderr, "%s\n", nc_error);
        WSACleanup();
        LocalFree(nc_error);

        return 4;
    }


}