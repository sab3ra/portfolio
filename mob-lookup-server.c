#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "mylist.h"
#include "mdb.h"
#define KeyMax 5

static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char **argv)
{
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        die("signal() failed");
    }
	if (argc != 3) {
        fprintf(stderr, "usage: %s <port> <filename>\n", argv[0]);
        exit(1);
    }
	unsigned short port = atoi(argv[2]);
	char *filename = argv[1];

	int servsock;
	if((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");
	
	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
    	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    	servaddr.sin_port = htons(port);

	if(bind(servsock,(struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        die("bind failed");
	if(listen(servsock, 5) <0)
	die("listen failed");
	int clntsock;
	socklen_t clntlen;
	struct sockaddr_in clntaddr;
	
	struct List list;
	initList(&list);

	while(1){
		clntlen = sizeof(clntaddr);
		if((clntsock = accept(servsock, (struct sockaddr *)&clntaddr, &clntlen)) <0)
			die("accept failed");
	printf("\nconnection started from: %s\n", inet_ntoa(clntaddr.sin_addr));

        FILE *fp = fopen(filename, "rb");
        if(fp == NULL){
		die(filename);
		printf("error opening file");
	}
        int loaded = loadmdb(fp, &list);
        if(loaded <0){
                die("loadmdb");
		printf("loadmdb doesnt work");
	}
        fclose(fp);

	FILE *input = fdopen(clntsock, "r");
	if(input == NULL){
		die("input failed");
		printf("input doesnt work");
	}
	 char key[KeyMax + 1];

    for(;;){
	   if (fgets(key, sizeof(key), input) == NULL){
		   break;
	   }
    size_t last = strlen(key)-1;
            if(key[last] == '\n'){
                    key[last] = '\0';
            }

    struct Node *node = list.head;
    int recNo = 1;
    char result[1000];
    int found = 0;

	
        while (node) {
            struct MdbRec *rec = (struct MdbRec *)node->data;
            if (strstr(rec->name, key) || strstr(rec->msg, key)) {
		    found = 1;
               snprintf(result, sizeof(result), "%4d: {%s} said {%s}\n", recNo, rec->name, rec->msg);
	       if(send(clntsock, result, strlen(result), 0) < 0){
                die("send failed");
            }
               }
        node = node->next;
            recNo++;
        }
	if(!found)
	{
		const char *msg = "no records found\n";
		send(clntsock, msg, strlen(msg), 0);
	}
	send(clntsock, "\n", 1, 0);
    }
        if(ferror(input)){
		die("input failed");
            }
            fclose(input);
            close(clntsock);
            
	    removeAllNodes(&list);
}
	freemdb(&list);
	close(servsock);
	return 0;
}
