#include "main.h"

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
    int size = 0;
    recv(sockfd, &size, sizeof(int), 0);
    char* str = calloc(size, sizeof(char));
    recv(sockfd, str, size * sizeof(char), 0);
    return str;
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
	int size = strlen(str);
	send(nsock, &size, sizeof(int), 0);
	send(nsock, str, size * sizeof(char), 0);
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
#pragma endregion
#pragma region Server functions
void srvAddContact(int nsock) {

}

void srvAddUserToGroup(int nsock) {

}

void srvClearHistory(int nsock) {

}

void srvCreateGroup(int nsock) {

}

void srvDisconnect(int nsock) {
	printf("Disconnecting...\n");
	close(nsock);
}

void srvGetContacts(int nsock) {

}

void srvGetGroupInfo(int nsock) {

}

void srvGetMessages(int nsock) {

}

void srvGetUser(int nsock) {

}

void srvGetUserGroups(int nsock) {

}

int srvLogin(int nsock) {
	printf("Logging in...\n");
	char* userId = calloc(37, sizeof(char));
	int res = recv(nsock, userId, 37 * sizeof(char), 0);
	if (res == -1) {
		perror("recv");
		free(userId);
		return 1;
	}
	struct User* user = getUser(userId);
	free(userId);
	enum ServerResponses response = 
		user->id == NULL ? FAILURE : SUCCESS;
	res = send(nsock, &response, sizeof(enum ServerResponses), 0);
	if (res == -1) {
        perror("send");
		return 1;
    }
	if (response == SUCCESS)
		doSendUser(nsock, user);
	userDestructor(user);
	return 0;
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

int srvRemoveGroup(int nsock) {
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

int srvRemoveMessage(int nsock) {
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

int srvRemoveUser(int nsock) {
    printf("Removing a user.\n");
    char* userId = doRecieveStr(nsock);
    enum ServerResponses response = removeUser(userId) == 0 ? SUCCESS : FAILURE;
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
	free(userId);
	return 0;
}

int srvResendMessage(int nsock) {
	printf("Resending a message.\n");
    struct Message* message = doRecieveMessage(nsock, 1);
    struct Message* msg = resendMesssage(message);
    enum ServerResponses response = msg->id == NULL ? FAILURE : SUCCESS;
	messageDestructor(msg);
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

int srvSendMessage(int nsock) {
	printf("Sending a message.\n");
    struct Message* message = doRecieveMessage(nsock, 1);
    char* messageId = saveMessage(message);
	messageDestructor(message);
    enum ServerResponses response = messageId == NULL ? FAILURE : SUCCESS;
	int res = send(nsock, &response, sizeof(enum ServerResponses), 0);
    if (res == -1) {
		perror("send");
		free(messageId);
		return 1;
	}
	if (response == SUCCESS) {
		doSendStr(nsock, messageId);
        printf("Done.\n");
		free(messageId);
	}
	else
    	printf("Failure.\n");
    return 0;
}
#pragma endregion
#pragma region Serverside
void handleClient(int nsock) {
    enum ServerOperations op;
	ssize_t size;
	int res;
    do {
		size = recv(nsock, &op, sizeof(enum ServerOperations), 0);
		if (size == -1) {
			perror("recv");
			break;
		}
		switch (op)
		{
		case ADD_CONTACT:
			srvAddContact(nsock);
			break;
		case ADD_USER_TO_GROUP:
			srvAddUserToGroup(nsock);
			break;
		case CLEAR_HISTORY:
			srvClearHistory(nsock);
			break;
		case CREATE_GROUP:
			srvCreateGroup(nsock);
			break;
		case DISCONNECT:
			srvDisconnect(nsock);
			break;
		case GET_CONTACTS:
			srvGetContacts(nsock);
			break;
		case GET_GROUP_INFO:
			srvGetGroupInfo(nsock);
			break;
		case GET_MESSAGES:
			srvGetMessages(nsock);
			break;
		case GET_USER:
			srvGetUser(nsock);
			break;
		case GET_USER_GROUPS:
			srvGetUserGroups(nsock);
			break;
		case LOGIN:
			res = srvLogin(nsock);
			break;
		case REGISTER_USER:
			res = srvRegisterUser(nsock);
			break;
		case REMOVE_GROUP:
			res = srvRemoveGroup(nsock);
			break;
		case REMOVE_MESSAGE:
			res = srvRemoveMessage(nsock);
			break;
		case REMOVE_USER:
			res = srvRemoveUser(nsock);
			break;
		case RESEND_MESSAGE:
			res = srvResendMessage(nsock);
			break;
		case SEND_MESSAGE:
			res = srvSendMessage(nsock);
			break;
		default: {
			printf("Recieved unknown command");
			op = DISCONNECT;
			break;
		}
		}
		if (res == 1){
			close(nsock);
			exit(1);
		}
	} while (op != DISCONNECT);
	close(nsock);
	exit(0);
}

void doWork(int sockfd) {
	for (;;) {
        int nsock = accept(sockfd, NULL, NULL);
		if (nsock == -1) {
			perror("Accept error");
			continue;
		}
		printf("Accepted a new client.\n");
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            continue;
        }
		if (pid == 0)
			handleClient(nsock);
		close(nsock);
	}
}
#pragma endregion

int main(int argc, char* argv[]) {
    struct Settings s = loadSettings();
    int sockfd = setupServer(s);
	connectToDb(s.connStr);
    doWork(sockfd);
	disconnectFromDb();
    return 0;
}