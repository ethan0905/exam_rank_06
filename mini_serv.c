/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_serv.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: esafar <esafar@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/01/15 15:51:16 by esafar            #+#    #+#             */
/*   Updated: 2023/01/16 16:44:00 by esafar           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>

//init variables and struct
typedef struct s_client{
    int id;
    char msg[1024];
}   t_client;

t_client clients[1024];

fd_set active, readfds, writefds;
int maxfd = 0, nextid = 0;
char buftoread[120000], buftowrite[120000];

void    err(char *msg)
{
    if (msg)
        write(2, msg, strlen(msg));
    else
        write(2, "Fatal error", 11);
    write(2, "\n", 1);
    exit(1);
}

void    sendAll(int senderfd)
{
    for (int fd = 0; fd <= maxfd; fd++)
        if (FD_ISSET(fd, &writefds) && fd != senderfd)
            send(fd, &buftowrite, strlen(buftowrite), 0); // <--
}

int main(int ac, char **av)
{
    //error handling
    if (ac != 2)
        err("Wrong number of arguments");
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        err(NULL);
    
    //bzero part
    FD_ZERO(&active);
    bzero(clients, sizeof(clients));
    maxfd = sockfd;
    FD_SET(sockfd, &active);
    
    //sockaddr part
    struct sockaddr_in servaddr;
    socklen_t len;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433);
    servaddr.sin_port = htons(atoi(av[1]));

    //bind and listen
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        err(NULL);
    if (listen(sockfd, 10) < 0)
        err(NULL);

    //while loop
    while (1)
    {
        //poll or select
        readfds = writefds = active;
        if (select(maxfd + 1, &readfds, &writefds, NULL, NULL) < 0)
            continue;
        for (int fd = 0; fd <= maxfd; fd++)
        {
            //== 
            if (FD_ISSET(fd, &readfds) && fd == sockfd)
            {
                //accept
                int clientfd = accept(sockfd, (struct sockaddr *)&servaddr, &len);
                if (clientfd < 0)
                    continue;
                maxfd = clientfd > maxfd ? clientfd : maxfd;
                clients[fd].id = nextid++; // <--
                FD_SET(clientfd, &active); // <--
                sprintf(buftowrite, "server: client %d has arrived\n", clients[fd].id);
                sendAll(clientfd);
                break;
            }

            //!=
            if (FD_ISSET(fd, &readfds) && fd != sockfd)
            {
                //receive
                int nbytes = recv(fd, buftoread, 70000, 0);
                if (nbytes <= 0)
                {
                    sprintf(buftowrite, "server: client %d has disconnected!\n", clients[fd].id);
                    sendAll(fd);
                    FD_CLR(fd, &active); // <--
                    close(fd);
                    break;
                }
                else
                {
                    for (int i = 0, j = strlen(clients[fd].msg); i < nbytes; i++, j++) // <--
                    {
                        //condition to stop writing the msg
                        clients[fd].msg[j] = buftoread[i];
                        if (clients[fd].msg[j] == '\n')
                        {
                            clients[fd].msg[j] = '\0';
                            sprintf(buftowrite, "client %d: %s\n", clients[fd].id, clients[fd].msg);
                            sendAll(fd);
                            bzero(clients[fd].msg, strlen(clients[fd].msg));
                            j = -1;
                        }
                    }
                    break;
                }
                
            }
        }
    }
}