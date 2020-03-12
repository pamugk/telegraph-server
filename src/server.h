enum ServerOperations {
    ADD_CONTACT,
    ADD_USER_TO_GROUP,
    CLEAR_HISTORY,
    CREATE_GROUP,
    DISCONNECT,
    GET_CONTACTS,
    GET_GROUP_INFO,
    GET_MESSAGES,
    GET_USER,
    GET_USER_GROUPS,
    LOGIN,
    REGISTER_USER,
    REMOVE_GROUP,
    REMOVE_MESSAGE,
    REMOVE_USER,
    RESEND_MESSAGE,
    SEND_MESSAGE
};

enum ServerNotifications {
    NEW_MESSAGE,
    REMOVED_GROUP,
    REMOVED_MESSAGE,
    REMOVED_USER,
    SHUTDOWN
};

enum ServerResponses {
    SUCCESS,
    FAILURE,
    RESTRICTED
};

int doSendMessage(int, struct Message*, int);