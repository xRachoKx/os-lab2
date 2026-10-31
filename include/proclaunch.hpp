#ifndef PROCLAUNCH_HPP
#define PROCLAUNCH_HPP

#include <vector>
#include <string>
#include <cstdint>

#ifdef _WIN32
    typedef intptr_t proc_handle_t;
    typedef long proc_pid_t;
#else
    #include <sys/types.h>
    typedef int proc_handle_t; // Для POSIX handle не используется напрямую, храним PID или заглушку
    typedef pid_t proc_pid_t;
#endif

struct Process {
    proc_handle_t handle;
    proc_pid_t pid;
};

// Функция запуска: принимает массив аргументов (argv), где argv[0] — имя программы
int pl_start(Process* proc, const std::vector<std::string>& args);

// Функция неблокирующей проверки (возвращает 0, если работает, или код завершения)
// Если процесс еще активен, возвращает специальное значение (например, 259 для Win) или флаг.
// Для унификации: возвращает true, если процесс завершился, и записывает code.
bool pl_poll(Process* proc, int& exit_code);

// Функция блокирующего ожидания завершения процесса
int pl_wait(Process* proc);

#endif // PROCLAUNCH_HPP
