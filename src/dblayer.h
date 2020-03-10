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

struct GroupList {
    int count;
    struct Group* list;
};

struct Message {
    char* id;
    char* fromId;
    char* toId;
    char* text;
};

struct MessageList {
    int count;
    struct Message* list;
};

struct User {
    char* id;
    char* phone;
    char* username;
    char* name;
    char* surname;
    char* biography;
};

struct UserList {
    int count;
    struct User* list;
};
#pragma endregion
#pragma region Function headers
int connectToDb(const char*);
int disconnectFromDb();

int addUserToGroup(char*, char*);
char* createGroup(struct Group newGroup);
struct Group getGroupInfo(char*);
struct GroupList getUserGroups(char*);
int removeGroup(char*);

int addToContacts(char*, char*);
int checkUserDetails(char*);
struct UserList getContacts(char*);
struct User getUser(char*);
char* registerUser(struct User newUser);
int removeUser(char*);
int removeFromContacts(char*, char*);

int clearHistory(char*, char*);
struct MessageList getMessages(char*, char*);
char* saveMessage(struct Message sentMessage);
int removeMessage(char*, char*, char*);
struct Message resendMesssage(char*, char*, char*);
#pragma endregion