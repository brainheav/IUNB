IUNB
====

If you don't speak russian:

Sorry, but this program exists exclusively for one website with only russian interface. I guess you can still use my code as some example of C++ with socket and GUI. I have done my best so there are english comments in my code.

I use this libraries:

Qt - http://qt-project.org/

boost::tcp - http://www.boost.org/


Что это?

Это программа для получения книг из списка "Лучшие" с сайта imhonet, исключая книги, которые с оценкой и книги, которые находятся в списках исключений на компьютере пользователя.

Как запустить:

1. Скачать в релизах архив IUNB-X.X.X.EXE.(zip|7z)
2. Распаковать и запустить exe.
3. Иногда требуется установить http://www.microsoft.com/ru-ru/download/details.aspx?id=30679


Как использовать:

1. Нажать на кнопку Authorize, если хотите получить список книг, исключая те, которые с оценками или использовать $username.pref.xml и списки исключений $username.lists.txt. Можно запомнить пароль и логин, если заменить $login и $password в файле настроек, упомянутом выше.
2. Нажать на кнопку Get Unread. В колонку слева начнут добавляться записи.
3. Кликайте по интересующему названию и программа загрузит данные с сайта - описание и лучшие комментарии.
4. Чтобы добавить книгу в список, выберите нужные книги и нажмите на кнопку со списком, который нужен.

ToDo

1. Возможность менять настройки и создавать списки с помощью графического интерфейса.
