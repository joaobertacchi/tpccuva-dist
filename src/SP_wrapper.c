#include <stdio.h>
#include <string.h>
#include <sp.h>
#include "SP_wrapper.h"

int SP_wp_started = 0;
mailbox mbox;
char private_group[MAX_GROUP_NAME + 1];
FILE *output;
service stype = 0;
char sender[MAX_GROUP_NAME + 1];
int num_groups;
char groups[1][MAX_GROUP_NAME + 1];
int16 mtype;
int endian_mismatch;

int SP_init(){
	int status;

	output = fopen("/dev/null", "w");
	/* output = stdout; */
	status = SP_connect("4803@localhost", NULL, 0, 0, &mbox, private_group);

        if(status == ACCEPT_SESSION){
                fprintf(output, "Connection accepted.\n");
                fprintf(output, "private_group: %s\n", private_group);
        }
        else{
                SP_error(status);
                //exit(EXIT_FAILURE);
		return 0;
        }

        status = SP_join(mbox,"tpcc-uva");
        if(status == 0) fprintf(output, "This process has joined to group tpcc-uva.\n");
        else{
                SP_error(status);
                //exit(EXIT_FAILURE);
		return 0;
        }
	SP_wp_started = 1;
	return SP_wp_started;
}

int SP_send(char *buff, int bufsize){
	int size;
	int status;

	if (!SP_wp_started)
		return (0);

	//size = strlen(buff) + 1; /* Including '\0' */
	size = bufsize;
	if (size > bufsize)
		return 0;
        status = SP_multicast(mbox, SAFE_MESS, "tpcc-uva", 0, sizeof(char)*size, buff);
        if(status >= 0) fprintf(output, "Message sent to tpcc-uva group successfully.\n");
        else{
                SP_error(status);
                //exit(EXIT_FAILURE);
		return 0;
        }
	return 1;
}

int SP_recv(char *buff, int bufsize){
	int status;
	int service = MEMBERSHIP_MESS;

	while (!SP_wp_started){
		fprintf(stdout, "Membership not started yet! Waiting 1 sec...\n");
		sleep(1);
	}
	
//	while (service == MEMBERSHIP_MESS){
		service = 0;
        	status = SP_receive(mbox, &service, sender, 1, &num_groups, groups, &mtype, &endian_mismatch, bufsize, buff);
//        	if(service == MEMBERSHIP_MESS)
//			fprintf(output, "Message received. Discarding...\n");
//	}
	if(status >= 0 && status <= bufsize){
                fprintf(output, "Message received:\n");
		fprintf(output, "size = %d\n", status);
		fprintf(output, "message type = %d\n", mtype);
		fprintf(output, "sender = %s\n", sender);
		fprintf(output, "content = \"%s\"\n", buff);
	}
	else{
        	SP_error(status);
		return 0;
	}
	stype = service;
	return 1;
}

/*
int main(){
	char buff[]="amigo";
	char buff2[100];

	SP_init();
	SP_send(buff, strlen(buff) + 1);
	SP_recv(buff2, 100);
	return 0;
}
*/
