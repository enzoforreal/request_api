#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

//#define APIKey "YOUR_SPOONACULAR_API_KEY"

struct MemoryStruct
{
    char *memory;
    size_t size;
};

int get_recipes_json(int number);
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
void print_recipes(const char *json_string);

int main()
{
    int number = 5; // Nombre de recettes à récupérer

    int success = get_recipes_json(number);
    if (!success)
    {
        printf("Error getting recipes\n");
        return 1;
    }

    return 0;
}

int get_recipes_json(int number)
{
    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "curl_easy_init() failed\n");
        return 0;
    }

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    char url[200];
    snprintf(url, sizeof(url), "http://localhost:8000/recipes/random?number=%d", number);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 0;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200)
    {
        printf("Error: HTTP status code %ld\n", http_code);
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return 0;
    }

    print_recipes(chunk.memory);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    free(chunk.memory);

    return 1;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL)
    {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void print_recipes(const char *json_string)
{
    cJSON *json = cJSON_Parse(json_string);
    if (!json)
    {
        printf("Error parsing JSON\n");
        return;
    }

    int array_size = cJSON_GetArraySize(json);
    printf("Recipes:\n");

    for (int i = 0; i < array_size; i++)
    {
        cJSON *item = cJSON_GetArrayItem(json, i);
        cJSON *id = cJSON_GetObjectItem(item, "id");
        cJSON *name = cJSON_GetObjectItem(item, "name");
        printf("%d: %s\n", id->valueint, name->valuestring);
    }

    cJSON_Delete(json);
}
