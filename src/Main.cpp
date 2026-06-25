#include <iostream>
#include <sstream>
#include <string>

#include "database/Basalt.hpp"

static void usage() {
    std::cout << "Commands:" << std::endl;
    std::cout << "  put <key> <val>" << std::endl;
    std::cout << "  get <key>" << std::endl;
    std::cout << "  del <key>" << std::endl;
    std::cout << "  scan <start> <end>" << std::endl;
    std::cout << "  help" << std::endl;
    std::cout << "  exit" << std::endl;
}

int main() {
    Basalt db("BasaltDB");
    std::cout << "Welcome to BasaltDB!\n";
    usage();

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line))
            break;

        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "exit" || cmd == "quit") {
            break;
        }

        if (cmd == "help") {
            usage();
            continue;
        }

        if (cmd == "put") {
            long long key_raw, val_raw;
            if (!(iss >> key_raw >> val_raw)) {
                std::cout << "Usage: put <key> <val>\n";
                continue;
            }

            int rc = db.put((Key)key_raw, (Val)val_raw);
            if (rc == 0)
                std::cout << "OK\n";
            else
                std::cout << "ERROR\n";
            continue;
        }

        if (cmd == "get") {
            long long key_raw;
            if (!(iss >> key_raw)) {
                std::cout << "Usage: get <key>\n";
                continue;
            }

            auto v = db.get((Key)key_raw);
            if (v.has_value())
                std::cout << v.value() << "\n";
            else
                std::cout << "NULL\n";
            continue;
        }

        if (cmd == "del") {
            long long key_raw;
            if (!(iss >> key_raw)) {
                std::cout << "Usage: del <key>\n";
                continue;
            }

            int rc = db.del((Key)key_raw);
            if (rc == 0)
                std::cout << "OK\n";
            else
                std::cout << "ERROR\n";
            continue;
        }

        if (cmd == "scan") {
            long long start_raw, end_raw;
            if (!(iss >> start_raw >> end_raw)) {
                std::cout << "Usage: scan <start> <end>\n";
                continue;
            }

            auto rows = db.scan((Key)start_raw, (Key)end_raw);
            for (const auto& row : rows)
                std::cout << row.first << " " << row.second << "\n";
            std::cout << "Count " << rows.size() << "\n";
            continue;
        }

        std::cout << "Unknown command\n";
    }
}
