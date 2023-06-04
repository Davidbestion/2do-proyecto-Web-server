#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>

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
    return path;
}
 //Method that returns all the files and directories from a directory
char *GetTable(char *path)
{
    DIR *dir;
    struct dirent *entry;
    int i = 0;

    dir = opendir(path);
    if (dir == NULL) {
        return NULL;
    }

    char **files = malloc(sizeof(char*) * 1024);
    char **folders = malloc(sizeof(char*) * 1024);
    int num_files = 0;
    int num_folders = 0;
    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_type == 8)//DT_REG = 8: el archivo es regular (no directorio)
        {
            files[num_files] = (char*)entry->d_name;
            num_files++;
        }
        else if(entry->d_type == 4)//DT_DIR = 4: el archivo es un directorio (carpeta)
        {
            folders[num_folders] = (char*)entry->d_name;
            num_folders++;
        }
    }
    closedir(dir);
     
    char *table = malloc(1024);
    strcpy(table, "");

    for (int i = 0; i < num_folders; i++)
    {
        char temp[256];
        sprintf(temp, "<tr><td><a href=\"%s/%s\">%s</a></td>", path, folders[i], folders[i]);
        strcat(table, temp);
    }

    for (int i = 0; i < num_files; i++)
    {
        char temp[256];
        sprintf(temp, "<tr><td><a href=\"%s/%s\">%s</a></td>", path, files[i], files[i]);
        strcat(table, temp);
    }

    free(files);
    free(folders);
    return table;
}

char *BuildResponse(char *directory, char *table)
{
    char *response = malloc(4096);
    sprintf(response, "HTTP/1.1 200 OK\nContent-Type: text/html\n\n<html><head>Directorio actual: %s </head><body><table><tr><th>Name</th><th>Size</th><th>Date</th></tr>%s</table></body></html>", directory, table);
    return response;
}

int main() {

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    char *directory = GetDirectory(NULL);
    char *table = GetTable(directory);
    char *response = BuildResponse(directory, table);

      // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
     // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
     // Bind socket to address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
     // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
     // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
      
      // Send response to client
    while(1)
    {
        write(new_socket, response, strlen(response));

        char buf[30000] = {0};
        read(new_socket, &buf, sizeof(buf));
        char *path = strtok(buf, " ");
        path = strtok(NULL, " ");
        if(opendir(path) == NULL) 
        {
            //Download the file
        }
        else
        {
            free(response);
            response = BuildResponse(path, GetTable(path));
        }
    }
    free(table);
    free(response);
    return 0;
}