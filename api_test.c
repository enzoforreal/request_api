#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#include "api.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

int main() {
  char *json = get_users_json();
  if (!json) {
    printf("Error getting users\n");
    return 1;
  }

  print_users(json);
  free(json);

  int user_id = post_user_json("John Doe");
  if (user_id < 0) {
    printf("Error creating user\n");
    return 1;
  }

  json = get_users_json(); // Mettre à jour la liste des utilisateurs
  if (!json) {
    printf("Error getting users\n");
    return 1;
  }
  print_users(json);
  free(json);

  char *update_res = put_user_json(user_id, "Jane Doe");
  printf("%s\n", update_res);

  json = get_users_json(); // Mettre à jour la liste des utilisateurs
  if (!json) {
    printf("Error getting users\n");
    return 1;
  }
  print_users(json);
  free(json);

  char *delete_res = delete_user_json(user_id);
  printf("%s\n", delete_res);

  json = get_users_json(); // Mettre à jour la liste des utilisateurs
  if (!json) {
    printf("Error getting users\n");
    return 1;
  }
  print_users(json);
  free(json);

  return 0;
}





static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

char *get_users_json() {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "curl_easy_init() failed\n");
    return NULL;
  }

  struct MemoryStruct chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/users");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return NULL;
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code != 200) {
    printf("Error: HTTP status code %ld\n", http_code);
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return NULL;
  }

  cJSON *json = cJSON_Parse(chunk.memory);
  if (!json) {
    printf("Error parsing JSON\n");
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return NULL;
  }

  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return chunk.memory;
}


void print_users(const char *json_string) {
  cJSON *json = cJSON_Parse(json_string);
  if (!json) {
    printf("Error parsing JSON\n");
    return;
  }

  int array_size = cJSON_GetArraySize(json);
  printf("Users:\n");

  for (int i = 0; i < array_size; i++) {
    cJSON *item = cJSON_GetArrayItem(json, i);
    cJSON *id = cJSON_GetObjectItem(item, "id");
    cJSON *name = cJSON_GetObjectItem(item, "name");
    printf("%d: %s\n", id->valueint, name->valuestring);
  }

  cJSON_Delete(json);
}

int post_user_json(const char *name) {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "curl_easy_init() failed\n");
    return -1;
  }

  char *url = "http://localhost:8000/users";
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "name", name);
  char *json_str = cJSON_Print(json);
  cJSON_Delete(json);

  struct MemoryStruct chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      free(chunk.memory);
      return -1;
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  if (http_code != 201) {
      printf("Error creating user: HTTP status code %ld\n", http_code);
      curl_easy_cleanup(curl);
      free(chunk.memory);
      return -1;
  }

  cJSON *json_response = cJSON_Parse(chunk.memory);
  if (!json_response) {
    printf("Error parsing JSON response\n");
    free(chunk.memory);
    return -1;
  }

  cJSON *id = cJSON_GetObjectItem(json_response, "id");
  if (!id) {
    printf("Error parsing JSON response\n");
    cJSON_Delete(json_response);
    free(chunk.memory);
    return -1;
  }

  int user_id = id->valueint;

  printf("User created successfully with ID %d\n", user_id);

  cJSON_Delete(json_response);
  curl_easy_cleanup(curl);
  curl_global_cleanup();
  free(chunk.memory);

  return user_id;
}


char *put_user_json(int id, const char *name) {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "curl_easy_init() failed\n");
    return NULL;
  }

  char url[50];
  sprintf(url, "http://localhost:8000/users/%d", id);
  cJSON *json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "name", name);
  char *json_str = cJSON_Print(json);
  cJSON_Delete(json);

  struct MemoryStruct chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return "Error updating user";
  }

  //printf("User updated successfully\n");

  long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
      printf("Error: HTTP status code %ld\n", http_code);
      curl_easy_cleanup(curl);
      free(chunk.memory);
      return NULL;
    }

  curl_easy_cleanup(curl);
  curl_global_cleanup();

  free(chunk.memory);

  return "User updated successfully";
}

char *delete_user_json(int id) {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "curl_easy_init() failed\n");
    return NULL;
  }

  char url[50];
  sprintf(url, "http://localhost:8000/users/%d", id);

  struct MemoryStruct chunk;
  chunk.memory = malloc(1);
  chunk.size = 0;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    free(chunk.memory);
    return "Error deleting user";
  }

  //printf("User deleted successfully\n");

  long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
      printf("Error: HTTP status code %ld\n", http_code);
      curl_easy_cleanup(curl);
      free(chunk.memory);
      return NULL;
    }

  curl_easy_cleanup(curl);
  curl_global_cleanup();

  free(chunk.memory);

  return "User deleted successfully";
}