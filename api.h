char* get_users_json(void);
void print_users(const char* json_string);
int post_user_json(const char* name);
char* put_user_json(int id, const char* name);
char* delete_user_json(int id);
