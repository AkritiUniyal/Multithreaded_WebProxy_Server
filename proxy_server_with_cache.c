#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>

#define PORT 8080
#define BUFFER_SIZE 1000000
#define CACHE_LIMIT 3
#define a 0.5
#define b 0.5

pthread_mutex_t lock;
int global_time = 0;

struct arc_cache
{
    char url[1000];
    char *data;
    int size;
    int time; // last accessed time (global_time)
    int freq;
    double score;
    struct arc_cache *next;
};

struct arc_cache *head = NULL;
int cache_size = 0;
void print_cache();

// Utility: Delete node with lowest score (for ARC eviction)
void delete_lowest_score_node()
{
    printf("\n[DEBUG] Starting eviction process...\n");

    if (!head)
    {
        pthread_mutex_unlock(&lock);
        return;
    }

    struct arc_cache *curr = head, *prev = NULL;
    struct arc_cache *lowest = head, *lowest_prev = NULL;

    while (curr)
    {
        if (curr->score < lowest->score)
        {
            lowest = curr;
            lowest_prev = prev;
        }
        prev = curr;
        curr = curr->next;
    }

    // Remove lowest from list
    if (lowest_prev == NULL)
    {
        head = lowest->next;
    }
    else
    {
        lowest_prev->next = lowest->next;
    }

    printf("[DEBUG] Evicting URL: %s (Score: %.2f)\n", lowest->url, lowest->score);

    free(lowest->data);
    free(lowest);
    cache_size--;

    printf("[DEBUG] Eviction complete.\n\n");
}

// Add or update cache element (thread-safe)
void add_cache_element(char *data, int size, char *url, int elapsed_time)
{
    pthread_mutex_lock(&lock);

    global_time++; // increment global time on every add/update

    // Check if exists already, update
    struct arc_cache *curr = head;
    while (curr)
    {
        if (strcmp(curr->url, url) == 0)
        {
            free(curr->data);
            curr->data = (char *)malloc(size);
            memcpy(curr->data, data, size);
            curr->size = size;

            curr->freq++;
            curr->time = global_time; // update last accessed time

            int time_diff = global_time - curr->time; // will be zero here
            curr->score = a * curr->freq + b * time_diff;

            pthread_mutex_unlock(&lock);
            return;
        }
        curr = curr->next;
    }

    if (cache_size >= CACHE_LIMIT)
    {
        printf("\n[DEBUG] Cache full. Performing eviction...\n\n");
        delete_lowest_score_node();
        print_cache();
    }

    struct arc_cache *new_node = (struct arc_cache *)malloc(sizeof(struct arc_cache));
    strncpy(new_node->url, url, sizeof(new_node->url) - 1);
    new_node->url[sizeof(new_node->url) - 1] = '\0';
    new_node->data = (char *)malloc(size);
    memcpy(new_node->data, data, size);
    new_node->size = size;
    new_node->time = global_time;
    new_node->freq = 1;
    new_node->score = a * new_node->freq + b * new_node->time; // time_diff is zero at creation
    new_node->next = head;
    head = new_node;
    cache_size++;

    pthread_mutex_unlock(&lock);
}

struct arc_cache *find(char *url)
{
    pthread_mutex_lock(&lock);
    struct arc_cache *temp = head;
    while (temp)
    {
        printf("[DEBUG] Checking cache URL: [%s] vs input URL: [%s]\n", temp->url, url);
        if (strcmp(temp->url, url) == 0)
        {
            temp->freq++;
            int old_time = temp->time;
            global_time++;
            temp->time = global_time;

            int time_diff = global_time - old_time;
            temp->score = a * temp->freq + b * time_diff;

            printf("[DEBUG] Cache hit for URL: %s\n", url);

            pthread_mutex_unlock(&lock);
            return temp;
        }
        temp = temp->next;
    }
    pthread_mutex_unlock(&lock);
    return NULL;
}

