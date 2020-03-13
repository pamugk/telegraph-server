#include "dblayer.h"

static PGconn* conn;

#pragma region Destructors
void groupDestructor(struct Group* group) {
    if (group->id != NULL)
        free(group->id);
    free(group->creatorId);
    for (int i = 0; i < group->countOfParticipants; i += 1)
        free(group->participants[i]);
    free(group->participants);
}

void groupListDestructor(struct GroupList* groupList) {
    for (int i = 0; i < groupList->count; i += 1)
        free(groupList->list[i]);
    free(groupList->list);
}

void messageDestructor(struct Message* message) {
    if (message->id != NULL)
        free(message->id);
    free(message->toId);
    free(message->fromId);
    if (message->text != NULL)
        free(message->text);
}

void messageListDestructor(struct MessageList* messageList) {
    for (int i = 0; i < messageList->count; i += 1)
        free(messageList->list[i]);
    free(messageList->list);
}

void userDestructor(struct User* user) {
    if (user->id != NULL)
        free(user->id);
    free(user->phone);
    free(user->username);
    free(user->name);
    free(user->surname);
    if (user->biography != NULL)
        free(user->biography);
}

void userListDestructor(struct UserList* userList) {
    for (int i = 0; i < userList->count; i += 1)
        free(userList->list[i]);
    free(userList->list);
}
#pragma endregion
#pragma region Utils
static void terminate(int code) {
    if(code != 0)
        fprintf(stderr, "%s\n", PQerrorMessage(conn));
    if(conn != NULL)
        PQfinish(conn);
    exit(code);
}

static void fillMessage(struct Message* message, int i, PGresult* res) {
    message->id = PQgetvalue(res, i, 0);
    message->fromId = PQgetvalue(res, i, 1);
    message->toId = PQgetvalue(res, i, 2);
    message->text = PQgetvalue(res, i, 3);
}

static void fillUser(struct User* user, int i, PGresult* res) {
    user->id = PQgetisnull(res, i, 0) ? NULL : PQgetvalue(res, i, 0);
    user->phone = PQgetisnull(res, i, 1) ? NULL :PQgetvalue(res, i, 1);
    user->username = PQgetisnull(res, i, 2) ? NULL :PQgetvalue(res, i, 2);
    user->name = PQgetisnull(res, i, 3) ? NULL :PQgetvalue(res, i, 3);
    user->surname = PQgetisnull(res, i, 4) ? NULL :PQgetvalue(res, i, 4);
    user->biography = PQgetisnull(res, i, 5) ? NULL : PQgetvalue(res, i, 5);
}
#pragma endregion
#pragma region DB connection management
int connectToDb(const char* connStr){
    printf("Connecting to DB\n");
    conn = PQconnectdb(connStr);
    conn = PQconnectdb(connStr);
    if(PQstatus(conn) != CONNECTION_OK) {
        printf("Connection failed");
        terminate(1);
    }
    printf("Done\n");
}

int disconnectFromDb() {
    terminate(0);
}
#pragma endregion
#pragma region Group management
int addUserToGroup(char* groupId, char* userId) {
    int outcome = checkUserDetails(userId);
    if (outcome != 0)
        return 1;
    const char* query =
        "INSERT INTO public.users_of_groups(group_id, user_id) "
	        "VALUES ($1, $2)";
    const char* params[] = { groupId, userId };
    PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

char* createGroup(struct Group* newGroup) {
    const char* query = 
        "INSERT INTO public.groups(name) VALUES ($1) RETURNING id;";
    const char* params[] = { newGroup->name };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    char* groupId = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        groupId = PQgetvalue(res, 0, 0);
        PQclear(res);
        for (int i = 0; i < newGroup->countOfParticipants; i += 1)
            addUserToGroup(groupId, newGroup->participants[i]);
    }
    else {
        printf("%s\n", PQresultErrorMessage(res));
        PQclear(res);
    }
    return groupId;
}

