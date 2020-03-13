#include "main.h"

#pragma region Multithreading management
pthread_mutex_t lock;

unsigned long countOfConnectedUsers;
unsigned long maxCountOfConnectedUsers;
char** connectedUsers;
int* connectedSockets;
static int limitReached;

void setupSemaphores() {
	printf("Setting up semaphore\n");
}

void sendShutdownNotification(int nsock, enum ServerNotifications notification) {
	printf("Sending shutdown notification\n");
	send(nsock, &notification, sizeof(enum ServerNotifications), 0);
	if (close(nsock) == -1)
		perror("close");
	printf("Done\n");
}

void stopServer(int sig) {
	pthread_mutex_lock(&lock);
	printf("Shutting the server down.\n");
	for (int i = 0UL; i < countOfConnectedUsers; i += 1UL) {
		free(connectedUsers[i]);
		sendShutdownNotification(connectedSockets[i], SHUTDOWN);
	}
	free(connectedUsers);
	free(connectedSockets);
	pthread_mutex_unlock(&lock);
	printf("Server is down, long live the server.\n");
	exit(0);
}

int sendNotificationToUser(char* userId, enum ServerNotifications notification, void* data) {
	printf("Sending notification to user %s\n", userId);
	pthread_mutex_lock(&lock);
	unsigned long i = 0;
	for (i; i < countOfConnectedUsers; i += 1UL) {
		printf("Checking user %s...\n", connectedUsers[i]);
		if (strcmp(connectedUsers[i], userId) == 0)
			break;
	}
	if (i == countOfConnectedUsers) {
		printf("Target not found\n");
		pthread_mutex_unlock(&lock);
		return 1;
	}
	int outcome = 0;
	switch (notification)
	{
		case NEW_MESSAGE:
		{
			printf("Sending notification about new message to \n");
			send(connectedSockets[i], &notification, sizeof(enum ServerNotifications), 0);
			doSendMessage(connectedSockets[i], (struct Message*)data, 0);
			break;
		}
		default: {
			printf("Some unknown notification type\n");
			outcome = 1;
			break;
		}
	}
	pthread_mutex_unlock(&lock);
	printf("Dong\n");
	return outcome;
}

int addNewCallback(char* userId, int notifierSocket) {
	printf("Adding a new callback\n");
	pthread_mutex_lock(&lock);
	printf("Current count of users: %u\n", countOfConnectedUsers);
	if (countOfConnectedUsers == maxCountOfConnectedUsers) {
		if (limitReached == 1)
			return 1;
		unsigned long newMaxCount = maxCountOfConnectedUsers * 2UL;
		limitReached = newMaxCount == ULONG_MAX ? 1 : 0;
		char** expandedUsers = (char**)calloc(newMaxCount, sizeof(char*));
		int* expandedSockets = (int*)calloc(newMaxCount, sizeof(int));
		pthread_t* expandedThreads= (pthread_t*)calloc(newMaxCount, sizeof(pthread_t));
		memcpy(expandedUsers, connectedUsers, countOfConnectedUsers * sizeof(char*));
		memcpy(expandedSockets, connectedSockets, countOfConnectedUsers * sizeof(int));
		void* tmp;
		tmp = connectedUsers;
		connectedUsers = expandedUsers;
		free(tmp);
		tmp = connectedSockets;
		connectedSockets = expandedSockets;
		free(tmp);
	}
	connectedUsers[countOfConnectedUsers] = userId;
	connectedSockets[countOfConnectedUsers] = notifierSocket;
	countOfConnectedUsers += 1UL;
	printf("New count of users: %u\n", countOfConnectedUsers);
	pthread_mutex_unlock(&lock);
	printf("Donk\n");
	return 0;
}

