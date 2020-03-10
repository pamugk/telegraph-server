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
		return 1;
	}
	struct User user = getUser(userId);
	enum ServerResponses response = 
		user.id == NULL ? FAILURE : SUCCESS;
	res = send(nsock, &response, sizeof(enum ServerResponses), 0);
	if (res == -1) {
        perror("send");
		return 1;
    }
	if (response == SUCCESS) {
		res = send(nsock, &user, sizeof(user), 0);
		if (res == -1) {
        	perror("send");
			return 1;
    	}
	}
	return 0;
}

void srvRegisterUser(int nsock) {

}

void srvRemoveGroup(int nsock) {

}

void srvRemoveMessage(int nsock) {

}

void srvRemoveUser(int nsock) {

}

void srvResendMessage(int nsock) {

}

void srvSendMessage(int nsock) {

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
			srvRegisterUser(nsock);
			break;
		case REMOVE_GROUP:
			srvRemoveGroup(nsock);
			break;
		case REMOVE_MESSAGE:
			srvRemoveMessage(nsock);
			break;
		case REMOVE_USER:
			srvRemoveUser(nsock);
			break;
		case RESEND_MESSAGE:
			srvResendMessage(nsock);
			break;
		case SEND_MESSAGE:
			srvSendMessage(nsock);
			break;
		default: {
			printf("Recieved unknown command");
			op = DISCONNECT;
			break;
		}
		if (res == 1)
			break;
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