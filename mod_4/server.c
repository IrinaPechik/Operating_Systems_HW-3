// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <stdbool.h>

#define MAX_CLIENTS 3  // Макс. количество курильщиков
#define MAX_BUFFER 256

// Компоненты для сигареты.
enum component
{
    TOBACCO = 1,
    PAPER = 2,
    MATCH = 3
};

// Структура курильщика
struct client
{
    int id;              // id (1, 2 или 3)
    int sock;            // client socket descriptor
    enum component comp; // компонент курильщика (tobacco, paper or match)
};

// Генерация двух компонентов сигареты
void generate_components(enum component *c1, enum component *c2)
{
    int r = rand() % 3 + 1;
    switch (r)
    {
    case 1:
        *c1 = PAPER;
        *c2 = MATCH;
        break;
    case 2:
        *c1 = TOBACCO;
        *c2 = MATCH;
        break;
    case 3:
        *c1 = TOBACCO;
        *c2 = PAPER;
        break;
    }
}

// Переводим компоненту сигареты в строку.
const char *get_component_name(enum component c)
{
    switch (c)
    {
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

// Функция для проверки наличия третьего компонента сигареты у курильщика.
int has_third_component(struct client *cl, enum component c1, enum component c2)
{
    return (cl->comp != c1 && cl->comp != c2);
}

// Функция для отправки сообщения сокету клиента.
void send_message(int sock, char *msg)
{
    if (send(sock, msg, strlen(msg), 0) < 0)
    {
        perror("send error");
        exit(1);
    }
}

int main(int argc, char *argv[])
{

    // Проверка аргументов.
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(1);
    }

    // Инициализируем сид генерации.
    srand(time(NULL));

    // Создаём сокет сервераю
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    int new_socket;
    // Порт.
    int port = atoi(argv[1]);       // get port number from command line argument
    struct sockaddr_in server_addr; // server address structure
    int opt = 1;
    pthread_t thread_id;

    // Настраиваем сокет.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Очищаем.

    server_addr.sin_family = AF_INET;         // IPv4 protocol
    server_addr.sin_addr.s_addr = INADDR_ANY; // any local IP address
    server_addr.sin_port = htons(port);       // specified port number

    // Привязываем сокет к адресу и порту.
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind error");
        exit(1);
    }

    // Слушаем входящие соединения.
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("listen error");
        exit(1);
    }

    printf("Сервер: запущен на порту %d\n", port);

    // Принимаем соединения от клиентов и храним их информацию в массиве.
    struct client clients[MAX_CLIENTS]; // множество клиентов.
    int num_clients = 0;                // количество подключённых клиентов.

    while (num_clients < MAX_CLIENTS)
    {
        // Принимаем новое подключение от клиента.
        struct sockaddr_in client_addr;             // Структура адреса клиента.
        socklen_t client_len = sizeof(client_addr); // Размер структуры адреса клиента

        int client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
        {
            perror("accept error");
            exit(1);
        }

        printf("Сервер: принял соединение от %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Получаем идентификатор клиента и компонент из клиентского сокета.
        char buffer[MAX_BUFFER];       // буфер сообщений
        memset(buffer, 0, MAX_BUFFER); // очищаем буфер

        if (recv(client_sock, buffer, MAX_BUFFER - 1, 0) < 0)
        {
            perror("recv error");
            exit(1);
        }

        printf("Сервер: получил сообщение от клиента: %s\n", buffer);

        int id;              // id клиента (1, 2 or 3)
        enum component comp; // component клиента (tobacco, paper or match)

        if (sscanf(buffer, "%d %u", &id, &comp) != 2 || id < 1 || id > MAX_CLIENTS || comp < TOBACCO || comp > MATCH)
        {
            fprintf(stderr, "Сервер: неверный формат сообщения от клиента\n");
            close(client_sock); // закрыть соединение с клиентом
            continue;           
        }

        printf("Сервер: получил идентификатор и компонент от клиента: %d %s\n", id, get_component_name(comp));

        clients[id - 1].id = id;
        clients[id - 1].sock = client_sock;
        clients[id - 1].comp = comp;

        num_clients++; //увеличиваем количество подключённых клиентов.

        printf("Сервер: подключил клиента %d с компонентом %s\n", id, get_component_name(comp));
    }

    printf("Сервер: подключил всех клиентов\n");
    while (1)
    {

        // Генерируем два компонента сиграеты.
        enum component c1, c2;         // два компонента сиграеты.
        generate_components(&c1, &c2);

        printf("Сервер: сгенерировал два компонента: %s и %s\n", get_component_name(c1), get_component_name(c2));

        char buffer[MAX_BUFFER];    
        memset(buffer, 0, MAX_BUFFER);    
        sprintf(buffer, "%d %d", c1, c2); 

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            send_message(clients[i].sock, buffer); 
        }

        printf("Сервер: отправил два компонента всем клиентам\n");

        // Дождидаемся сообщения от одного из клиентов, у которого есть третий компонент
        int smoker_found = 0; // флажок, указывающий, обнаружен ли курильщик
        int smoker_id;        // id курильщика, который может сделать сигарету

        while (!smoker_found)
        {

            for (int i = 0; i < MAX_CLIENTS; i++)
            {

                memset(buffer, 0, MAX_BUFFER);

                if (recv(clients[i].sock, buffer, MAX_BUFFER - 1, 0) < 0)
                {
                    perror("recv error");
                    exit(1);
                }

                printf("Сервер: получил сообщение от клиента %d: %s\n", clients[i].id, buffer);

                if (strcmp(buffer, "курю") == 0)
                {                              
                    smoker_found = 1;          
                    smoker_id = clients[i].id; 
                    break;                     
                }
            }
            if (smoker_found)
            {
                break; 
            }
        }

        printf("Сервер: нашел курильщика с третьим компонентом: %d\n", smoker_id);

        // Ждём несколько секунд для имитации время курения
        sleep(3);

        printf("Сервер: начал новый раунд\n");
        smoker_found = 0;
    }

    // закрываем все клиентские сокеты и серверный сокет.
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        close(clients[i].sock);
    }
    close(server_fd);

    return 0;
}