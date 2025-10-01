#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>

#include <ws2tcpip.h>

void get_msg_text(DWORD, char **);



int main(int argc, char *argv[])
{
    struct addrinfo *ps_address;
    void            *pv_address;
    char            *nc_error;
    DWORD           dw_error;
    struct addrinfo s_hints;
    char            ac_ipstr[INET6_ADDRSTRLEN];
    struct sockaddr_in *ps_ipv4;
    struct sockaddr_in6 *ps_ipv6;
    char            *pc_ipver;
    struct addrinfo *ps_res;
    int             i_status;
    WSADATA         s_wsaData;

    //Check if one command line arg, should be host's name
    if(argc != 2) {
        //print into stderr file the usage message
        fprintf(stderr, "usage: WSshowip hostname\n");
        //exit with case 1
        return 1;
    }

    //initialize winsock and request version 2.2
    //check status of startup, if 0 then pass
    i_status = WSAStartup(MAKEWORD(2,2), &s_wsaData);

    //check if startup failed
    if (i_status != 0)
    {
        //get error code, cast DWORD to i_status to allow get_msg_text to work
        dw_error = (DWORD)i_status;
        //use get_msg_text to get error message from i_status, and a nc_error pointer to store the error message
        get_msg_text(dw_error, &nc_error);
        //print the failure code
        fprintf(stderr, "WSAStartup failed with code%d.\n", i_status);
        //write the error message to the file
        fprintf(stderr, "%s\n", nc_error);
        //exit with case 2
        return 2;
    }

    //Now that winsock is initialized, check if version is correct
    if (LOBYTE(s_wsaData.wVersion) < 2 || HIBYTE(s_wsaData.wVersion) < 2) {
        //print to the file stdrr, version not available
        fprintf(stderr, "Version 2.2 of Winsock is not available.\n");
        //run cleanup to close winsock
        WSACleanup();
        //exit with case 3
        return 3;
    }

    //set the desired IP address characteristics
    //set memory at s_hints to 0, size of s_hints
    
    printf("%zu/n", sizeof(s_hints));
    memset(&s_hints, 0, sizeof(s_hints));

    s_hints.ai_family = AF_UNSPEC; //either IPv4 or IPv6
    s_hints.ai_socktype = SOCK_STREAM; //Initialize a socktype as stream
    
    //request a list of matching IPs by pulling out addrinfo structs relating to host node
    i_status = getaddrinfo(argv[1], NULL, &s_hints, &ps_res);

    //check if getaddrinfo failed
    if (i_status != 0)
    {
        //get error code, can use WSAGetLastError to get error code instead of i_status for more specific message
        dw_error = (DWORD)WSAGetLastError();
        get_msg_text(dw_error, &nc_error);
        //write to stderr file with failure code
        fprintf(stderr, "getaddrinfo failed with code$ld.\n", dw_error);
        fprintf(stderr, "%s\n", nc_error);
        //free memory up related to nc_error
        LocalFree(nc_error);
        //cleanup winsock again
        WSACleanup();

        //exit with case 4
        return 4;
    }

    //print out addresses
    printf("IP addresses for #s:\n\n", argv[1]);

    for (ps_address = ps_res; ps_address != NULL; ps_address = ps_address -> ai_next)
    {
        //check if IPv4
        if (ps_address->ai_family == AF_INET) {
            ps_ipv4 = (struct sockaddr_in *) ps_address->ai_addr;
            pv_address = &(ps_ipv4->sin_addr);
            pc_ipver = "IPv4";
        }
        else {
            ps_ipv6 = (struct sockaddr_in6 *) ps_address->ai_addr;
            pv_address = &(ps_ipv6->sin6_addr);
            pc_ipver = "IPv6";
        }
        //now we have an address, convert into string and print
        //this method takes the family, address, a buffer, and size of buffer to store string
        inet_ntop(ps_address->ai_family, pv_address, ac_ipstr, sizeof(ac_ipstr));
        printf(" %s: %s\n", pc_ipver, ac_ipstr);
    }

    //terminate winsock
    WSACleanup();
    //free memory allocated to adderinfo structs
    freeaddrinfo(ps_res);
    //return success code
    return 0;
}
    //get_msg_text function definition
    void get_msg_text(DWORD dw_error, char **pnc_msg) {
        DWORD dw_flags;
        //set message options
        dw_flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        //create the message string
        FormatMessage(dw_flags, NULL, dw_error, LANG_SYSTEM_DEFAULT, (LPTSTR)pnc_msg, 0, NULL);
    }