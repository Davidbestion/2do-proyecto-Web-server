#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define PORT 9000

 //Method that returns the new directory.
char *GetDirectory(char *path)
{
    if(path == NULL)
    {
        char *home_dir = getenv("HOME");
        if (home_dir == NULL)
        {
            exit(EXIT_FAILURE);
        }
        else
        {
            return home_dir;
        }
    }

    char* linux_path = malloc(strlen(path) + 1);
    char* p = linux_path;
    while (*path) {
        if (*path == '%') {
            char code[3] = { path[1], path[2], '\0' };
            *p++ = (char) strtol(code, NULL, 16);
            path += 3;
        } else {
            *p++ = *path++;
        }
    }
    *p = '\0';
    return linux_path;

    // int i, j;
    // char* true_path = malloc(strlen(path) + 1);
    // for (i = 0, j = 0; path[i] != '\0'; i++, j++) {
    //     if (path[i] == '%' && path[i+1] == '2' && path[i+2] == '0') {
    //         // space character found
    //         true_path[j] = ' ';
    //         i += 2;
    //     } else if (path[i] == '/') {
    //         // slash character found
    //         true_path[j] = '/';
    //     } else {
    //         // regular character found
    //         true_path[j] = path[i];
    //     }
    // }
    // true_path[j] = '\0';
    // return true_path;
    // return path;
}

//Method that returns all the files and directories from a directory
typedef struct File 
{
    char *name;
    int is_dir;
} File;

char *GetTable(File *files, int length, char *path)
{
    char *result = malloc(1);
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
        // result = realloc(result, strlen(result) + template + strlen(path) + (long)(2 * strlen(files[i].name)));
        // snprintf(result, strlen(result) + template + strlen(path) + (long)(2 * strlen(files[i].name)), "<tr><td><a href=\"%s/%s\">%s</a></td>", path, files[i].name, files[i].name);    
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
    if (response == NULL) {
        return NULL;
    }
    strcpy(response, response_template_1);
    strcat(response, directory);
    strcat(response, response_template_2);
    strcat(response, table);
    strcat(response, response_template_3);
    //snprintf(response, response_len, "%s%s%s%s%s", response_template_1, directory, response_template_2, table, response_template_3);
    return response;

}

void CreatePage(char* path, int socket)
{
    char *directory = GetDirectory(path);
    printf("Salio fel GetDirectory.\n");
    char *table = GetFiles(directory);//"<tr><td><a href=\"/Desktop/directorio1\">directorio1</a></td><td>0</td><td>2017-01-20 10:46:34</td></tr><tr><td><a href=\"/Desktop/directorio2\">directorio2</a></td><td>0</td><td>2017-02-28 12:34:56</td></tr><tr><td><a href=\"/Desktop/file3\">file3</a></td><td>3.8k</td><td>2017-03-30 09:12:11</td></tr><tr><td><a href=\"/Desktop/file4\">file4</a></td><td>4.7G</td><td>2017-12-31 11:59:59</td></tr>";
    printf("Salio del GetTable.\n");
    char *response = BuildResponse(directory, table);
    printf("Creo la response.\n");

    write(socket, response, strlen(response));
    printf("Escribio.\n");
}

// void LoadPage(int socket, int first_time)
// {
    
// }
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

    int sfd = get_server_fd(server_ip, server_port);
    if(sfd < 0)
    {
        printf("Error: no se pudo crear el server.\n");
        exit(EXIT_FAILURE);
    }

    int first_time = 1;
    while(1)
    {
        int cfd = accept(sfd, NULL, NULL);
        if(cfd < 0)
        {
            printf(" accept() error: no se puede conectar con cliente.\n");
            exit(EXIT_FAILURE);
        }

        printf("Cliente: conectado\n");

        char buf[1024] = {0};
        if(read(cfd, &buf, sizeof(buf)))
        {
            printf("Entro al while.\n");
            printf("%s\n", buf);
            
            char* path;
            if(first_time > 0)
            {
                printf("Entro al if.\n");
                first_time = 0;
                path = dir;
            }
            else
            {
                printf("Entro al else.\n");
                //buf[bytes_read] = '\0';
                path = strtok(buf, " ");
                printf("%s",path);
                path = strtok(NULL, " ");
            }
            printf("%s\n", path);

            CreatePage(path, cfd);
            printf("Salio del CreatePage.\n");
            close(cfd);
            printf("Cerro el socket.\n \n");
        }
    }

    
    // do
    // {
    //     printf("Entro al do-while.\n");
    //     char* path = NULL;
    //     if(first_time != 1)
    //     {
    //         first_time = 0;
    //         path = strtok(buf, " ");
    //         path = strtok(NULL, " ");
    //     }

    //     LoadPage(path, new_socket);

    // } while (read(new_socket, &buf, sizeof(buf)));
    
    
    // char buf;
    // while(read(new_socket, &buf, sizeof(buf))>0)
    // {
    //     if(first_time)
    //     {
    //         first_time = 0;
    //         continue;
    //     }
    //     // Send response to client
    //     char *path = strtok(buf, " ");
    //     *path = strtok(NULL, " ");

        // if(method == "GET")
        // {
        //     //response = "";
        //     //write(new_socket, response, strlen(response));
        // }
        

        
        // if(opendir(path) == NULL) 
        // {
        //     //Download the file
            
        // }
        // else
        // {
        //     free(response);

        //     directory = GetDirectory(path);
        //     table = GetFiles(directory);

        //     response = BuildResponse(directory, table);

        //     //ssize_t bytes_sent = 
        //     write(new_socket, response, strlen(response));
        //     first_time = 1;
        //     // if (bytes_sent <= 0)
        //     // {
        //     //     break;
        //     // }
            
        // }
        // //free(buf);
        // //close(new_socket);
    //}
    //free(table);
    //free(response);
    //close(new_socket);
    printf("Conexion cortada");
    return 0;
}