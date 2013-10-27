#include "iunb.h"
#include "prog_end.h"
int main() try
{
	Brainheav::sock_heplers::WinSockInit(MAKEWORD(2,2));

	std::string login;		std::cout << "Login: ";		std::cin >> login;
	std::string password;	std::cout << "Password: ";	std::cin >> password;
	std::string filename;	std::cout << "Filename: ";	std::cin >> filename; filename+=".html";

	Brainheav::iunb my(login, password, "iunb.xml");
	std::string res=my.get_undread();

	std::ofstream file(filename);
	file.write(res.data(), res.size());
	file.close();

	system(filename.c_str());
}
catch (Brainheav::sock_heplers::WSErr & exc)
{
	std::cout << '\n' << exc.what() << ' ' << exc.err << '\n';
}
catch (std::exception & exc)
{
	std::cout << '\n' << exc.what() << '\n';
}
catch (...)
{
	std::cout << "Unknown exception";
}