// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>

#define MAX_CLIENTS 10 // максимальное число клиентов
#define MAX_BUFFER 256 // размер буфера для сообщений
#define MAX_FD 1024 // максимальный дескриптор сокета

// функция для генерации двух случайных компонентов из трех возможных
void generate_components(char *components) {
    int r = rand() % 3; // случайное число от 0 до 2
    switch (r) {
        case 0:
            strcpy(components, "табак и бумага");
            break;
        case 1:
            strcpy(components, "табак и спички");
            break;
        case 2:
            strcpy(components, "бумага и спички");
            break;
    }
}

int main(int argc, char *argv[]) {
    int server_fd; // дескриптор сокета сервера
    int client_fd[MAX_CLIENTS]; // дескрипторы сокетов клиентов
    int client_id[MAX_CLIENTS]; // идентификаторы клиентов
    char client_comp[MAX_CLIENTS][MAX_BUFFER]; // компоненты клиентов
    struct sockaddr_in server_addr; // адрес сервера
    struct sockaddr_in client_addr; // адрес клиента
    socklen_t client_len; // размер адреса клиента
    char buffer[MAX_BUFFER]; // буфер для сообщений
    int port; // порт для прослушивания
    int i; // счетчик для циклов
    int n; // число байтов для чтения/записи
    int smoker; // флаг для определения курильщика
    char components[MAX_BUFFER]; // компоненты на столе
    fd_set readfds; // множество дескрипторов для чтения
    int maxfd; // максимальный дескриптор в множестве

    // проверяем аргументы командной строки
    if (argc != 2) {
        printf("Использование: %s порт\n", argv[0]);
        exit(1);
    }

    // получаем порт из аргумента
    port = atoi(argv[1]);

    // создаем сокет сервера
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Ошибка при создании сокета");
        exit(1);
    }
    // заполняем адрес сервера
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(port);
server_addr.sin_addr.s_addr = INADDR_ANY;

// связываем сокет сервера с адресом
if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    perror("Ошибка при связывании сокета");
    exit(1);
}

// прослушиваем входящие соединения
if (listen(server_fd, MAX_CLIENTS) == -1) {
    perror("Ошибка при прослушивании сокета");
    exit(1);
}

printf("Сервер: запущен на порту %d\n", port);

// инициализируем генератор случайных чисел
srand(time(NULL));

// обнуляем дескрипторы клиентов
for (i = 0; i < MAX_CLIENTS; i++) {
    client_fd[i] = -1;
}

// начинаем цикл обмена данными
while (1) {
    // очищаем множество дескрипторов для чтения
    FD_ZERO(&readfds);

    // добавляем сокет сервера в множество
    FD_SET(server_fd, &readfds);
    maxfd = server_fd;

    // добавляем сокеты клиентов в множество
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (client_fd[i] > 0) {
            FD_SET(client_fd[i], &readfds);
        }
        if (client_fd[i] > maxfd) {
            maxfd = client_fd[i];
        }
    }

    // ждем активности на одном из сокетов
    if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
        perror("Ошибка при вызове select");
        exit(1);
    }

    // проверяем, есть ли новое соединение на сокете сервера
    if (FD_ISSET(server_fd, &readfds)) { // принимаем новое соединение от клиента 
    client_len = sizeof(client_addr); 
    int new_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len); 
    if (new_fd == -1) {
         perror("Ошибка при принятии соединения");
          exit(1);
    }
            printf("Сервер: принял соединение от %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // читаем идентификатор и компонент от клиента
        n = recv(new_fd, buffer, MAX_BUFFER - 1, 0);
        if (n == -1) {
            perror("Ошибка при чтении данных от клиента");
            exit(1);
        }
        buffer[n] = '\0'; // добавляем нуль-терминатор

        // разбиваем строку на две части по пробелу
        char *token = strtok(buffer, " ");
        int id = atoi(token); // первая часть - идентификатор
        token = strtok(NULL, " ");
        char *comp = token; // вторая часть - компонент

        // ищем свободное место в массиве клиентов
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fd[i] == -1) {
                client_fd[i] = new_fd; // сохраняем дескриптор
                client_id[i] = id; // сохраняем идентификатор
                strcpy(client_comp[i], comp); // сохраняем компонент
                break;
            }
        }

        // проверяем, не переполнен ли массив
        if (i == MAX_CLIENTS) {
            printf("Сервер: достигнут лимит клиентов\n");
            close(new_fd); // закрываем новое соединение
        } else {
            // отправляем приветственное сообщение клиенту
            sprintf(buffer, "Сервер: приветствую клиента %d с компонентом %s\n", id, comp);
            n = send(new_fd, buffer, strlen(buffer), 0);
            if (n == -1) {
                perror("Ошибка при отправке данных клиенту");
                exit(1);
            }
        }
    }

    // проверяем, есть ли сообщения от клиентов
    for (i = 0; i < MAX_CLIENTS; i++) { 
        if (client_fd[i] > 0 && FD_ISSET(client_fd[i], &readfds)) {
             // читаем сообщение от клиента
              n = recv(client_fd[i], buffer, MAX_BUFFER - 1, 0);
               if (n == -1) { 
                perror("Ошибка при чтении данных от клиента"); exit(1);
            }
            buffer[n] = '\0'; // добавляем нуль-терминатор
                        // проверяем, является ли клиент курильщиком
            if (strcmp(buffer, "Клиент: курю сигарету\n") == 0) {
                smoker = 1; // устанавливаем флаг курильщика
                printf("Сервер: получил от клиента %d сообщение о том, что он курит сигарету\n", client_id[i]);
                break; // прерываем цикл
            }
        }
    }

    // если не нашли курильщика, завершаем программу
    if (smoker == 0) {
        printf("Сервер: не нашел курильщика, выхожу из цикла\n");
        break;
    }

    // генерируем два случайных компонента
    generate_components(components);

    printf("Сервер: положил на стол %s\n", components);

    // отправляем компоненты всем клиентам
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (client_fd[i] > 0) {
            sprintf(buffer, "Сервер: на столе %s\n", components);
            n = send(client_fd[i], buffer, strlen(buffer), 0);
            if (n == -1) {
                perror("Ошибка при отправке данных клиенту");
                exit(1);
            }
        }
    }

    // обнуляем флаг курильщика
    smoker = 0;
}

// закрываем сокеты
for (i = 0; i < MAX_CLIENTS; i++) {
    if (client_fd[i] > 0) {
        close(client_fd[i]);
    }
}
close(server_fd);

return 0;
}