void removeCallback(int notifierSocket) {
	printf("Removing a callback\n");
	pthread_mutex_lock(&lock);
	unsigned long i = 0;
	sendShutdownNotification(notifierSocket, SHUTDOWN);
	while (i < countOfConnectedUsers && connectedSockets[i] != notifierSocket) i += 1;
	if (i == countOfConnectedUsers) {
		printf("Removed socket not found\n");
		pthread_mutex_unlock(&lock);
		return;
	}
	free(connectedUsers[i]);
	unsigned long newCountOfUsers = countOfConnectedUsers - 1UL;
	for (i; i < newCountOfUsers; i += 1UL) {
		connectedUsers[i] = connectedUsers[i + 1UL];
		connectedSockets[i] = connectedSockets[i + 1UL];
	}
	connectedUsers[i] = NULL;
	connectedSockets[i] = -1;
	countOfConnectedUsers = newCountOfUsers;
	pthread_mutex_unlock(&lock);
	printf("Donq\n");
}

void setupMultithreadingPart() {
	countOfConnectedUsers = 0UL;
	maxCountOfConnectedUsers = 256UL;
	connectedUsers = (char**)calloc(maxCountOfConnectedUsers, sizeof(char*));
	connectedSockets = (int*)calloc(maxCountOfConnectedUsers, sizeof(int));
	limitReached = 0;

	static struct sigaction act;
	act.sa_handler = stopServer;
	sigfillset(&(act.sa_mask));
	sigaction(SIGINT, &act, NULL);
}
#pragma endregion
#pragma region Setup
struct Settings loadSettings() {
	printf("Loading settings\n");
    struct Settings s;
    FILE* settingsFile = fopen("res/settings.txt", "r");
    if (settingsFile == NULL) {
        perror("fopen");
        exit(1);
    }
    fscanf(settingsFile, "%d%s", &s.port, s.connStr);
    fclose(settingsFile);
	printf("Done\n");
    return s;
}

int setupServer(struct Settings s) {
	printf("Setting up server socket\n");
    int sockfd;
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(s.port);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(1);
	}
	if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) == -1) {
		perror("bind error");
		exit(1);
	}
	if (listen(sockfd, 5) == -1) {
		perror("listen error");
		exit(1);
	}
	printf("Done\n");
    return sockfd;
}
#pragma endregion
#pragma region Auxillary functions
char* doRecieveStr(int sockfd) {
    int8_t isNull;
	int res = recv(sockfd, &isNull, sizeof(int8_t), 0);
    char* str = NULL;
	if (isNull != 0) {
        size_t size;
        recv(sockfd, &size, sizeof(size_t), 0);
        str = calloc(size, sizeof(char));
        recv(sockfd, str, size * sizeof(char), 0);
	}
    return str;
}

struct Group* doRecieveGroup(int nsock) {
    struct Group* group = (struct Group*) malloc(sizeof(struct Group));
	group->id = doRecieveStr(nsock);
	group->creatorId = doRecieveStr(nsock);
	group->name = doRecieveStr(nsock);
    recv(nsock, &group->countOfParticipants, sizeof(int), 0);
    for (int i = 0; i < group->countOfParticipants; i += 1)
        group->participants[i] = doRecieveStr(nsock);
	return group;
}

struct Message* doRecieveMessage(int nsock, int recieveHeaderOnly) {
	struct Message* message = (struct Message*) malloc(sizeof(struct Message));
	message->id = doRecieveStr(nsock);
	message->fromId = doRecieveStr(nsock);
	message->toId = doRecieveStr(nsock);
    if (recieveHeaderOnly == 0)
	    message->text = doRecieveStr(nsock);
	return message;
}

struct User* doRecieveUser(int sockfd) {
    struct User* user = (struct User*) malloc(sizeof(struct User));
    user->id = doRecieveStr(sockfd);
    user->phone = doRecieveStr(sockfd);
    user->username = doRecieveStr(sockfd);
    user->name = doRecieveStr(sockfd);
    user->surname = doRecieveStr(sockfd);
    user->biography = doRecieveStr(sockfd);
    return user;
}


