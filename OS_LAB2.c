#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>

#define PORT 8080
#define BUFFER_SIZE 1024

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int r) {
    wasSigHup = 1;
}
int main() {
    int server_socket;
    int client_socket = -1;

    //создание сокета
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Couldn't create socket");
        return 1;
    }

    //настройка адреса сервера
    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(PORT);

    //привязка сокета к адресу
    if (bind(server_socket, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) == -1) {
        perror("Socket binding error");
        close(server_socket);
        return 1;
    }
    //прослушивание сокета
    if (listen(server_socket, 5) == -1) {
        perror("Socket listening error");
        close(server_socket);
        return 1;
    }
    printf("Server started on port %d\n", PORT);

    //регистрация обработчика сигналов
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    //блокировка сигналов
    sigset_t blocked_mask, orig_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blocked_mask, NULL);
    sigemptyset(&orig_mask);

    //основной цикл обработки событий
    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(server_socket, &fds);
        int max_fd = server_socket;
        //добавление сокета, если есть подключенный
        if (client_socket != -1) {
            FD_SET(client_socket, &fds);
            if (client_socket > max_fd)
                max_fd = client_socket;
        }
        int res = pselect(max_fd + 1, &fds, NULL, NULL, NULL, &orig_mask);
        if (res == -1) {
            if (errno == EINTR) {
                if (wasSigHup) {
                    printf("The SIGHUP signal has been processed\n");
                    wasSigHup = 0;
                }
                continue;
            }
            perror("pselect");
            break;
        }
        //проверка новых подключений
        if (FD_ISSET(server_socket, &fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_client = accept(server_socket,
                (struct sockaddr*)&client_addr,
                &client_len);
            if (new_client == -1) {
                perror("accept");
                continue;
            }
            //получение информации о клиенте
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            int client_port = ntohs(client_addr.sin_port);

            //проверка клиентского сокета
            if (client_socket == -1) {
                client_socket = new_client;
                printf("New connection accepted from %s:%d (fd: %d)\n",
                    client_ip, client_port, client_socket);
            }
            else {
                printf("Connection from %s:%d closed\n",
                    client_ip, client_port);
                close(new_client);
            }
        }
        //обработка данных от клиента
        if (client_socket != -1 && FD_ISSET(client_socket, &fds)) {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);

            if (bytes_read > 0) {
                printf("Received %zd bytes from client\n", bytes_read);
            }
            else if (bytes_read == 0) {
                //клиент отключился
                printf("Client disconnected\n");
                close(client_socket);
                client_socket = -1;
            }
            else {
                //ошибка при чтении
                perror("Error of reading");
                close(client_socket);
                client_socket = -1;
            }
        }
    }

    //завершение работы
    if (client_socket != -1) {
        close(client_socket);
    }
    close(server_socket);
    return 0;
}