struct Group* getGroupInfo(char* groupId) {
    const char* query = 
        "WITH grp AS (SELECT id, name "
	        "FROM public.groups WHERE id=$1)"
        "SELECT id, name, user_id "
        "FROM grp JOIN users_of_groups "
	    "ON grp.id = group_id;";
    const char* params[] = { groupId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    struct Group* group = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        group =  (struct Group*) malloc(sizeof(struct Group));
        group->countOfParticipants = PQntuples(res);
        group->participants = (char**)calloc(group->countOfParticipants, sizeof(char*));
        for(int i = 0; i < group->countOfParticipants; i += 1) {
            group->id = PQgetvalue(res, i, 0);
            group->name = PQgetvalue(res, i, 1);
            group->participants[i] = PQgetvalue(res, i, 2);
        }
    }
    else
        printf("%s\n", PQresultErrorMessage(res));
    PQclear(res);
    return group;
}

struct GroupList* getUserGroups(char* userId) {
    const char* query =
        "SELECT group_id FROM users_of_groups WHERE user_id = $1;";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    struct GroupList* groups = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        groups = (struct GroupList*)malloc(sizeof(struct GroupList));
        groups->count = PQntuples(res);
        PQclear(res);
        groups->list = (struct Group**)calloc(groups->count, sizeof(struct Group*));
        for (int i = 0; i < groups->count; i += 1)
            groups->list[i] = getGroupInfo(PQgetvalue(res, i, 0));
    }
    else {
        printf("%s\n", PQresultErrorMessage(res));
        PQclear(res);
    }
    return groups;
}

int removeGroup(char* groupId) {
    const char* query = 
        "DELETE FROM public.groups WHERE id=$1;";
    const char* params[] = { groupId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    int outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}
#pragma endregion
#pragma region User management
int addToContacts(char* userId, char* contactId) {
    int outcome = checkUserDetails(userId);
    if (outcome!= 0)
        return 1;
    outcome = checkUserDetails(contactId);
    if (outcome != 0)
        return 1;
    const char* query = 
        "INSERT INTO public.contacts(user_id, contact_id) "
        "VALUES($1, $2);";
    const char* params[] = { userId, contactId };
    PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

int checkUserDetails(char* userId) {
    const char* query = 
        "SELECT EXISTS(SELECT * FROM public.users WHERE id=$1);";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    int outcome = 
        PQresultStatus(res) == PGRES_TUPLES_OK ?
        strcmp(PQgetvalue(res, 0, 0), "true")
        : 1;
    PQclear(res);
    return outcome;
}

struct UserList* getContacts(char* userId) {
    const char* query =
        "WITH ctcts AS "
            "(SELECT contact_id FROM public.contacts WHERE user_id = $1) "
        "SELECT id, phone, username, name, surname, biography "
        "FROM ctcts JOIN public.users ON contact_id = users.id "
        "ORDER BY surname, name;";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    struct UserList* users = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        users = (struct UserList*)malloc(sizeof(struct UserList));
        users->count = PQntuples(res);
        users->list = (struct User**)calloc(users->count, sizeof(struct User*));
        for(int i = 0; i < users->count; i += 1) {
            users->list[i] = (struct User*)malloc(sizeof(struct User));
            fillUser(users->list[i], i, res);
        }
    }
    else
        printf("%s\n", PQresultErrorMessage(res));
    PQclear(res);
    return users;
}

struct User* getUser(char* userId) {
    const char* query =
        "SELECT id, phone, username, name, surname, biography "
        "FROM public.users WHERE id = $1;";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    struct User* user = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        user = (struct User*)malloc(sizeof(struct User));
        fillUser(user, 0, res);
    }
    else
        printf("%s\n", PQresultErrorMessage(res));
    PQclear(res);
    return user;
}

