#include "proclaunch.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
    #define sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define sleep_ms(ms) usleep((ms) * 1000)
#endif

int main(int argc, char* argv[]) {
    // Режим "Ребёнка" для тестов
    if (argc >= 4 && strcmp(argv[1], "--child") == 0) {
        int sleep_time = std::atoi(argv[3]);
        int exit_code = std::atoi(argv[2]);

        if (sleep_time > 0) {
            sleep_ms(sleep_time);
        }
        
        // Кейс 3: Вывод аргумента для проверки экранирования кавычек/пробелов
        if (argc >= 5) {
            std::cout << argv[4] << std::flush;
        }

        return exit_code;
    }

    // Режим "Родителя" (Запуск тестов)
    std::string self_path = argv[0];
    int failed_tests = 0;

    std::cout << "=== ЗАПУСК ЛАБОРАТОРНЫХ ТЕСТОВ ===\n\n";

    // ----------------------------------------------------
    // Тест 1: Ребёнок на 1 сек, код 42; родитель в цикле вызывает pl_poll и спит по 100 мс
    // Ожидается: Родитель успел выполнить больше 3 итераций; код 42
    {
        Process proc;
        std::vector<std::string> args = { self_path, "--child", "42", "1000" };
        std::cout << "Тест 1: ";
        
        if (pl_start(&proc, args) == 0) {
            int iterations = 0;
            int exit_code = -1;
            while (!pl_poll(&proc, exit_code)) {
                iterations++;
                sleep_ms(100);
            }
            if (iterations > 3 && exit_code == 42) {
                std::cout << "[ OK ] (Итераций: " << iterations << ", Код: " << exit_code << ")\n";
            } else {
                std::cout << "[ FAIL ] (Итераций: " << iterations << ", Код: " << exit_code << ")\n";
                failed_tests++;
            }
        } else {
            std::cout << "[ FAIL ] (Не удалось запустить процесс)\n";
            failed_tests++;
        }
    }

    // ----------------------------------------------------
    // Тест 2: Ребёнок на 0.3 с, код 7; вызов pl_wait
    // Ожидается: Код 7
    {
        Process proc;
        std::vector<std::string> args = { self_path, "--child", "7", "300" };
        std::cout << "Тест 2: ";

        if (pl_start(&proc, args) == 0) {
            int code = pl_wait(&proc);
            if (code == 7) std::cout << "[ OK ] (Код: 7)\n";
            else { std::cout << "[ FAIL ] (Код: " << code << ")\n"; failed_tests++; }
        } else { std::cout << "[ FAIL ]\n"; failed_tests++; }
    }

    // ----------------------------------------------------
    // Тест 3: Аргумент hello "big" world\ 
    // Ожидается: Ребёнок получил его без искажений (проверяем код возврата 0)
    {
        Process proc;
        // Передаем сложную строку с кавычками и пробелами
        std::string complex_arg = "hello \"big\" world\\";
        std::vector<std::string> args = { self_path, "--child", "0", "0", complex_arg };
        std::cout << "Тест 3: ";

        if (pl_start(&proc, args) == 0) {
            int code = pl_wait(&proc);
            if (code == 0) std::cout << "[ OK ] (Строка передана успешно)\n";
            else { std::cout << "[ FAIL ] (Код: " << code << ")\n"; failed_tests++; }
        } else { std::cout << "[ FAIL ]\n"; failed_tests++; }
    }

    // ----------------------------------------------------
    // Тест 4: Запуск несуществующей программы no_such_program_12345
    // Ожидается: pl_start вернула -1
    {
        Process proc;
        std::vector<std::string> args = { "no_such_program_12345" };
        std::cout << "Тест 4: ";

        int res = pl_start(&proc, args);
        if (res == -1) {
            std::cout << "[ OK ] (pl_start вернула -1)\n";
        } else {
            std::cout << "[ FAIL ] (pl_start вернула " << res << ")\n";
            failed_tests++;
        }
    }

    // ----------------------------------------------------
    // Тест 5: Ребёнок с кодом 300
    // Ожидается: Linux: 44 (код обрезается до 8 бит: 300 % 256 = 44); Windows: 300
    {
        Process proc;
        std::vector<std::string> args = { self_path, "--child", "300", "0" };
        std::cout << "Тест 5: ";

        if (pl_start(&proc, args) == 0) {
            int code = pl_wait(&proc);
#ifdef _WIN32
            if (code == 300) std::cout << "[ OK ] (Windows вернул 300)\n";
            else { std::cout << "[ FAIL ] (Ожидалось 300, получено " << code << ")\n"; failed_tests++; }
#else
            if (code == 44) std::cout << "[ OK ] (Linux вернул 44)\n";
            else { std::cout << "[ FAIL ] (Ожидалось 44, получено " << code << ")\n"; failed_tests++; }
#endif
        } else { std::cout << "[ FAIL ]\n"; failed_tests++; }
    }

    std::cout << "\nВсего ошибок: " << failed_tests << "\n";
    return (failed_tests == 0) ? 0 : 1;
}
