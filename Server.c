#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 9000

 //Method that converts the path from HTTP format to Linux format.
char *GetDirectory(char *path)
{
    char* normal_path = malloc(strlen(path) + 1);
    char* p = normal_path;
    while (*path) 
    {
        if (*path == '%') 
        {
            char code[3] = { path[1], path[2], '\0' };
            *p++ = (char) strtol(code, NULL, 16);
            path += 3;
        } 
        else 
        {
            *p++ = *path++;
        }
    }
    *p = '\0';

    return normal_path;
}

//Checks if the inner_path is part of the path.
int contains(char* path, char* inner_path) 
{
    int len1 = strlen(path);
    int len2 = strlen(inner_path);
    
    if (len1 < len2) {
        return 0;
    }
    
    if (strncmp(path, inner_path, len2) == 0) {
        return 1;
    }
    
    return contains(path + 1, inner_path);
}

//Method that returns all the files and directories from a directory
typedef struct File 
{
    char *name;
    int is_dir;
} File;

char *GetTable(File *files, int length, char *path)
{
    size_t total_size = 1;
    char *result = malloc(total_size);
    result[0] = '\0';
    size_t template = (long)(strlen("<tr><td><a href=\"%s/%s\">%s</a></td>") + 1);
    for (int i = 0; i < length; i++)
    {
        char row[template + strlen(path) + 2 * strlen(files[i].name)]; // create a separate buffer for the HTML table row
        snprintf(row, sizeof(row), "<tr><td><a href=\"%s/%s\">%s</a></td>", path, files[i].name, files[i].name); // format the HTML table row
        if (result == NULL) {
            result = strdup(row); // allocate memory for the first row
        } else {
            result = realloc(result, strlen(result) + strlen(row) + 1); // increase the size of the result buffer
            strcat(result, row); // concatenate the row to the result string
        }
    }

    return result;
}

char* GetFiles(char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL) 
    {
        return NULL;
    }

    File* files = NULL;
    struct dirent *entry;
    int count = 0;
    int num_files = 0;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        num_files++;

        File* file = malloc(sizeof(File));
        file->name = strdup(entry->d_name);

        if (entry->d_type == 4) //DT_DIR = 4
        {
            file->is_dir = 1;
        } 
        else 
        {
            file->is_dir = 0;
        }

        files = realloc(files, (count + 1) * sizeof(File));
        files[count] = *file;
        (count)++;
    }

    closedir(dir);
    char *table = GetTable(files, num_files, path);
    free(files);
    return table;
}

char *BuildResponse(char *directory, char *table)
{
    char* response_template_1 = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><head>Directorio actual: ";
    char* response_template_2 = " </head><body><table><tr><th>Name</th><th>Size</th><th>Date</th></tr>";
    char* response_template_3 = "</table></body></html>";
    
    //template_1 + directory + template_2 + table + template_3.
    size_t response_len = strlen(response_template_1) + strlen(directory) + strlen(response_template_2) + strlen(table) + strlen(response_template_3) + 1;
    char* response = malloc(response_len);
    if (response == NULL) 
    {
        return NULL;
    }
    strcpy(response, response_template_1);
    strcat(response, directory);
    strcat(response, response_template_2);
    strcat(response, table);
    strcat(response, response_template_3);
    
    return response;

}

void CreatePage(char* path, int socket)
{
    char *table = GetFiles(path);
    char *response = BuildResponse(path, table);
    printf("Creo la response.\n");

    write(socket, response, strlen(response));
    printf("Escribio.\n");
}
struct sockaddr_in build_server_addr(char* server_ip, int server_port)
{
    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;            // Familia de direcciones
    server.sin_port = htons(server_port);   // Puerto del servidor
    inet_aton(server_ip, &server.sin_addr); // Dirección IP del servidor
    return server;
}

int get_server_fd(char* server_ip, int server_port)
{
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd < 0) 
    {
        return -1;
    }
    
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in server = build_server_addr(server_ip, server_port);

    if(bind(sfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        return -1;
    }

    if(listen(sfd, 1) < 0)
    {
        return -1;
    }
    return sfd;

}
struct download_args {
    char* path;
    int socket;
};

