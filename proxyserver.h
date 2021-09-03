#ifndef PROXYSERVER_H
#define PROXYSERVER_H
#include <queue>

namespace ProxyServer {





    class Server
    {
        std::queue<int> connections;
    public:
        Server() {
            //инициализируем начальные значения
            // передаём управление в loop
        }
        void loop() {
            // ждём соединения
            // обрабатываем сокет кладём в очередь обработки
            // Если в очереди меньше или одного элемента
                // даём сисгнал на пробуждение
        }
        void handler() {
            while(true) {
                // Будим поток
                while(!connections.empty()) {
                    // Запускаем поток на выполнение задачи и передачи на следующий адрессный путь
                    // Удаляем из очереди
                }
                // усыпляем поток
            }
        }
        void handeThread() {
            // получить запрос sql
            // создать сокет для отправки пакета
            // отправить запрос дальше
            // получили ответ, возвращаем получателю
        }
    };
}



#endif // PROXYSERVER_H
