#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

#pragma region Data model
struct Group {
    char* id;
    char* creatorId;
    int countOfParticipants;
    char** participants;
    char* name;
};

void groupDestructor(struct Group* group) {
    free(group->id);
    free(group->creatorId);
    for (int i = 0; i < group->countOfParticipants; i += 1)
        free(group->participants[i]);
    free(group->participants);
}

struct GroupList {
    int count;
    struct Group** list;
};

void groupListDestructor(struct GroupList* groupList) {
    for (int i = 0; i < groupList->count; i += 1)
        groupDestructor(groupList->list + i);
    free(groupList->list);
}

struct Message {
    char* id;
    char* fromId;
    char* toId;
    char* text;
};

void messageDestructor(struct Message* message) {
    free(message->id);
    free(message->toId);
    free(message->fromId);
    free(message->text);
}

struct MessageList {
    int count;
    struct Message* list;
};

void messageListDestructor(struct MessageList* messageList) {
    for (int i = 0; i < messageList->count; i += 1)
        messageDestructor(messageList->list + i);
    free(messageList->list);
}

struct User {
    char* id;
    char* phone;
    char* username;
    char* name;
    char* surname;
    char* biography;
};

void userDestructor(struct User* user) {
    free(user->id);
    free(user->phone);
    free(user->username);
    free(user->name);
    free(user->surname);
    free(user->biography);
}

struct UserList {
    int count;
    struct User* list;
};

void groupListDestructor(struct UserList* userList) {
    for (int i = 0; i < userList->count; i += 1)
        userDestructor(userList->list + i);
    free(userList->list);
}
#pragma endregion
#pragma region Function headers
int connectToDb(const char*);
int disconnectFromDb();

int addUserToGroup(char*, char*);
char* createGroup(struct Group* newGroup);
struct Group* getGroupInfo(char*);
struct GroupList* getUserGroups(char*);
int removeGroup(char*);

int addToContacts(char*, char*);
int checkUserDetails(char*);
struct UserList* getContacts(char*);
struct User* getUser(char*);
char* registerUser(struct User* newUser);
int removeUser(char*);
int removeFromContacts(char*, char*);

int clearHistory(char*, char*);
struct MessageList* getMessages(char*, char*);
char* saveMessage(struct Message* sentMessage);
int removeMessage(char*, char*, char*);
struct Message* resendMesssage(char*, char*, char*);
#pragma endregion