#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER 256 

enum component {
    TOBACCO = 1,
    PAPER = 2,
    MATCH = 3
};

const char* get_component_name(enum component c) {
    switch (c) {
        case TOBACCO:
            return "табак";
        case PAPER:
            return "бумага";
        case MATCH:
            return "спички";
        default:
            return "неизвестный компонент";
    }
}

void send_message(int sock, char *msg) {
    if (send(sock, msg, strlen(msg), 0) < 0) {
        perror("Ошибка отправки");
        exit(1);
    }
}

int main(int argc, char *argv[]) {

    if (argc != 5) {
        fprintf(stderr, "Использование: %s server_ip server_port client_id client_component\n", argv[0]);
        exit(1);
    }

    // Получение IP-адреса сервера и номера порта из аргументов командной строки
    char *server_ip = argv[1]; // IP-адрес сервера
    int server_port = atoi(argv[2]); // номер порта сервера

    // Получение идентификатора клиента и компонента из аргументов командной строки
    int id = atoi(argv[3]); // идентификатор клиента (1, 2 или 3)
    enum component comp = atoi(argv[4]); // компонент клиента (табак, бумага или спички)

    if (id < 1 || id > 3 || comp < TOBACCO || comp > MATCH) {
        fprintf(stderr, "Недопустимый идентификатор клиента или компонент\n");
        exit(1);
    }

    printf("Клиент %d: имею компонент %s\n", id, get_component_name(comp));

    // Создание клиентского сокета
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Ошибка создания сокета");
        exit(1);
    }

    // Подключение к серверному сокету
    struct sockaddr_in server_addr; // структура адреса сервера
    memset(&server_addr, 0, sizeof(server_addr)); // очистка структуры
    server_addr.sin_family = AF_INET; // использование протокола IPv4
    server_addr.sin_addr.s_addr = inet_addr(server_ip); // использование указанного IP-адреса
    server_addr.sin_port = htons(server_port); // использование указанного номера порта

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Ошибка подключения");
        exit(1);
    }

    printf("Клиент %d: подключился к серверу %s:%d\n", id, server_ip, server_port);

    // Отправка идентификатора клиента и компонента на серверный сокет
    char buffer[MAX_BUFFER]; // буфер для сообщений
    memset(buffer, 0, MAX_BUFFER); // очистка буфера
    sprintf(buffer, "%d %d", id, comp); // запись идентификатора и компонента в буфер

    send_message(client_sock, buffer); // отправка буфера на серверный сокет

    printf("Клиент %d: отправил идентификатор и компонент серверу\n", id);

    // Запуск основного цикла клиента
    while (1) {

        // Получение двух компонентов с серверного сокета
        memset(buffer, 0, MAX_BUFFER); // очистка буфера

        if (recv(client_sock, buffer, MAX_BUFFER - 1, 0) < 0) {
            perror("Ошибка приема");
            exit(1);
        }

        printf("Клиент %d: получил сообщение от сервера: %s\n", id, buffer);

        enum component c1, c2; // два компонента

        if (sscanf(buffer, "%d %d", &c1, &c2) != 2 || c1 < TOBACCO || c1 > MATCH || c2 < TOBACCO || c2 > MATCH) {
            fprintf(stderr, "Клиент %d: неверный формат сообщения от сервера\n", id);
            continue; // пропуск этой итерации цикла и ожидание другого сообщения
        }

        printf("Клиент %d: получил два компонента от сервера: %s и %s\n", id, get_component_name(c1), get_component_name(c2));

        // Проверка, есть ли у клиента третий компонент, необходимый для сигареты
        if (comp != c1 && comp != c2) {

            printf("Клиент %d: имею третий компонент: %s\n", id, get_component_name(comp));

            send_message(client_sock, "курю"); // отправка "курю" на серверный сокет

            printf("Клиент %d: отправил сообщение серверу: курю\n", id);

            printf("Клиент %d: забираю компоненты со стола и курю сигарету\n", id);

            // ожидание нескольких секунд для имитации времени курения
            sleep(3);

            printf("Клиент %d: закончил курить сигарету\n", id);

        } else {

            printf("Клиент %d: не имею третьего компонента\n", id);

        }
    }

    // Закрытие клиентского сокета
    close(client_sock);

    return 0;
}