int doSendStr(int nsock, const char* str) {
    int res = 0;
    int8_t isNull = str == NULL ? 0 : 1;
	res = send(nsock, &isNull, sizeof(int8_t), 0);
	if (isNull != 0) {
		size_t size = strlen(str) + 1;
		res = send(nsock, &size, sizeof(size_t), 0);
        if (res == -1){
            perror("send");
            return 1;
        }
		res = send(nsock, str, size * sizeof(char), 0);
        if (res == -1){
            perror("send");
            return 1;
        }
	}
	return 0;
}

int doSendGroup(int nsock, struct Group* group) {
	doSendStr(nsock, group->id);
	doSendStr(nsock, group->creatorId);
	doSendStr(nsock, group->name);
    send(nsock, &group->countOfParticipants, sizeof(int), 0);
    for (int i = 0; i < group->countOfParticipants; i += 1)
        doSendStr(nsock, group->participants[i]);
	return 0;
}

int doSendGroups(int nsock, struct GroupList* groups) {
    send(nsock, &groups->count, sizeof(int), 0);
    for (int i = 0; i < groups->count; i += 1)
        doSendGroup(nsock, groups->list[i]);
    return 0;
}

int doSendMessage(int nsock, struct Message* message, int sendHeaderOnly) {
	doSendStr(nsock, message->id);
	doSendStr(nsock, message->fromId);
	doSendStr(nsock, message->toId);
    if (sendHeaderOnly == 0)
	    doSendStr(nsock, message->text);
	return 0;
}

int doSendMessages(int nsock, struct MessageList* messages) {
    send(nsock, &messages->count, sizeof(int), 0);
    for (int i = 0; i < messages->count; i += 1)
        doSendMessage(nsock, messages->list[i], 0);
    return 0;
}

int doSendUser(int nsock, struct User* user) {
	doSendStr(nsock, user->id);
	doSendStr(nsock, user->phone);
	doSendStr(nsock, user->username);
	doSendStr(nsock, user->name);
	doSendStr(nsock, user->surname);
	doSendStr(nsock, user->biography);
	return 0;
}

int doSendUsers(int nsock, struct UserList* users) {
    printf("Sending users\n");
	send(nsock, &(users->count), sizeof(int), 0);
    for (int i = 0; i < users->count; i += 1)
        doSendUser(nsock, users->list[i]);
    return 0;
}
#pragma endregion
#pragma region Server functions
int srvAddContact(int nsock, char* loggedInUserId) {
	printf("Adding contact.\n");
    char* contactId = doRecieveStr(nsock);
    enum ServerResponses response = 
		addToContacts(loggedInUserId, contactId) == 0 ? SUCCESS : FAILURE;
	free(contactId);
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
        perror("send");
		return 1;
    }
	if (response == SUCCESS)
        printf("Done.\n");
    else
    	printf("Failure\n");
    return 0;
}

int srvAddUserToGroup(int nsock, char* loggedInUserId) {
	printf("Adding user to group.\n");
	char* groupId = doRecieveStr(nsock);
    char* userId = doRecieveStr(nsock);
    enum ServerResponses response = addUserToGroup(groupId, userId) == 0 ? SUCCESS : FAILURE;
    free(groupId);
	free(userId);
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
        perror("send");
		return 1;
    }
	if (response == SUCCESS)
        printf("Done.\n");
    else
    	printf("Failure\n");
    return 0;
}

int srvClearHistory(int nsock, char* loggedInUserId) {
	printf("Clearing history of messages.\n");
    char* toId = doRecieveStr(nsock);
    enum ServerResponses response = clearHistory(loggedInUserId, toId) == 0 ? SUCCESS : FAILURE;
	free(toId);
    int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
        perror("send");
		return 1;
    }
	if (response == SUCCESS)
        printf("Done.\n");
    else
    	printf("Failure\n");
    return 0;
}

