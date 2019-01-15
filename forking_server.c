#include "networking.h"

void process(char *s);
void subserver(int from_client);
int num_from_string(char s);
int add_client(int chatroom_id, int * chatrooms, int client_socket);

/*
char * chat_shared_mem(char id, int size){
  key_t key = ftok("forking_server.c", id);
  printf("key: %d\n", key);

  // gets id
  int shmid = shmget(key, size, 0666|IPC_CREAT);
  printf("id: %d\n", shmid);

  // attach to shared memory
  char * chat_history = shmat(shmid, 0, 0);
  if(chat_history == (void*) -1){
    perror("shmat");
  }
  //shmctl(shmid, IPC_RMID, NULL);
  return chat_history;
}
*/
int main() {
    key_t key = 123;
  int shmid = shmget(key, 100000 * NUM_CHATS, IPC_CREAT|0666);
  char (*chat_hist)[5][100000];
  chat_hist = shmat(shmid, 0, 0);
  for (int i = 0; i<NUM_CHATS; i++){
    char title[100];
    sprintf(title, "=====\nCHAT %d\n=====\n", i);
    strcpy((*chat_hist)[i], title);
  }
  for (int i = 0; i<NUM_CHATS; i++){
    //printf("chat_hist[%d]: %s", i, (*chat_hist)[i]);
  }

  //char * chat_history = chat_shared_mem('a', 1000000000)
  //memset(chat_history, 0, sizeof(chat_history));

  char * chat_history = calloc(100000, sizeof(char));

  int listen_socket;
  int listen_socket_1;
  int client;
  listen_socket = server_setup();
  listen_socket_1 = listen_socket;

  int clients[NUM_CLIENTS];
  for(int i = 0; i < NUM_CLIENTS; i++){
    clients[i] = 0;
  }
  char buffer[BUFFER_SIZE];

  fd_set read_fds;

  while (1) {
    FD_ZERO(&read_fds);
    FD_SET(listen_socket, &read_fds);

    for (int i = 0; i < NUM_CLIENTS; i++){
      if(clients[i]>0){
        FD_SET(clients[i], &read_fds);
        if(clients[i] > listen_socket_1){
	  listen_socket_1 = clients[i];
        }
      }
    }

    //select will block until either fd is ready
    select(listen_socket_1 + 1, &read_fds, NULL, NULL, NULL);

   //if listen_socket triggered select
    if (FD_ISSET(listen_socket, &read_fds)){
      // client socket <- the special id to read and write to client
      if((client = server_connect(listen_socket))){
	for(int i = 0; i < NUM_CLIENTS && clients[i]!=client; i++){
	  if(!clients[i]){
      // client socket id is stored in clients array
	    clients[i] = client;
	    i = NUM_CLIENTS; // stop the loop
	  }
	}
      }
    }

    // generates key
    key_t key = ftok("networking.h", 'A');
    printf("key: %d\n", key);

    // gets id
    int shmid = shmget(key, NUM_CHATS * NUM_CLIENTS, 0666|IPC_CREAT);
    printf("id: %d\n", shmid);

    // attach to shared memory
    int * chatrooms = shmat(shmid, 0, 0);
    if(chatrooms == (void*) -1){
      perror("shmat");
    }

    for (int i = 0; i<NUM_CLIENTS && clients[i]; i++){
      if(FD_ISSET(clients[i], &read_fds)){
	if(read(clients[i], buffer, sizeof(buffer))>0){
	  printf("[subserver %d] received: %s\n", getpid(), buffer);

    // joining part
    char * tempbuff;
    if ( (tempbuff = strchr(buffer, '~')) ){
      if (! strncmp("~join", tempbuff, 5) ){
        // printf("NEEDS TO JOIN A NEW SERVER!!!\n");
        // char * ans = tempbuff + 6;
        memcpy(tempbuff, tempbuff + 6, 6 * sizeof(char));
        printf("BUFFER: %s\n", tempbuff);
        // printf("ANS: %s\n", ans);
        // printf("PID: %d\n", getpid());
        printf("subserver %d wants to connect to chatroom: %s\n", getpid(), tempbuff);
        int chatroom_id = num_from_string(*tempbuff);
        printf("chatroom_id: %d\n", chatroom_id);
        // adding the client to chatroom and the main server's lists of clients in chats
        add_client(chatroom_id, chatrooms, clients[i]);
        for (int i=0; i < NUM_CHATS * NUM_CLIENTS; i++){
          printf("%d ", chatrooms[i]);
        }

        // notification to say you have joined
        sprintf(buffer, "you have joined chatroom %d\n", chatroom_id);
        printf("info in buffer: %s\n", buffer);
        write(clients[i], buffer, sizeof(buffer));
      }
    }
    else {
      // normal portion otherwise
      strcat(chat_history, buffer);
      for (int i = 0; i<NUM_CLIENTS && clients[i]; i++){
      write(clients[i], chat_history, sizeof(buffer));
     }
    }
	}
	else{
	  close(clients[i]);
	  clients[i] = 0;
	}
      }
    }
  }
  return 0;
}

int num_from_string(char s){
  int num = s - '0';
  return num;
}

// chatroom number to enter; array of users in chatrooms; pid()
int add_client(int chatroom, int * chatrooms, int client_socket){
  int slot = chatroom;
  printf("chatroom: %d\n", chatroom);
  // // chatroom format 0 1 2 0 1 2 0 1 2
  // int col = 1;
  // while(chatrooms[slot] && slot< NUM_CHATS * NUM_CLIENTS){
  //   slot = col * NUM_CHATS + chatroom;
  //   col++;
  // }
  // chatroom format 0 0 0 1 1 1 2 2 2
  printf("%d\n", NUM_CLIENTS);
  slot = chatroom * NUM_CLIENTS;
  int increment = 0;
  while(increment < NUM_CLIENTS && chatrooms[slot]){
    increment++;
    slot += increment;
  }
  printf("slot: %d\n", slot);
  chatrooms[slot] = client_socket;
  return slot;
}

// void subserver(int client_socket) {
//   char buffer[BUFFER_SIZE];
//
//   while (read(client_socket, buffer, sizeof(buffer))) {
//
//     printf("[subserver %d] received: [%s]\n", getpid(), buffer);
//     process(buffer);
//     write(client_socket, buffer, sizeof(buffer));
//   }//end read loop
//   close(client_socket);
//   exit(0);
// }
