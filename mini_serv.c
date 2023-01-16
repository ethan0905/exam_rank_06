/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: esafar <esafar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/01/15 15:51:16 by esafar            #+#    #+#             */
/*   Updated: 2023/01/16 15:34:34 by esafar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct  s_clients {
    int     id;
    char    msg[1024];
}               t_clients;

t_clients   clients[1024];

fd_set      readfds, writefds, active;
int         maxfd = 0, nextid = 0;
char        buftoread[120000], buftowrite[120000];

void    err(char *str)
{
    if (str)
        write(2, str, strlen(str));
    else
        write(2, "Fatal error", strlen("Fatal error"));
    write(2, "\n", 1);
    exit(1);
}

void    sendAll(int senderfd)
{
    for (int fd = 0; fd <= maxfd; fd++)
        if (FD_ISSET(fd, &writefds) && fd != senderfd)
            send(fd, buftowrite, strlen(buftowrite), 0);
}

int main(int ac, char **av)
{
    if (ac != 2)
        err("Wrong number of arguments");

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        err(NULL);

    FD_ZERO(&active);
    bzero(&clients, sizeof(clients));
    maxfd = sockfd;
    FD_SET(sockfd, &active);

    struct sockaddr_in  servaddr;
    socklen_t           len;
   	bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

    if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) < 0)
        err(NULL);
    if (listen(sockfd, 10) < 0)
        err(NULL);

    while(1)
    {
        readfds = writefds = active;
        if (select(maxfd + 1, &readfds, &writefds, NULL, NULL) < 0)
            continue;
        for(int fd = 0; fd <= maxfd; fd++)
        {
            if (FD_ISSET(fd, &readfds) && fd == sockfd)
            {
                int clientfd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
                if (clientfd < 0)
                    continue;
                maxfd = clientfd > maxfd ? clientfd : maxfd;
                clients[clientfd].id = nextid++;
                FD_SET(clientfd, &active);
                sprintf(buftowrite, "server: client %d just arrived\n", clients[clientfd].id);
                sendAll(clientfd);
                break;
            }
            if (FD_ISSET(fd, &readfds) && fd != sockfd)
            {
                int res = recv(fd, buftoread, 65536, 0);
                if (res <= 0) {
                    sprintf(buftowrite, "server: client %d just left\n", clients[fd].id);
                    sendAll(fd);
                    FD_CLR(fd, &active);
                    close(fd);
                    break;
                }
                else
                {
                    for (int i = 0, j = strlen(clients[fd].msg); i < res; i++, j++)
                    {
                        clients[fd].msg[j] = buftoread[i];
                        if (clients[fd].msg[j] == '\n')
                        {
                            clients[fd].msg[j] = '\0';
                            sprintf(buftowrite, "client %d: %s\n", clients[fd].id, clients[fd].msg);
                            sendAll(fd);
                            bzero(&clients[fd].msg, strlen(clients[fd].msg));
                            j = -1;
                        }
                    }
					break;
                }
            }
        }
    }
}