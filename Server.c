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
        result = realloc(result, strlen(result) + template + strlen(path) + (long)(2 * strlen(files[i].name)));
        snprintf(result, strlen(result) + template + strlen(path) + (long)(2 * strlen(files[i].name)), "<tr><td><a href=\"%s/%s\">%s</a></td>", path, files[i].name, files[i].name);    
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
    // strcpy(response, response_template_1);
    // strcat(response, directory);
    // strcat(response, response_template_2);
    // strcat(response, table);
    // strcat(response, response_template_3);
    snprintf(response, response_len, "%s%s%s%s%s", response_template_1, directory, response_template_2, table, response_template_3);
    return response;

}

int main() 
{
    char* dir = "/media/david/24ACA5E9ACA5B628";
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    char *directory = GetDirectory(dir);
    char *table = GetFiles(directory);//"<tr><td><a href=\"/Desktop/directorio1\">directorio1</a></td><td>0</td><td>2017-01-20 10:46:34</td></tr><tr><td><a href=\"/Desktop/directorio2\">directorio2</a></td><td>0</td><td>2017-02-28 12:34:56</td></tr><tr><td><a href=\"/Desktop/file3\">file3</a></td><td>3.8k</td><td>2017-03-30 09:12:11</td></tr><tr><td><a href=\"/Desktop/file4\">file4</a></td><td>4.7G</td><td>2017-12-31 11:59:59</td></tr>";
    char *response = BuildResponse(directory, table);
    //free(table);

      // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        exit(EXIT_FAILURE);
    }
     // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) 
    {
        exit(EXIT_FAILURE);
    }
     // Bind socket to address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) 
    {
        exit(EXIT_FAILURE);
    }
     // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) 
    {
        exit(EXIT_FAILURE);
    }
    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) 
    {
        exit(EXIT_FAILURE);
    }

    write(new_socket, response, strlen(response));
    
    char buf[30000] = {0};
    while(read(new_socket, &buf, sizeof(buf))>0)
    {
        // Send response to client
        

        char *path = strtok(buf, " ");
        path = strtok(NULL, " ");

        // if (path && path[0] == '/') 
        // {
        //     memmove(path, path + 1, strlen(path));
        //     //path = path // Remove the leading '/'
        // }
        
        if(opendir(path) == NULL) 
        {
            //Download the file
        }
        else
        {
            directory = GetDirectory(path);
            table = GetFiles(directory);

            free(response);
            response = BuildResponse(directory, table);

            // ssize_t bytes_sent = 
            // write(new_socket, response, strlen(response));
            // if (bytes_sent <= 0)
            // {
            //     break;
            // }
            
        }
        //close(new_socket);
    }
    //free(table);
    free(response);
    return 0;
}