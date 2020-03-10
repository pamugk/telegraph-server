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

void groupDestructor(struct Group*);

struct GroupList {
    int count;
    struct Group** list;
};

void groupListDestructor(struct GroupList*);

struct Message {
    char* id;
    char* fromId;
    char* toId;
    char* text;
};

void messageDestructor(struct Message*);

struct MessageList {
    int count;
    struct Message** list;
};

void messageListDestructor(struct MessageList*);

struct User {
    char* id;
    char* phone;
    char* username;
    char* name;
    char* surname;
    char* biography;
};

void userDestructor(struct User*);

struct UserList {
    int count;
    struct User** list;
};

void userListDestructor(struct UserList*);
#pragma endregion
#pragma region Function headers
int connectToDb(const char*);
int disconnectFromDb();

int addUserToGroup(char*, char*);
char* createGroup(struct Group*);
struct Group* getGroupInfo(char*);
struct GroupList* getUserGroups(char*);
int removeGroup(char*);

int addToContacts(char*, char*);
int checkUserDetails(char*);
struct UserList* getContacts(char*);
struct User* getUser(char*);
char* registerUser(struct User*);
int removeUser(char*);
int removeFromContacts(char*, char*);

int clearHistory(char*, char*);
struct MessageList* getMessages(char*, char*);
char* saveMessage(struct Message*);
int removeMessage(struct Message*);
struct Message* resendMesssage(struct Message*);
#pragma endregion