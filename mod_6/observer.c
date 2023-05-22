// observer.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_BUFFER 256 // размер буфера для сообщений

// функция для визуализации данных
void visualize_data(char *message) {
    // здесь можно использовать различные техники визуализации данных
    // например, графики, таблицы, карты тепла, таймлайны и т.д.
    // для простоты я просто печатаю сообщение
    printf("%s", message);
}

int main(int argc, char *argv[]) {
    int client_fd; // дескриптор сокета клиента
    struct sockaddr_in server_addr; // адрес сервера
    char buffer[MAX_BUFFER]; // буфер для сообщений
    char *server_ip; // IP-адрес сервера
    int server_port; // порт сервера
    int client_id; // идентификатор клиента
    char client_comp[MAX_BUFFER]; // компонент клиента
    int n; // число байтов для чтения/записи

    // проверяем аргументы командной строки
    if (argc != 5) {
        printf("Использование: %s ip-адрес порт идентификатор компонент\n", argv[0]);
        exit(1);
    }
    // получаем IP-адрес, порт, идентификатор и компонент из аргументов
    server_ip = argv[1];
    server_port = atoi(argv[2]);
    client_id = atoi(argv[3]);
    strcpy(client_comp, argv[4]);

    // создаем сокет клиента
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("Ошибка при создании сокета");
        exit(1);
    }

    // заполняем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // подключаемся к серверу
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Ошибка при подключении к серверу");
        exit(1);
    }

    printf("Клиент %d: подключился к серверу %s:%d\n", client_id, server_ip, server_port);

    // отправляем идентификатор и компонент серверу
    sprintf(buffer, "%d %s", client_id, client_comp);
    n = send(client_fd, buffer, strlen(buffer), 0);
    if (n == -1) {
        perror("Ошибка при отправке данных серверу");
        exit(1);
    }

    // читаем приветственное сообщение от сервера
    n = recv(client_fd, buffer, MAX_BUFFER - 1, 0);
    if (n == -1) {
        perror("Ошибка при чтении данных от сервера");
        exit(1);
    }
    buffer[n] = '\0'; // добавляем нуль-терминатор

    printf("%s", buffer); // печатаем приветственное сообщение

    // начинаем цикл обмена данными
    while (1) {
        // читаем компоненты от сервера
        n = recv(client_fd, buffer, MAX_BUFFER - 1, 0);
        if (n == -1) {
            perror("Ошибка при чтении данных от сервера");
            exit(1);
        }
        buffer[n] = '\0'; // добавляем нуль-терминатор

        // визуализируем данные
        visualize_data(buffer);
    }

    // закрываем сокет
    close(client_fd);

    return 0;
}