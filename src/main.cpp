#include <iostream>
#include <fstream>
#include "iunb.h"
int main() try
{
    std::string login;		std::cout << "Login: ";		std::cin >> login;
    std::string password;	std::cout << "Password: ";	std::cin >> password;

    IUNB my(login, password, "iunb.xml");
    std::string result = my.get_unread();
    std::ofstream file("unread.html", std::ios::trunc);
    file.write(result.data(), result.size());
    return 0;
}
catch (std::exception & exc)
{
    std::cout << '\n' << exc.what() << '\n';
}
