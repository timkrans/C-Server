#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define PUBLIC_DIR "public"

const char *get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";

    if (!strcmp(ext, ".html")) return "text/html";
    if (!strcmp(ext, ".css"))  return "text/css";
    if (!strcmp(ext, ".js"))   return "application/javascript";
    if (!strcmp(ext, ".map"))  return "application/json";
    if (!strcmp(ext, ".json")) return "application/json";
    if (!strcmp(ext, ".png"))  return "image/png";
    if (!strcmp(ext, ".jpg"))  return "image/jpeg";
    if (!strcmp(ext, ".svg"))  return "image/svg+xml";
    if (!strcmp(ext, ".woff")) return "font/woff";
    if (!strcmp(ext, ".woff2"))return "font/woff2";

    return "application/octet-stream";
}

void send_404(int fd) {
    const char *msg =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "404 Not Found";
    send(fd, msg, strlen(msg), 0);
}

void serve_file(int client_fd, const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        send_404(client_fd);
        return;
    }

    struct stat st;
    fstat(fd, &st);

    const char *mime = get_mime_type(path);

    char header[256];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %lld\r\n"
        "Content-Type: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        (long long)st.st_size, mime);

    send(client_fd, header, strlen(header), 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        send(client_fd, buffer, bytes, 0);
    }

    close(fd);
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("Serving static files from ./%s\n", PUBLIC_DIR);
    printf("http://localhost:%d\n", PORT);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) continue;

        char request[BUFFER_SIZE] = {0};
        recv(client_fd, request, sizeof(request) - 1, 0);

        char method[8], path[512];
        sscanf(request, "%7s %511s", method, path);

        if (strcmp(path, "/") == 0)
            strcpy(path, "/index.html");

        if (strstr(path, "..")) {
            send_404(client_fd);
            close(client_fd);
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s%s", PUBLIC_DIR, path);

        serve_file(client_fd, full_path);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
