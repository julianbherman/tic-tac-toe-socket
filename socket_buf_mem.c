#include "socket_buf_mem.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFSIZE 256

sock_buf_node *sock_buf_head;
sock_buf_node *sock_buf_tail;

pthread_mutex_t lock_buf;

void sock_buf_init(){
	sock_buf_head = NULL;
	sock_buf_tail = NULL;
	pthread_mutex_init(&lock_buf, NULL);
}

// creates a new node with allocated buffer memory
sock_buf_node *create_new_node( int sock ) {
	pthread_mutex_lock(&lock_buf); 
    // create sock_buf_node
    sock_buf_node *new = malloc( sizeof(sock_buf_node) );
    new->sock = sock;
    new->buf = (char *) malloc(BUFSIZE+1);
    new->bytes = 0;
    new->buf_offset = 0;
    new->next = NULL;

    // update sock_buf_tail
    if ( sock_buf_tail != NULL ) sock_buf_tail->next = new;
    sock_buf_tail = new;

    // update sock_buf_head
    if ( sock_buf_head == NULL ) sock_buf_head = new;
	pthread_mutex_unlock(&lock_buf);

    return new;
}

// finds, removes, and frees existing data associated with sock
// returns 0 on success
// returns -1 on failure 
int rm_sock_buf_node( int sock ) {
	pthread_mutex_lock(&lock_buf);
    sock_buf_node *prev = sock_buf_head;
    sock_buf_node *tmp = sock_buf_head;
    while( tmp != NULL ) {
	if ( tmp->sock == sock ) {
	    if ( sock_buf_head == sock_buf_tail ) {
		sock_buf_head = NULL;
		sock_buf_tail = NULL;
	    }
	    else if ( tmp == sock_buf_head ) {
		sock_buf_head = tmp->next;
	    }
	    else if ( tmp == sock_buf_tail ) {
		sock_buf_tail = prev;
		sock_buf_tail->next = NULL;
	    } // tmp is not sock_buf_head or sock_buf_tail
	    else {
		prev->next = tmp->next;
	    }
	    free(tmp->buf);
	    free(tmp);
	    pthread_mutex_unlock(&lock_buf);
	    return 0; // success
	}
	prev = tmp;
	tmp = tmp->next;
    }
	pthread_mutex_unlock(&lock_buf);
    return -1; // failure
}

// returns the sock_buf_node of sock 
// if not found, returns NULL
sock_buf_node *get_buf_info( int sock ) {
	pthread_mutex_lock(&lock_buf);
    sock_buf_node *tmp = sock_buf_head;
    while( tmp != NULL ) {
	if ( tmp->sock == sock ) { return tmp; }
	tmp = tmp->next;
    }
	pthread_mutex_unlock(&lock_buf);
    return NULL; // there is no game associated with sock
}
