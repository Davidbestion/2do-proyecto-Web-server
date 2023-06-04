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
int *GetDirectory(char *path, char *folder_name)
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
    if(folder_name == NULL)
    {
        return NULL;
    }
    char *path = ("%s%s", path, folder_name);
    return path;
}

//Method that returns all the files and directories from a directory
char **GetFilesAndDirs(char *path)
{
    DIR *dir;
    struct dirent *entry;
    char **files = NULL;
    int i = 0;

    dir = opendir(path);
    if (dir == NULL) {
        return NULL;
    }

    char **result;
    char *files;
    char *folders;
    int num_files = 0;
    int num_folders = 0;
    while((entry = readdir(dir)) != NULL)
    {
        if(opendir(("path/%s",entry->d_name)) == NULL)
        {
            files[num_files] = entry->d_name;
            num_files++;
        }
        else
        {
            folders[num_folders] = entry->d_name;
            num_folders++;
        }
    }
    char str;//ME QUEDE AQUI.
    result[0] = ("%s",sprintf(str, "%d", num_files + num_folders));
    result[1] = folders;
    result[2] = files;

    return result;
}
char* GetTable(char **all_files)
{
    char* result;
    for (int i = 0; i < 2; i++)//all_files allways has only 2 elements.
    {
        for (int j = 0; j < sizeof(all_files[i]); j++)
        {
            /* code */
        }
        
    }
    
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    char *directory = GetDirectory(NULL, NULL);
    char **table = GetTable(directory);
    char *response = "HTTP/1.1 200 OK\nContent-TYpe: text/html\n\n<html><head>Directorio actual: %s </head><body><table><tr><th>Name</th><th>Size</th><th>Date</th></tr>%s</table></body></html>", directory, table;
    
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
    write(new_socket, response, strlen(response));

    char buf;
    read(new_socket, &buf, sizeof(buf))
    
    if(strstr(buf,"GET ", ))
    
    return 0;
}