int srvCreateGroup(int nsock, char* loggedInUserId) {
	printf("Creating a new group.\n");
	struct Group* group = doRecieveGroup(nsock);
	char* createdGroupId = createGroup(group);
	groupDestructor(group);
    enum ServerResponses response = createdGroupId == NULL ? FAILURE : SUCCESS;
    int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
	if (res == -1) {
		perror("send");
		free(createdGroupId);
		return 1;
	}
    if (response == SUCCESS) {
        doSendStr(nsock, createdGroupId);
        printf("Done.\n");
		free(createdGroupId);
    }
	else
    	printf("Failure.\n");
	return 0;
}

int srvGetContacts(int nsock, char* loggedInUserId) {
	printf("Collecting contacts.\n");
    struct UserList* contacts = getContacts(loggedInUserId);
	enum ServerResponses response = contacts == NULL ? FAILURE : SUCCESS; 
    int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
        perror("send");
		if (contacts != NULL)
			userListDestructor(contacts);
		return 1;
    }
	if (response == SUCCESS) {
		doSendUsers(nsock, contacts);
		userListDestructor(contacts);
		free(contacts);
	}
    return 0;
}

int srvGetGroupInfo(int nsock, char* loggedInUserId) {
	printf("Getting group info.\n");
    char* groupId = doRecieveStr(nsock);
    struct Group* group = getGroupInfo(groupId);
    enum ServerResponses response = group == NULL ? FAILURE : SUCCESS;
	free(groupId);
    int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
        perror("send");
		groupDestructor(group);
		return 1;
    }
	if (response == SUCCESS) {
		doSendGroup(nsock, group);
		groupDestructor(group);
	}
	return 0;
}

int srvGetMessages(int nsock, char* loggedInUserId) {
	printf("Collecting messages.\n");
    char* fromId = doRecieveStr(nsock);
    struct MessageList* messages = getMessages(fromId, loggedInUserId);
    free(fromId);
	enum ServerResponses response = messages == NULL ? FAILURE : SUCCESS; 
    int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
        perror("send");
		if (messages != NULL)
			messageListDestructor(messages);
		return 1;
    }
	if (response == SUCCESS) {
		doSendMessages(nsock, messages);
		messageListDestructor(messages);
	}
    return 0;
}

int srvGetUser(int nsock, char* loggedInUserId) {
	printf("Getting a user.\n");
	char* userId = doRecieveStr(nsock);
	struct User* user = getUser(userId);
	free(userId);
	enum ServerResponses response = user == NULL ? FAILURE : SUCCESS;
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
	if (res == -1) {
        perror("send");
		if (user != NULL)
			userDestructor(user);
		return 1;
    }
	if (response == SUCCESS) {
		doSendUser(nsock, user);
		userDestructor(user);
	}
	return 0;
}

int srvGetUserGroups(int nsock, char* loggedInUserId) {
	printf("Collecting groups.\n");
    struct GroupList* groups = getUserGroups(loggedInUserId);
	enum ServerResponses response = groups == NULL ? FAILURE : SUCCESS; 
    int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
        perror("send");
		if (groups != NULL)
			groupListDestructor(groups);
		return 1;
    }
	if (response == SUCCESS) {
		doSendGroups(nsock, groups);
		groupListDestructor(groups);
	}
    return 0;
}

char* srvLogin(int nsock) {
	printf("Logging in...\n");
	char* userId = doRecieveStr(nsock);
	printf("%s\n", userId);
	struct User* user = getUser(userId);
	enum ServerResponses response = 
		user == NULL ? FAILURE : SUCCESS;
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
	if (res == -1) {
        perror("send");
		if (user != NULL)
			userDestructor(user);
		return NULL;
    }
	if (response == SUCCESS) {
		doSendUser(nsock, user);
		free(user);
		printf("Done\n");
	}
	else {
		printf("FAILURE\n");
		free(userId);
		userId = NULL;
	}
	return userId;
}

void srvLogout(int nsock) {
	printf("Disconnecting...\n");
}