void print_cache()
{
    printf("\n-------------------- Cache Contents --------------------\n");
    printf("%-40s %-8s %-14s %-8s %-10s %-6s\n",
           "URL", "Size", "Last Accessed", "Freq", "Score", "Age");
    printf("---------------------------------------------------------\n");

    struct arc_cache *curr = head;
    while (curr)
    {
        int age = global_time - curr->time;
        printf("%-40s %-8d %-14d %-8d %-10.2f %-6d\n",
               curr->url, curr->size, curr->time, curr->freq, curr->score, age);
        curr = curr->next;
    }
    printf("---------------------------------------------------------\n\n");
}

void *handle_request(void *arg)
{
    printf("[DEBUG] Thread ID: %lu\n", (unsigned long)pthread_self());
    sleep(1);
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};
    int received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0)
    {
        close(client_socket);
        return NULL;
    }

    char method[10], url[1000];
    sscanf(buffer, "%s %s", method, url);
    printf("[DEBUG] Handling request: Method = %s | URL = %s\n", method, url);

    char *actual_url = strstr(url, "://") ? strstr(url, "://") + 3 : url;
    char hostname[1000] = {0};
    sscanf(actual_url, "%[^/]", hostname);

    // ✅ First cache check
    struct arc_cache *cached = find(url);
    if (cached)
    {
        printf("[DEBUG] Cache hit for URL: %s\n", url);
        send(client_socket, cached->data, cached->size, 0);
        print_cache();
        close(client_socket);
        return NULL;
    }

    // Fetch from origin server
    struct sockaddr_in server_addr;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("[ERROR] Failed to create socket to web server");
        close(client_socket);
        return NULL;
    }

    struct hostent *host = gethostbyname(hostname);
    if (!host)
    {
        perror("[ERROR] Host resolution failed");
        close(client_socket);
        close(server_socket);
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[ERROR] Connection to web server failed");
        close(client_socket);
        close(server_socket);
        return NULL;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);
    printf("[DEBUG] Sending request to web server...\n");
    send(server_socket, buffer, received, 0);

    char response[BUFFER_SIZE] = {0};
    int total_size = recv(server_socket, response, sizeof(response), 0);
    gettimeofday(&end, NULL);

    if (total_size <= 0)
    {
        printf("[DEBUG] No response received from server.\n");
        close(server_socket);
        close(client_socket);
        return NULL;
    }

    int elapsed_time = (end.tv_sec - start.tv_sec) * 1000 +
                       (end.tv_usec - start.tv_usec) / 1000;

    printf("[DEBUG] Received response from server (%d bytes, %d ms)\n", total_size, elapsed_time);

    // ✅ Re-check cache after fetch
    cached = find(url);
    if (cached)
    {
        printf("[DEBUG] Cache inserted by another thread, using it.\n");
        send(client_socket, cached->data, cached->size, 0);
        print_cache();
        close(server_socket);
        close(client_socket);
        return NULL;
    }

    // ✅ Safe to add to cache
    send(client_socket, response, total_size, 0);
    printf("[DEBUG] Sending response to client\n");

    add_cache_element(response, total_size, url, elapsed_time);
    print_cache();

    close(server_socket);
    close(client_socket);

    return NULL;
}

int main()
{
    int server_fd;
    struct sockaddr_in proxy_addr;

    pthread_mutex_init(&lock, NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("[ERROR] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    proxy_addr.sin_family = AF_INET;
    proxy_addr.sin_addr.s_addr = INADDR_ANY;
    proxy_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&proxy_addr, sizeof(proxy_addr)) < 0)
    {
        perror("[ERROR] Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("[ERROR] Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Proxy server listening on port %d...\n", PORT);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int *client_socket = (int *)malloc(sizeof(int));
        if (!client_socket)
        {
            perror("[ERROR] malloc failed");
            continue;
        }

        *client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (*client_socket < 0)
        {
            perror("[ERROR] Accept failed");
            free(client_socket);
            continue;
        }

        printf("[DEBUG] Accepted new client connection\n");

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_request, client_socket) != 0)
        {
            perror("[ERROR] pthread_create failed");
            close(*client_socket);
            free(client_socket);
            continue;
        }
        printf("[DEBUG] Created thread with ID: %lu\n", (unsigned long)tid);

        pthread_detach(tid);
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