void DownloadFile(char* path, int socket)
{
    char* error_message = "Error getting file.";
    char *http_response_template = "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Disposition: attachment; filename=%s\n\n";
     // Open file for reading
    FILE *file = fopen(path, "rb");
    if (!file) {
        printf("%s\n", error_message);
        exit(EXIT_FAILURE);
    }
     // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
     // Allocate buffer for file contents
    char *file_contents = malloc(file_size);
    if (!file_contents) {
        printf("%s\n", error_message);
        fclose(file);
        exit(EXIT_FAILURE);
    }

     // Read file contents into buffer
    size_t bytes_read = fread(file_contents, 1, file_size, file);
    if (bytes_read != file_size) 
    {
        printf("%s\n", error_message);
        free(file_contents);
        fclose(file);
        exit(EXIT_FAILURE);
    }

     // Determine MIME type based on file extension
    char *file_extension = strrchr(path, '.');
    char *mime_type;
    if (file_extension && strcmp(file_extension, ".pdf") == 0) {
        mime_type = "application/pdf";
    } else if (file_extension && strcmp(file_extension, ".jpg") == 0) {
        mime_type = "image/jpeg";
    } else if (file_extension && strcmp(file_extension, ".png") == 0) {
        mime_type = "image/png";
    } else {
        mime_type = "application/octet-stream";
    }

    //Build http response.
    char headers[1000];
    sprintf(headers, http_response_template, mime_type, path);
    sprintf(headers + strlen(headers), "Content-Length: %ld\n\n", file_size);
    write(socket, headers, strlen(headers));

    //Send file contents to client
    write(socket, file_contents, file_size);
    fclose(file);
}

void *DownloadThread(void *arg)
{
    struct download_args *args = (struct download_args *)arg;

    DownloadFile(args->path, args->socket);
    pthread_exit(NULL); 
}

int ClientDisconnected(int client) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client, &read_fds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    int activity = select(client + 1, &read_fds, NULL, NULL, &timeout);
    if (activity == -1) 
    {
        printf("Error checking if client is disconnected.");
        exit(EXIT_FAILURE);
    } 
    else if (activity == 0) 
    {
        // Timeout occurred, client is still connected
        return 0;
    } 
    else 
    {
        // Check if client socket is readable
        if (FD_ISSET(client, &read_fds)) {
            char buffer[1];
            int num_bytes = recv(client, buffer, sizeof(buffer), MSG_PEEK);
            if (num_bytes == 0) 
            {
                // Client has disconnected
                return 1;
            } else if (num_bytes == -1) 
            {
                printf("Error checking if client is disconnected.");
                exit(EXIT_FAILURE);
            }
        }
    }
     
    // Client is still connected
    return 0;
}

void ExecuteProgram(int cfd, int sfd, char* dir)
{
    char* path;

    char buf[1024] = {0};
    int bytes_read = read(cfd, &buf, sizeof(buf));
    if(bytes_read > 0)
    {
        printf("%s\n", buf);

        path = strtok(buf, " ");
        path = strtok(NULL, " ");

        path = GetDirectory(path);

        if(path == NULL)
        {
            printf("Error: invalid path.\n");
            close(cfd);
            exit(EXIT_FAILURE);
        }
        if(contains(path, dir) == 0)
        {
            path = dir;
        }

        printf("Updated path: %s\n", path);

        if(opendir(path) == NULL)
        {
            struct download_args args;
            args.path = path;
            args.socket = cfd;
            pthread_t tid;
                    

            int result = pthread_create(&tid, NULL, DownloadThread, (void*)&args);
            if(result != 0)
            {
                printf("Error: not created thread.");
                close(cfd);
            }
            pthread_join(tid, NULL);

        }
        else
        {
            CreatePage(path, cfd);
        }
                
    }
    else
    {
        printf("Read error: could not read request.");
    }
    close(cfd);
        

    
}

int main(int argn, char*argv[]) 
{
    if(argn < 3)
    {
        printf("Usage: %s <server_port> <directory_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    int server_port = atoi(argv[1]);
    char* dir = argv[2];
    char* server_ip = "127.0.0.1";

    printf("Server starting in port: %s.\n", argv[1]);
    printf("Direction of the server: %s\n", dir);
    printf("IP address: %s\n", server_ip);

    int sfd = get_server_fd(server_ip, server_port);
    if(sfd < 0)
    {
        printf("Error: no se pudo crear el server.\n");
        exit(EXIT_FAILURE);
    }

    printf("Server ready and waiting for connections.\n");

    int first_time = 1;
    while(1)
    {
        int cfd = accept(sfd, NULL, NULL);
        if(cfd < 0)
        {
            printf("accept() error: no se puede conectar con cliente.\n");
            exit(EXIT_FAILURE);
        }

        int pid = fork();
        
        char num[20];
        if(pid != 0)
        {
            sprintf(num,"%d", pid);
            printf("%s\n", num);
            ExecuteProgram(cfd, sfd, dir);
            exit(0);
            return 0;
        }
        else
        {
            sprintf(num, "%d", pid);
            printf("%s\n", num);
            close(cfd);
        }
    }
    printf("Conexion cortada");
    return 0;
}