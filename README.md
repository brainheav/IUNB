IUNB
====

If you don't speak russian:

Sorry, but this program exists exclusively for one website with only russian interface. I guess you can still use my code as some example of C++ with socket and GUI (in future). I have done my best so there are english comments in my code.

In previous version program was windows-only and used WinSock. Now this is boost::tcp and you can find this library here - http://www.boost.org/. 

Цель программы - получить с сайта imhonet книги из списка "Лучшие", но исключая уже прочитанные.

Как запустить:

1. Скачать в релизах архив Executable.zip
2. Распаковать и запутсть exe.
3. Иногда требуется установить http://www.microsoft.com/ru-ru/download/details.aspx?id=30679


Как использовать:

1. Ввести пароль, логин Имхонета.
2. Подождать, пока программа обрабатывает данные с сайта.
3. Список первых 10 непрочитанных будет сохранён в файле unread.html рядом с программой.
4. Его можно открыть в любом браузере.

ToDo

1. Графический интерфейс.
3. Возможность добавлять книги в списки: Прочитанное, Чёрный, Позже.
3.А. Эти списки будут храниться на компьютере пользователя.
3.Б. Книги из этих списков не будут показываться в программе.