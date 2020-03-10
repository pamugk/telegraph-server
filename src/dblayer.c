#include "dblayer.h"

static PGconn* conn;

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
    user->id = PQgetvalue(res, i, 0);
    user->phone = PQgetvalue(res, i, 1);
    user->username = PQgetvalue(res, i, 2);
    user->name = PQgetvalue(res, i, 3);
    user->surname = PQgetvalue(res, i, 4);
    user->biography = PQgetvalue(res, i, 5);
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
    outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

char* createGroup(struct Group* newGroup) {
    const char* query = 
        "INSERT INTO public.groups(name) VALUES ($1) RETURNING id;";
    const char* params[] = { newGroup->name };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    char* groupId = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        groupId = PQgetvalue(res, 0, 0);
        PQclear(res);
        for (int i = 0; i < newGroup->countOfParticipants; i += 1)
            addUserToGroup(groupId, newGroup->participants[i]);
    }
    PQclear(res);
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
    struct Group* group = (struct Group*) malloc(sizeof(struct Group));
    group->id = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        group->countOfParticipants = PQntuples(res);
        group->participants = (char**)calloc(group->countOfParticipants, sizeof(char*));
        for(int i = 0; i < group->countOfParticipants; i += 1) {
            group->id = PQgetvalue(res, i, 0);
            group->name = PQgetvalue(res, i, 1);
            group->participants[i] = PQgetvalue(res, i, 2);
        }
    }
    PQclear(res);
    return group;
}

struct GroupList* getUserGroups(char* userId) {
    const char* query =
        "SELECT group_id FROM users_of_groups WHERE user_id = $1;";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    struct GroupList* groups = (struct GroupList*)malloc(sizeof(struct GroupList));
    groups->count = -1;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        groups->count = PQntuples(res);
        PQclear(res);
        groups->list = (struct Group**)calloc(groups->count, sizeof(struct Group*));
        for (int i = 0; i < groups->count; i += 1)
            groups->list[i] = getGroupInfo(PQgetvalue(res, i, 0));
    }
    PQclear(res);
    return groups;
}

int removeGroup(char* groupId) {
    const char* query = 
        "DELETE FROM public.groups WHERE id=$1;";
    const char* params[] = { groupId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
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
    outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

int checkUserDetails(char* userId) {
    const char* query = 
        "SELECT EXISTS(SELECT * FROM public.users WHERE id=$1);";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
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
            "(SELECT contact_id FROM public.contacts WHERE userId = $1) "
        "SELECT id, phone, username, name, surname, biography "
        "FROM ctcts JOIN public.users ON contact_id = users.id;";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    struct UserList* users = (struct UserList*)malloc(sizeof(struct UserList));
    users->count = -1;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        users->count = PQntuples(res);
        users->list = (struct User**)calloc(users->count, sizeof(struct User*));
        for(int i = 0; i < users->count; i += 1)
            fillUser(users->list + i, i, res);
    }
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
    struct User* user = (struct User*)malloc(sizeof(struct User));
    user->id = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
        fillUser(user, 0, res);
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
    char* generatedId = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
        generatedId = PQgetvalue(res, 0, 0);
    PQclear(res);
    return generatedId;
}

int removeUser(char* userId) {
    const char* query = "DELETE FROM public.users WHERE id=$1;";
    const char* params[] = { userId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
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
    int outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

struct Message* getMessage(char* messageId) {
    const char* query = 
        "SELECT id, text FROM public.messages WHERE id = $1;";
    const char* params[] = { messageId };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    struct Message* message = (struct Message*)malloc(sizeof(struct Message));
    message->id = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK){
        message->id = PQgetvalue(res, 0, 0);
        message->text = PQgetvalue(res, 0, 1);
    }
    PQclear(res);
    return message;
}

struct MessageList* getMessages(char* fromId, char* toId) {
    const char* query = 
        "WITH msgs AS (SELECT * FROM public.sent_messages "
            "WHERE (from_id=$1 AND to_id=$2) OR (from_id=$2 AND to_id=$1)) "
        "SELECT id, from_id, to_id, text "
        "FROM msgs JOIN public.messages ON msgs.message_id = messages.id;";
    const char* params[] = { fromId, toId };
    PGresult* res = PQexecParams(conn, query, 2, NULL, params, NULL, NULL, 0);
    struct MessageList* messages = (struct MessageList*)malloc(sizeof(struct MessageList*));
    messages->count = -1;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        messages->count = PQntuples(res);
        messages->list = (struct Message**)calloc(messages->count, sizeof(struct Message*));
        for(int i = 0; i < messages->count; i += 1)
            fillMessage(messages->list + i, i, res);
    }
    PQclear(res);
    return messages;
}

char* saveMessage(struct Message* message) {
    const char* query = 
        "INSERT INTO public.messages(text) "
	        "VALUES ($1) RETURNING id;";
    const char* params[] = { message->text };
    PGresult* res = PQexecParams(conn, query, 1, NULL, params, NULL, NULL, 0);
    char* messageId = NULL;
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        messageId = PQgetvalue(res, 0, 0);
        PQclear(res);
        struct Message* msg = resendMesssage(message->fromId, message->toId, messageId);
        messageDestructor(msg);
    }
    PQclear(res);
    return messageId;
}

int removeMessage(char* fromId, char* toId, char* messageId) {
    const char* query =
        "DELETE FROM public.sent_messages "
        "WHERE message_id=$1 AND from_id=$2 AND to_id=$3;";
    const char* params[] = { messageId, fromId, toId };
    PGresult* res = PQexecParams(conn, query, 3, NULL, params, NULL, NULL, 0);
    int outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    return outcome;
}

struct Message* resendMesssage(char* fromId, char* toId, char* messageId) {
    struct Message* resentMessage = getMessage(messageId);
    if (resentMessage->id == NULL)
        return resentMessage;
    resentMessage->fromId = fromId;
    resentMessage->toId = toId;
    const char* query =
        "INSERT INTO public.sent_messages(message_id, from_id, to_id) "
	    "VALUES ($1, $2, $3);";
    const char* params[] = { messageId, fromId, toId };
    PGresult* res = PQexecParams(conn, query, 3, NULL, params, NULL, NULL, 0);
    int outcome = PQresultStatus(res) == PGRES_COMMAND_OK ? 0 : 1;
    PQclear(res);
    if (outcome == 1)
        resentMessage->id = NULL;
    return resentMessage;
}
#pragma endregion