int srvRegisterUser(int nsock) {
	printf("Registering a new user.\n");
	struct User* user = doRecieveUser(nsock);
	char* registeredUserId = registerUser(user);
	userDestructor(user);
    enum ServerResponses response = registeredUserId == NULL ? FAILURE : SUCCESS;
    int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
	if (res == -1) {
		perror("send");
		free(registeredUserId);
		return 1;
	}
    if (response == SUCCESS) {
        doSendStr(nsock, registeredUserId);
        printf("Done.\n");
		free(registeredUserId);
    }
	else
    	printf("Failure.\n");
	return 0;
}

int srvRemoveGroup(int nsock, char* loggedInUserId) {
    printf("Removing a group.\n");
    char* groupId = doRecieveStr(nsock);
    enum ServerResponses response = removeGroup(groupId) == 0 ? SUCCESS : FAILURE;
	free(groupId);
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
		perror("send");
		return 1;
	}
	if (response == SUCCESS)
        printf("Done.\n");
	else
    	printf("Failure.\n");
	return 0;
}

int srvRemoveMessage(int nsock, char* loggedInUserId) {
	printf("Removing a message.\n");
    struct Message* message = doRecieveMessage(nsock, 1);
    enum ServerResponses response = removeMessage(message) == 0 ? SUCCESS : FAILURE;
	messageDestructor(message);
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
		perror("send");
		return 1;
	}
	if (response == SUCCESS)
        printf("Done.\n");
	else
    	printf("Failure.\n");
    return 0;
}

int srvRemoveUser(int nsock, char* loggedInUserId) {
    printf("Removing a user.\n");
    enum ServerResponses response = removeUser(loggedInUserId) == 0 ? SUCCESS : FAILURE;
    int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
		perror("send");
		return 1;
	}
	if (response == SUCCESS) {
        printf("Done.\n");
    }
	else
    	printf("Failure.\n");
	return 0;
}

int srvResendMessage(int nsock, char* loggedInUserId) {
	printf("Resending a message.\n");
    struct Message* message = doRecieveMessage(nsock, 1);
    struct Message* msg = resendMesssage(message);
    enum ServerResponses response = msg == NULL ? FAILURE : SUCCESS;
	messageDestructor(message);
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
	int outcome = 0;
    if (res == -1) {
		perror("send");
		outcome = 1;
	}
	if (response == SUCCESS){
        printf("Done.\n");
		sendNotificationToUser(msg->toId, NEW_MESSAGE, msg);
	}
	else
    	printf("Failure.\n");
	if (msg != NULL)
		messageDestructor(msg);
    return outcome;
}

int srvSendMessage(int nsock, char* loggedInUserId) {
	printf("Sending a message.\n");
    struct Message* message = doRecieveMessage(nsock, 0);
    char* messageId = saveMessage(message);
	message->id = messageId;
    enum ServerResponses response = messageId == NULL ? FAILURE : SUCCESS;
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
	int outcome = 0;
    if (res == -1) {
		perror("send");
		outcome = 1;
	}
	if (response == SUCCESS) {
		doSendStr(nsock, messageId);
		sendNotificationToUser(message->toId, NEW_MESSAGE, message);
        printf("Done.\n");
	}
	else
    	printf("Failure.\n");
	messageDestructor(message);
    return outcome;
}
#pragma endregion
#pragma region Serverside
struct sockets {
	int recieverSocket;
	int notifierSocket;
};

