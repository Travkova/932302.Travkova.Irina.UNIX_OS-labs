#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/types.h>

#define PORT 1255

volatile sig_atomic_t wasSigHup = 0;
void sigHupHandler(int r){
    wasSigHup = 1;
}
int main(){
    int server_fd;
    int client_fd = -1;
    //создание сокета
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Couldn't create socket");
        exit(EXIT_FAILURE);
    }
    //настройка адреса сервера
    struct sockaddr_in srv_addr;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(PORT);
    //привяззка сокета к адресу
    if (bind(server_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        perror("Socket binding error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    //перевод сокета в режим ожидания подключений
    if (listen(server_fd, 5) < 0) {
        perror("Socket listening error");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Server started on port %d\n", PORT);
    //настройка обработчика сигналов
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);
    //блокировка сигналов
    sigset_t blocked_mask, orig_mask;
    sigemptyset(&blocked_mask);
    sigaddset(&blocked_mask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blocked_mask, &orig_mask);
    //основной цикл обработки событий
    fd_set fds;
    int max_fd = server_fd;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(server_fd, &fds);
        max_fd = server_fd;
        //добавление сокета, если есть подключенный
        if (client_fd != -1){
            FD_SET(client_fd, &fds);
            if (client_fd > max_fd)
                max_fd = client_fd;
        }
        int res = pselect(max_fd + 1, &fds, NULL, NULL, NULL, &orig_mask);
        if (res == -1) {
            if (errno == EINTR) {
                printf("Got signal\n");
                if (wasSigHup) {
                    wasSigHup = 0;
                    printf("The SIGHUP signal has been processed\n");
                }
                continue;
            } else{
                break;
            }
        }
        //проверка новых подключений
        if (FD_ISSET(server_fd, &fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (new_client_fd != -1) {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                int client_port = ntohs(client_addr.sin_port);
                if (client_fd == -1) {
                    client_fd = new_client_fd;
                    printf("New connection from %s:%d\n",
                           client_ip, client_port);
                } else {
                    printf("Rejected connection from %s:%d\n",
                           client_ip, client_port);
                    close(new_client_fd);
                }
            }
        }
        //проверка данных от подключенного клиента
        if (client_fd != -1 && FD_ISSET(client_fd, &fds)){
            char data_buffer[1024];
            ssize_t bytes_recieved = read(client_fd, data_buffer, sizeof(data_buffer));
            if (bytes_recieved > 0) {
                printf("Received %zd bytes from client\n", bytes_recieved);
            } else if (bytes_recieved == 0){
                printf("Client disconnected\n");
                close(client_fd);
                client_fd = -1;
            } else{
                perror("Error of reading data");
                close(client_fd);
                client_fd = -1;
            }
        }
    }
    //завершение работы - закрытие сокетов
    close(server_fd);
    return 0;
}
