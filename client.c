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

    for (ps_address = ps_servinfo; ps_address != NULL; ps_address = ps_address -> ai_next) {
        sockfd = socket(ps_address->ai_family, ps_address->ai_socktype, ps_address->ai_protocol);
        if (sockfd == INVALID_SOCKET) {
            dw_error = (DWORD)WSAGetLastError();
            get_msg_text(dw_error, &nc_error);
            fprintf(stderr, "socket failed with code %ld.\n", dw_error);
            fprintf(stderr, "%s\n", nc_error);
            LocalFree(nc_error);

            continue;
        }

        i_addrlen = (int)ps_address->ai_addrlen;
        i_status = connect(sockfd, ps_address->ai_addr, i_addrlen);

        if (i_status == SOCKET_ERROR) {
            dw_error = (DWORD)WSAGetLastError();
            get_msg_text(dw_error, &nc_error);
            fprintf(stderr, "connect failed with code %ld.\n", dw_error);
            fprintf(stderr, "%s\n", nc_error);
            LocalFree(nc_error);
            closesocket(sockfd);

            continue;
        }

        break;
    }

    if (ps_address == NULL) {
        fprintf(stderr, "Failed to connect\n");
        freeaddrinfo(ps_servinfo);
        WSACleanup();

        return 5;
    }

    inet_ntop(ps_address->ai_family, get_in_addr((struct sockaddr *)ps_address->ai_addr), ac_server, sizeof(ac_server));
    printf("Connecting to %s\n", ac_server);
    freeaddrinfo(ps_servinfo);

    i_numbytes = recv(sockfd, ac_buf, MAXDATASIZE-1, 0);
    if (i_numbytes == SOCKET_ERROR) {
        dw_error = (DWORD)WSAGetLastError();
            get_msg_text(dw_error, &nc_error);
            fprintf(stderr, "recv failed with code %ld.\n", dw_error);
            fprintf(stderr, "%s\n", nc_error);
            LocalFree(nc_error);
            closesocket(sockfd);
            WSACleanup();

            return 6;
    }
    else if(i_numbytes == 0) {
        fprintf(stderr, "Socket closed by server. \n");
        WSACleanup();

        return 7;
    }

    ac_buf[i_numbytes] = '\0';
    printf("Recieved '%s'\n", ac_buf);

    closesocket(sockfd);
    WSACleanup();

    return 0;
}

void get_msg_text(DWORD dw_error, char **pnc_msg) {
    DWORD dw_flags;
    dw_flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    FormatMessage(dw_flags, NULL, dw_error, LANG_SYSTEM_DEFAULT, (LPTSTR)pnc_msg, 0, NULL);
}

void* get_in_addr(struct sockaddr *sa) {
    if (sa -> sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}