void* handleClient(void *vargp) {
	pthread_detach(pthread_self());
    enum ServerOperations op;
	struct sockets sockets = *(struct sockets*)vargp;
	ssize_t size;
	int res;
	char* userId;
	int loggedIn = 1;
    do {
		size = recv(sockets.recieverSocket, &op, sizeof(enum ServerOperations), 0);
		if (size == -1) {
			perror("recv");
			break;
		}
		switch (op)
		{
		case ADD_CONTACT: {
			res = userId == NULL ? 1 : srvAddContact(sockets.recieverSocket, userId);
			break;
		}
		case ADD_USER_TO_GROUP: {
			res = userId == NULL ? 1 : srvAddUserToGroup(sockets.recieverSocket, userId);
			break;
		}
		case CLEAR_HISTORY: {
			res = userId == NULL ? 1 : srvClearHistory(sockets.recieverSocket, userId);
			break;
		}
		case CREATE_GROUP: {
			res = userId == NULL ? 1 : srvCreateGroup(sockets.recieverSocket, userId);
			break;
		}
		case DISCONNECT: {
			srvLogout(sockets.recieverSocket);
			break;
		}
		case GET_CONTACTS: {
			res = userId == NULL ? 1 : srvGetContacts(sockets.recieverSocket, userId);
			break;
		}
		case GET_GROUP_INFO: {
			res = userId == NULL ? 1 : srvGetGroupInfo(sockets.recieverSocket, userId);
			break;
		}
		case GET_MESSAGES: {
			res = userId == NULL ? 1 : srvGetMessages(sockets.recieverSocket, userId);
			break;
		}
		case GET_USER: {
			res = userId == NULL ? 1 : srvGetUser(sockets.recieverSocket, userId);
			break;
		}
		case GET_USER_GROUPS: {
			res = userId == NULL ? 1 : srvGetUserGroups(sockets.recieverSocket, userId);
			break;
		}
		case LOGIN: {
			userId = srvLogin(sockets.recieverSocket);
			if (userId == NULL)
				res = 1;
			else  {
				loggedIn = 0;
				addNewCallback(userId, sockets.notifierSocket);
			}
			break;
		}
		case REGISTER_USER: {
			res = srvRegisterUser(sockets.recieverSocket);
			break;
		}
		case REMOVE_GROUP: {
			res = userId == NULL ? 1 : srvRemoveGroup(sockets.recieverSocket, userId);
			break;
		}
		case REMOVE_MESSAGE: {
			res = userId == NULL ? 1 : srvRemoveMessage(sockets.recieverSocket, userId);
			break;
		}
		case REMOVE_USER: {
			res = userId == NULL ? 1 : srvRemoveUser(sockets.recieverSocket, userId);
			if (res == 0)
				op = DISCONNECT;
			break;
		}
		case RESEND_MESSAGE: {
			res = userId == NULL ? 1 : srvResendMessage(sockets.recieverSocket, userId);
			break;
		}
		case SEND_MESSAGE: {
			res = userId == NULL ? 1 : srvSendMessage(sockets.recieverSocket, userId);
			break;
		}
		default: {
			printf("Recieved unknown command\n");
			op = DISCONNECT;
			break;
		}
		}
		if (res == 1)
			op = DISCONNECT;
	} while (op != DISCONNECT);
	close(sockets.recieverSocket);
	if (loggedIn == 0)
		removeCallback(sockets.notifierSocket);
	pthread_exit(NULL);
}

void doWork(int sockfd) {
	for (;;) {
        int recieverSocket = accept(sockfd, NULL, NULL);
		if (recieverSocket == -1) {
			perror("Accept error");
			continue;
		}
		int notifierSocket = accept(sockfd, NULL, NULL);
		if (notifierSocket == -1) {
			perror("Accept error");
			continue;
		}
		printf("Accepted a new client.\n");

    	pthread_t tid;
		struct sockets sockets;
		sockets.recieverSocket = recieverSocket;
		sockets.notifierSocket = notifierSocket;
		int res = pthread_create(&tid, NULL, handleClient, (void *)&sockets);
		if (res == -1)
			perror("pthread_create");
	}
}
#pragma endregion

int main(int argc, char* argv[]) {
    struct Settings s = loadSettings();
    int sockfd = setupServer(s);
	setupSemaphores();
	setupMultithreadingPart();
	connectToDb(s.connStr);
    doWork(sockfd);
	disconnectFromDb();
    return 0;
}