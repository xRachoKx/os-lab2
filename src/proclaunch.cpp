#include "proclaunch.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <sys/wait.h>
#endif

// Вспомогательная функция для сборки аргументов в Windows
#ifdef _WIN32
std::string prepare_windows_cmd(const std::vector<std::string>& args) {
    std::string cmd = "";
    for (size_t i = 0; i < args.size(); ++i) {
        std::string arg = args[i];
        if (i > 0) cmd += " ";

        // Экранирование по правилам из методички
        if (arg.empty()) {
            cmd += "\"\"";
            continue;
        }

        bool need_quotes = (arg.find(' ') != std::string::npos || arg.find('\t') != std::string::npos);
        if (need_quotes) {
            cmd += "\"";
        }

        for (size_t j = 0; j < arg.length(); ++j) {
            // Считаем обратные слеши перед кавычкой
            if (arg[j] == '"') {
                cmd += "\\\"";
            } else if (arg[j] == '\\' && j + 1 < arg.length() && arg[j+1] == '"') {
                cmd += "\\\\";
            } else {
                cmd += arg[j];
            }
        }

        // Если ставили кавычку в начале и последний символ слеш — удваиваем его перед закрывающей кавычкой
        if (need_quotes) {
            if (!arg.empty() && arg.back() == '\\') {
                cmd += "\\";
            }
            cmd += "\"";
        }
    }
    return cmd;
}
#endif

int pl_start(Process* proc, const std::vector<std::string>& args) {
    if (args.empty()) return -1;

#ifdef _WIN32
    std::string cmd_str = prepare_windows_cmd(args);
    
    // Создаем изменяемый буфер (malloc/vector), так как CreateProcess может менять строку
    std::vector<char> cmd_buf(cmd_str.begin(), cmd_str.end());
    cmd_buf.push_back('\0');

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return -1; // Файл не найден или иная ошибка
    }

    CloseHandle(pi.hThread); // Поток не нужен
    proc->handle = (intptr_t)pi.hProcess;
    proc->pid = (long)pi.dwProcessId;
    return 0;

#else // POSIX (Linux)
    // Подготовка структуры аргументов для execvp
    std::vector<char*> argv;
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0) return -1; // Ошибка fork

    if (pid == 0) {
        // Ребенок
        execvp(argv[0], argv.data());
        _exit(127); // Если execvp не удался
    }

    // Родитель
    proc->pid = pid;
    proc->handle = pid; // Для POSIX хэндлом выступает сам PID
    return 0;
#endif
}

bool pl_poll(Process* proc, int& exit_code) {
#ifdef _WIN32
    DWORD win_code;
    if (GetExitCodeProcess((HANDLE)proc->handle, &win_code)) {
        if (win_code == 259) { // STILL_ACTIVE
            return false; 
        }
        exit_code = (int)win_code;
        CloseHandle((HANDLE)proc->handle);
        return true;
    }
    return false;
#else // POSIX
    int status;
    pid_t res = waitpid(proc->pid, &status, WNOHANG);
    if (res == 0) {
        return false; // Еще работает
    } else if (res > 0) {
        if (WIFEXITED(status)) exit_code = WEXITSTATUS(status);
        else if (WIFSIGNALED(status)) exit_code = 128 + WTERMSIG(status);
        return true;
    }
    return false;
#endif
}

int pl_wait(Process* proc) {
#ifdef _WIN32
    WaitForSingleObject((HANDLE)proc->handle, INFINITE);
    DWORD win_code;
    GetExitCodeProcess((HANDLE)proc->handle, &win_code);
    CloseHandle((HANDLE)proc->handle);
    return (int)win_code;
#else // POSIX
    int status;
    pid_t res;
    do {
        res = waitpid(proc->pid, &status, 0);
    } while (res == -1 && errno == EINTR); // Повторяем при прерывании сигналом

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
#endif
}