char* registerUser(struct User* user) {
    const char* query = 
        "INSERT INTO public.users("
            "phone, username, name, surname, biography) "
	    "VALUES ($1, $2, $3, $4, $5) "
        "RETURNING id;";
    const char* params[] = { 
        user->phone, user->username, 
        user->name, user->surname, user->biography 
    };
    PGresult* res = PQexecParams(conn, query, 5, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    char* generatedId = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
        generatedId = PQgetvalue(res, 0, 0);
    else
        printf("%s\n", PQresultErrorMessage(res));
    PQclear(res);
    return generatedId;
}

int removeUser(char* userId) {
    const char* query = "DELETE FROM public.users WHERE id=$1;";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    int outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

int removeFromContacts(char* userId, char* contactId) {
    const char* query = 
        "DELETE FROM public.contacts "
        "WHERE user_id=$1 AND contact_id=$2;";
    const char* params[] = { userId, contactId };
    PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    int outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}
#pragma endregion
#pragma region Message management
int clearHistory(char* fromId, char* toId) {
    const char* query =
        "DELETE FROM public.sent_messages "
        "WHERE from_id=$1 AND to_id=$2 OR from_id=$2 AND to_id=$1;";
    const char* params[] = { fromId, toId };
    PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    int outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

struct Message* getMessage(char* messageId) {
    const char* query = 
        "SELECT id, text FROM public.messages WHERE id = $1;";
    const char* params[] = { messageId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    struct Message* message = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK){
        message = (struct Message*)malloc(sizeof(struct Message));
        message->id = PQgetvalue(res, 0, 0);
        message->text = PQgetvalue(res, 0, 1);
    }
    else
        printf("%s\n", PQresultErrorMessage(res));
    PQclear(res);
    return message;
}

struct MessageList* getMessages(char* fromId, char* toId) {
    const char* query = 
        "WITH msgs AS (SELECT * FROM public.sent_messages "
            "WHERE (from_id=$1 AND to_id=$2) OR (from_id=$2 AND to_id=$1)) "
        "SELECT id, from_id, to_id, text "
        "FROM msgs JOIN public.messages ON msgs.message_id = messages.id "
        "ORDER BY msgs.timestamp;";
    const char* params[] = { fromId, toId };
    PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    struct MessageList* messages = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        messages = (struct MessageList*)malloc(sizeof(struct MessageList*));
        messages->count = PQntuples(res);
        messages->list = (struct Message**)calloc(messages->count, sizeof(struct Message*));
        for(int i = 0; i < messages->count; i += 1) {
            messages->list[i] = (struct Message*)malloc(sizeof(struct Message));
            fillMessage(messages->list[i], i, res);
        }
    }
    else
        printf("%s\n", PQresultErrorMessage(res));
    PQclear(res);
    return messages;
}

char* saveMessage(struct Message* message) {
    const char* query = 
        "INSERT INTO public.messages(text) VALUES ($1) RETURNING messages.id;";
    const char* params[] = { message->text };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    char* messageId = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        messageId = PQgetvalue(res, 0, 0);
        message->id = calloc(strlen(messageId), sizeof(char));
        strcpy(message->id, messageId);
        messageId = message->id;
        PQclear(res);
        struct Message* msg = resendMesssage(message);
        if (msg != NULL)
            free(msg);
        message->id = NULL;
    }
    else {
        printf("%s\n", PQresultErrorMessage(res));
        PQclear(res);
    }
    return messageId;
}

int removeMessage(struct Message* message) {
    const char* query =
        "DELETE FROM public.sent_messages "
        "WHERE message_id=$1 AND from_id=$2 AND to_id=$3;";
    const char* params[] = { message->id, message->fromId, message->toId };
    PGresult* res = PQexecParams(conn, query, 3, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    int outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

struct Message* resendMesssage(struct Message* message) {
    struct Message* resentMessage = getMessage(message->id);
    if (resentMessage == NULL)
        return resentMessage;
    const char* query =
        "INSERT INTO public.sent_messages(message_id, from_id, to_id) "
	    "VALUES ($1, $2, $3);";
    const char* params[] = { message->id, message->fromId, message->toId };
    PGresult* res = PQexecParams(conn, query, 3, NULL, params, NULL, NULL, 0);
    printf("%s\n", PQresStatus(PQresultStatus(res)));
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        printf("%s\n", PQresultErrorMessage(res));
        messageDestructor(resentMessage);
        resentMessage = NULL;
    }
    else
        printf("%s\n", PQresultErrorMessage(res));
    PQclear(res);
    return resentMessage;
}
#pragma endregion