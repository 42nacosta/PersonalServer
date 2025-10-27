#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define PORT "3490"
#define BACKLOG 10
#define MAXDATASIZE 100

struct thread_info
{
   HANDLE h_thread;
   SOCKET   new_fd;
};

int active_thread_count(struct thread_info *);
void add_socket_to_thread_list(SOCKET,struct thread_info *,int *);
void *get_in_addr(struct sockaddr *);
void get_msg_text(DWORD, char **);
DWORD WINAPI thread_function(LPVOID);

int main(int argc, char *argv[]) {
    struct addrinfo *ps_address;
    int i_addrlen;
    struct sockaddr_storage s_client;
    char *nc_error;
    DWORD dw_error;
    struct addrinfo s_hints;
    int i_lc;
    int i_location;
    SOCKET new_fd;
    struct addrinfo * ps_servinfo;
    char ac_server[INET6_ADDRSTRLEN];
    socklen_t sin_size;
    SOCKET sockfd;
    int i_status;
    DWORD dw_status;
    struct thread_info as_threads[BACKLOG];
    HANDLE h_thread;
    DWORD dw_thread_id;
    WSADATA s_wsaData;
    BOOL B_yes = TRUE;

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
    s_hints.ai_flags = AI_PASSIVE; //use my ip

    i_status = getaddrinfo(NULL, PORT, &s_hints, &ps_servinfo);

    if (i_status != 0) {
        dw_error = (DWORD)WSAGetLastError();
        get_msg_text(dw_error, &nc_error);
        fprintf(stderr, "getaddrinfo failed with code %d \n", i_status);
        fprintf(stderr, "%s\n", nc_error);
        LocalFree(nc_error);
        WSACleanup();

        return 3;
    }

    for (ps_address= ps_servinfo ; ps_address != NULL; ps_address = ps_address->ai_next) {
        sockfd = socket(ps_address->ai_family, ps_address->ai_socktype, ps_address->ai_protocol);
        if (socket == INVALID_SOCKET) {
            dw_error =(DWORD)WSAGetLastError();
            get_msg_text(dw_error, &nc_error);
            fprintf(stderr, "socket failed with code %d \n", i_status);
            fprintf(stderr, "%s\n", nc_error);
            LocalFree(nc_error);

            return 4;
        }

        i_status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&B_yes, sizeof(B_yes));

        if (i_status = SOCKET_ERROR) {
            dw_error = (DWORD)WSAGetLastError();
            get_msg_text(dw_error, &nc_error);
            fprintf(stderr,"setsockopt failed with code %ld.\n",dw_error);
            fprintf(stderr,"%s\n",nc_error);
            LocalFree(nc_error);
            closesocket(sockfd);
            freeaddrinfo(ps_servinfo);
            WSACleanup();

            return 4;
        }

        i_addrlen = (int) ps_address->ai_addrlen;
        i_status = bind(sockfd, ps_address->ai_addr, i_addrlen);

        if (i_status = SOCKET_ERROR) {
            dw_error = (DWORD)WSAGetLastError();
            get_msg_text(dw_error, &nc_error);
            fprintf(stderr,"setsockopt failed with code %ld.\n",dw_error);
            fprintf(stderr,"%s\n",nc_error);
            LocalFree(nc_error);
            closesocket(sockfd);
            freeaddrinfo(ps_servinfo);
            WSACleanup();

            return 4;
        }

        break;
    }

    if (ps_address == NULL) {
        dw_error = (DWORD)WSAGetLastError();
        get_msg_text(dw_error, &nc_error);
        fprintf(stderr,"listen failed with code %ld.\n",dw_error);
        fprintf(stderr,"%s\n",nc_error);
        LocalFree(nc_error);
        closesocket(sockfd);
        freeaddrinfo(ps_servinfo);
        WSACleanup();

        return 6;
    }

    freeaddrinfo(ps_servinfo);
    printf("Waiting for connection ... \n");

    while (1) {
        if (active_thread_count(as_threads) < BACKLOG) {
            sin_size = sizeof(s_client);
            new_fd = accept(sockfd, (struct sockaddr*) &s_client, &sin_size);

            if (new_fd == INVALID_SOCKET) {
                dw_error = WSAGetLastError();
                get_msg_text(dw_error, &nc_error);
                printf(stderr, "accept failed with code %d\n", dw_error);
                printf(stderr, "%s\n", nc_error);
                LocalFree(nc_error);
            }
            else {
                inet_ntop(s_client.ss_family, get_in_addr((struct sockaddr *)&s_client), ac_server, sizeof(ac_server));
                printf("Got connection from %s\n", ac_server);

                add_socket_to_thread_list(new_fd, as_threads, &i_location);
                h_thread = CreateThread(NULL, 0, thread_function, (void *)&(as_threads[i_location].new_fd), 0, &dw_thread_id);
                as_threads[i_location].h_thread = h_thread;

                if (h_thread == NULL) {
                    dw_error = WSAGetLastError();
                    get_msg_text(dw_error, &nc_error);
                    fprintf(stderr,"CreateThread failed with code %ld.\n",dw_error);
                    fprintf(stderr,"%s\n",nc_error);
                    closesocket(new_fd);
                }
            }
        }
        else {
            fprintf(stderr,"Connection refused.  Backlog is full.\n");
        }

        for (i_lc = 0; i_lc < BACKLOG; i_lc++) {
            if (as_threads[i_lc].h_thread != NULL) {
                dw_status = WaitForSingleObject(as_threads[i_lc].h_thread, 0);

                if (dw_status == WAIT_OBJECT_0) {
                    CloseHandle(as_threads[i_lc].h_thread);
                    as_threads[i_lc].h_thread = NULL;
                }
            }
        }
    }
    return 7;
}
int active_thread_count (struct thread_info *ns_threads) {
    int i_count;
    int i_lc;

    i_count = 0;

    for (i_lc = 0; i_lc < BACKLOG; i_lc++) {
        if (ns_threads[i_lc].h_thread == NULL) {
            i_count ++;
        }
    }

    return i_count;
}

void add_socket_to_thread_list(SOCKET new_fd, struct thread_info *ns_threads, int *pi_location) {
    int i_lc;
    for (i_lc = 0; i_lc < BACKLOG; i_lc++) {
        if (ns_threads[i_lc].h_thread == NULL) {
            ns_threads[i_lc].new_fd = new_fd;
            *pi_location = i_lc;
            break;
        }
    }

    return;
}
DWORD WINAPI thread_function(LPVOID lpParam) {
    char *nc_error;
    DWORD dw_error;
    DWORD dw_exit_code;
    SOCKET *p_new_fd;
    int i_status;

    dw_exit_code = 0;

    p_new_fd = (SOCKET *)lpParam;

    i_status = send(*p_new_fd, "Hello, world!", 13, 0);

    if (i_status == SOCKET_ERROR) {
        dw_error =(DWORD)WSAGetLastError();
        get_msg_text(dw_error, &nc_error);
        fprintf(stderr, "send failed with code %d \n", i_status);
        fprintf(stderr, "%s\n", nc_error);
        LocalFree(nc_error);

        dw_exit_code = 1;
    }

    closesocket(*p_new_fd);

    return(dw_exit_code);
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