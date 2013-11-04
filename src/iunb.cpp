// Headers
///////////////////////////////////////////////////////////////////////////////

// Main user interface
#include "iunb.h"
#include "ui_iunb.h"

// Login and password dialog interface
#include "log_pass.h"
#include "ui_log_pass.h"

// Replace algorithm
#include <boost/algorithm/string.hpp>

// !Headers
///////////////////////////////////////////////////////////////////////////////


// IUNB Private functions
///////////////////////////////////////////////////////////////////////////////

// Wait for threads to finish
// This makes possible to change settings, cookie, etc without reload
void IUNB::wait_for_tasks()
{
    //
    emit status_prepared("General: Waiting until tasks finished, stand by");

    // Display exception what() in status bar, if any
    while (tasks_list.size())
    {
        tasks_list.remove_if([this](std::future<void> &ref)
        {
            if (ref.wait_for(std::chrono::seconds(0))!=std::future_status::timeout)
            {
                try
                {
                    ref.get();
                }
                catch (std::exception &ref)
                {
                    emit status_prepared(QString("Fail: ").append(ref.what()));
                }
                catch (...)
                {
                    emit status_prepared("Fail: Unknown");
                }
                return true;
            }
            return false;
        });
    }

    //
    emit status_prepared("General: Tasks finished");
} // !void IUNB::wait_for_tasks()

// Load settings or create default
void IUNB::load_settings()
{
    //
    std::string xml_filename("./rsrc/");
    xml_filename += username + ".pref.xml";
    //
    wait_for_tasks();
    //
    emit status_prepared("XML: Loading");
    //
    try
    {
        boost::property_tree::read_xml(xml_filename, xml_pref);
    }
    catch (boost::property_tree::xml_parser_error &)
    {
        //
        emit status_prepared("XML: File not found, create default ");
        //
        std::ifstream deffile("./rsrc/.pref.xml");
        std::ofstream userfile(xml_filename);
        userfile << deffile.rdbuf();
        //
        deffile.close(); userfile.close();
        //

        //
        emit status_prepared("XML: Default file created ");
        boost::property_tree::read_xml(xml_filename, xml_pref);
    }
    //
    emit status_prepared("XML: Loaded");

    // Load exclude lists
    load_lists();
} // !void IUNB::load_settings()

void IUNB::load_lists()
{
    //
    emit status_prepared("Exclude lists: Loading");
    // Load lists file
    std::string all_lists_filename("./rsrc/lists/");
    all_lists_filename+=username+".lists.txt";
    std::ifstream all_lists_file(all_lists_filename);

    // Create default if this doesn't exist
    if (!all_lists_file.is_open())
    {
        //
        emit status_prepared("Exclude lists: File not found, creating default");
        //
        all_lists_file.close();
        all_lists_file.clear();
        //
        std::ifstream deffile("./rsrc/lists/.lists.txt");
        std::ofstream userfile(all_lists_filename);
        userfile << deffile.rdbuf();
        //
        deffile.close(); userfile.close();
        //
        all_lists_file.open(all_lists_filename);
        //
        emit status_prepared("Exclude lists: Default file created");
    }

    // Refresh QAction in QActionGroup excl_lists
    if (excl_lists) delete excl_lists;
    excl_lists = new QActionGroup(this);
    connect(excl_lists, SIGNAL(triggered(QAction*)),
            this, SLOT(add_exclude_book(QAction*)));
    // and clear old lists
    excl_id.clear();
    new_excl_id.clear();

    // Related to exclude list file QAction
    QAction * pQA;
    // and shared_ptr to this file
    pofstr shp_ofstr;
    // Filename of this file
    std::string list_filename;
    // Name of this action
    std::string list_name;

    // Used to load id from exclude file into excl_id set
    std::ifstream list_file;
    unsigned long long book_id;

    // Add action to every file in ...lists.txt
    while (all_lists_file)
    {
        // Get list's filename
        getline(all_lists_file, list_filename, ';');
        boost::replace_first(list_filename, "$username", username);
        // Get list's name - action's text
        getline(all_lists_file, list_name);
        // if (end)
        if (!all_lists_file) break;

        // Load book id from exclude lists
        list_file.open(list_filename);
        while (list_file)
        {
            list_file >> book_id;
            excl_id.emplace(book_id);
            list_file.ignore(INTMAX_MAX, '\n');
        }
        list_file.close();
        list_file.clear();

        // Create action and display
        pQA = new QAction(list_name.c_str(), excl_lists);
        shp_ofstr.reset(new std::ofstream(list_filename, std::ios::app));
        pQA->setData(QVariant::fromValue(shp_ofstr));
        ui->TB_main->addAction(pQA);
    }

    //
    emit status_prepared("Exclude lists: Loaded");
} // !void IUNB::load_lists()

//Unload new exclude book's id
void IUNB::unload_new_excl_id()
{
    //
    wait_for_tasks();

    for (auto i : new_excl_id)
    {
        excl_id.emplace(i);
    }

    new_excl_id.clear();
} // !void IUNB::unload_new_excl_id()

// Connect, send auth info, save parsed reply to cookie to be auth.
void IUNB::authorize(const std::string &login, const std::string &password)
{
    //
    emit status_prepared("Authorize: Starting");

    // Get from XML POST request
    // and replace $password and $login with user input if any
    std::string post_req = xml_pref.get<std::string>("pref.auth.POST");
    boost::replace_first(post_req, "$login", login);
    boost::replace_first(post_req, "$password", password);

    // Get from XML GET request
    // and replace $Content-Length with size of post request
    std::string get_req = xml_pref.get<std::string>("pref.auth.GET");
    boost::replace_first(get_req, "$Content-Length",
                         std::to_string(post_req.size()));

    // Add POST to GET
    get_req+=post_req;

    //
    emit status_prepared("Authorize: Connecting");

    // Standart TCP socket
    boost::asio::ip::tcp::socket socket(io_service);
    // Query = imhonet.ru:80 if default.
    boost::asio::ip::tcp::resolver::query query(
                xml_pref.get<std::string>("pref.site.addr"),
                xml_pref.get<std::string>("pref.site.port"));
    // Connect to machine-like endpoint range
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::connect(socket,resolver.resolve(query));
    // Send GET request with POST data from above
    boost::asio::write(socket, boost::asio::buffer(get_req));
    // Size of this string will be equal to bytes in socket
    std::string reply(wait_for_reply(socket), '\0');
    // Get reply from server with user id, user hash and phpsid
    boost::asio::read(socket, boost::asio::buffer(&reply[0], reply.size()));

    //
    emit status_prepared("Authorize: Parsing reply");

    // Make cookie
    std::string new_cookie("Cookie:");
    // Add to cookie user_id from reply if any
    add_cookie(new_cookie, reply,
               xml_pref.get<std::string>("pref.auth.user_id"));
    // Add to cookie user_hash from reply if any
    add_cookie(new_cookie, reply,
               xml_pref.get<std::string>("pref.auth.user_hash"));
    // Add to cookie PHPSID from reply if any
    add_cookie(new_cookie, reply,
               xml_pref.get<std::string>("pref.auth.PHPSID"));

    //
    emit status_prepared("Authorize: Parsed");

    //
    emit cookie_updated(QByteArray(new_cookie.c_str(), new_cookie.size()));
} // !void IUNB::authorize(...)

// Run authorize (...) asynchronously
void IUNB::async_authorize(const std::string &login,
                           const std::string &password)
{
    tasks_list.emplace_back(std::async(std::launch::async,
                                       &IUNB::authorize, this, login, password));
} // !void IUNB::async_authorize(...)

// Waiting either 1 second or until socket buffer is full
// and return size of available bytes
size_t IUNB::wait_for_reply(const boost::asio::ip::tcp::socket &socket)
{
    boost::asio::ip::tcp::socket::receive_buffer_size buf_size;
    socket.get_option(buf_size);
    size_t size = socket.available();
    for (size_t wait_count(0), prev_size(0); wait_count<10; prev_size=size)
    {
        if (prev_size==size)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++wait_count;
        } else wait_count=0;
        size = socket.available();
        if (size==buf_size.value()) break;
    }
    return size;
} // !size_t IUNB::wait_for_reply(...)

// Add to out_str cookie sequence from src,
// that begins with beg_req and end with ';'
void IUNB::add_cookie(std::string &out_str, const std::string &src,
                      const std::string &beg_req)
{
    auto beg_pos = src.find(beg_req);
    if (beg_pos!=src.npos)
    {
        auto end_pos = src.find(';', beg_pos);
        if (end_pos!=src.npos)
        {
            out_str+=' ';
            out_str.append(src, beg_pos, ++end_pos-beg_pos);
        }
    }
} // !void IUNB::add_cookie(...)

// Send GET with cookie
// Get reply and parse it for unread books
// Repeat until count(unread books) < num from xml settings
void IUNB::get_unread()
{
    //
    emit status_prepared("Unread: Starting");

    // GET request without cookie and page_num yet
    std::string get_req = xml_pref.get<std::string>("pref.unread.GET");
    // and replace $Cookie with cookie
    boost::replace_first(get_req, "$Cookie", cookie);

    // Find pos for replacing $pagenumber with 1-...N+1 in for () below
    // This allow to visit next page
    size_t len = 0;
    char c_page_num[50];
    size_t pos = get_req.find("$pagenumber");
    if (pos!=get_req.npos)
        len=strlen("$pagenumber");

    // Buffer for receiving from server
    std::string buf;

    // Start searching from this page
    size_t page_num = xml_pref.get<size_t>("pref.unread.start_page");
    // Minimum desired number of unread books
    size_t num = xml_pref.get<size_t>("pref.unread.num");

    // Used in for(...) below
    size_t offs(0), prev_offs(0), size(0), beg_search(0);

    // Standart TCP socket
    boost::asio::ip::tcp::socket socket(io_service);
    // Query = imhonet.ru:80 if default.
    boost::asio::ip::tcp::resolver::query query(
                xml_pref.get<std::string>("pref.site.addr"),
                xml_pref.get<std::string>("pref.site.port"));
    // Connect to machine-like endpoint range
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::connect(socket,resolver.resolve(query));

    // count - unread books. num - desired.
    for (size_t count(0); count < num; )
    {
        // Convert integer page_num to c-style string c_page_num,
        // and replace $pagenumber with it
        // pos from above, if any
        if (pos!=get_req.npos)
        {
            _itoa_s(page_num++, c_page_num, 10);
            get_req.replace(pos, len, c_page_num);
            len=strlen(c_page_num);
        }

        //
        emit status_prepared(QString("Unread: Page processing ")
                           .append(c_page_num));

        // Send GET request with cookie data from above
        boost::asio::write(socket, boost::asio::buffer(get_req));

        // Get Reply and parse it
        while (size=wait_for_reply(socket))
        {
            // Resize if not enough
            if (buf.size()-offs < size) buf.resize((buf.size()+size)*2);
            // Get reply and save to string with offset - offs
            boost::asio::read(socket,
                              boost::asio::buffer(&buf[offs], size));
            offs+=size;

            // 14 == strlen("data-rate=\"\"")
            parse_for_unread(buf, beg_search, count);
            beg_search=offs-14;

            // Have I received any?
            if (offs!=prev_offs) prev_offs=offs;
            else break;
        }// !while (...)
    }// !for (...)

} // !void IUNB::get_unread()

// Run get_unread() asynchronously
void IUNB::async_get_unread()
{
    tasks_list.emplace_back(std::async(std::launch::async,
                                       &IUNB::get_unread, this));
} // !void IUNB::async_get_unread()

// Parse string for unread books
void IUNB::parse_for_unread(const std::string &src,
                            size_t beg_search,
                            size_t &out_count)
{
    //
    auto beg_pos = src.npos;
    auto end_pos = src.npos;
    //
    QListWidgetItem *item;
    std::shared_ptr<item_info> it_inf;
    //
    unsigned long long id;

     for (  auto unread_book_pos = src.find("data-rate=\"\"", beg_search);
            unread_book_pos!=src.npos;
            unread_book_pos = src.find("data-rate=\"\"", ++unread_book_pos))
     {
         // URL with required inf
         beg_pos = src.rfind("<a href=\"", unread_book_pos);
         if (beg_pos==src.npos) continue;

         // Get book id
         while (!isdigit(src[++beg_pos]));
         id = strtoul(src.c_str()+beg_pos, 0, 10);

         // Is this a book from exclude lists?
         if (excl_id.count(id)) continue;

         // Get book name
         beg_pos = src.find('>', beg_pos);
         if ((beg_pos++)==src.npos) continue;
         while (src[++beg_pos]==' ');
         end_pos = src.find('<', beg_pos);
         if (end_pos==src.npos) continue;
         while (src[--end_pos]==' ');

         // Add item to list widget
         item = new QListWidgetItem(QString::fromUtf8(src.c_str()+beg_pos,
                                                      end_pos-beg_pos));
         it_inf.reset(new item_info(id));
         item->setData(Qt::UserRole,
                       QVariant::fromValue(it_inf));
         emit book_found(item);

         ++out_count;
     }

} // !void IUNB::parse_for_unread(...)

// Get book's description and best comments
void IUNB::get_book_info(QListWidgetItem *item)
{
    // Item_info inside item
    pit_inf it_inf = item->data(Qt::UserRole).value<pit_inf>();

    // Book id
    const std::string id = std::to_string(it_inf->id);

    // Get description
    std::string descr;
    std::future<void> descr_task(std::async(std::launch::async,
                                            [this, &descr, &id]()
    {
        //
        emit status_prepared("Book info: Getting description");

        // GET request
        descr = xml_pref.get<std::string>("pref.book_info.GET_descr");
        // replace $id with id
        boost::replace_first(descr, "$id", id);

        // Standart TCP socket
        boost::asio::ip::tcp::socket socket(io_service);
        // Query = imhonet.ru:80 if default.
        boost::asio::ip::tcp::resolver::query query(
                    xml_pref.get<std::string>("pref.site.addr"),
                    xml_pref.get<std::string>("pref.site.port"));
        // Connect to machine-like endpoint range
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::connect(socket,resolver.resolve(query));
        // Send GET request
        boost::asio::write(socket, boost::asio::buffer(descr));

        // Used in while (...) below
        size_t offs(0), prev_offs(0), size(0), beg_search(0);
        // Get reply and parse it
        while (size=wait_for_reply(socket))
        {
            // Resize if not enough
            if (descr.size()-offs < size) descr.resize((descr.size()+size)*2);
            // Get reply and save to string with offset - offs
            boost::asio::read(socket,
                              boost::asio::buffer(&descr[offs], size));
            offs+=size;

            // Return true if description found
            if (parse_for_descr(descr, beg_search))
            {
                break;
            }
            else
            {
                // Have I received any?
                if (offs!=prev_offs)
                {
                    // 34 = strlen("data-content=\"Похожие книги\">")
                    prev_offs=offs;
                    beg_search=offs-34;
                }
                else // Descriprion is not found and never will be
                {
                    // So clear string
                    // then this allow to reload description
                    // because empty means never loaded before
                    descr.clear();
                    break;
                }
            }
        }// !while (...)

    })); // !Get description

    // Get comments
    std::string comm;
    std::future<void> comm_task(std::async(std::launch::async,
                                            [this, &comm, &id]()
    {
        std::string buf;
        //
        emit status_prepared("Book info: Getting comments");

        // GET request
        buf = xml_pref.get<std::string>("pref.book_info.GET_comm");
        // replace $id with id
        boost::replace_first(buf, "$id", id);

        // Standart TCP socket
        boost::asio::ip::tcp::socket socket(io_service);
        // Query = imhonet.ru:80 if default.
        boost::asio::ip::tcp::resolver::query query(
                    xml_pref.get<std::string>("pref.site.addr"),
                    xml_pref.get<std::string>("pref.site.port"));
        // Connect to machine-like endpoint range
        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::connect(socket,resolver.resolve(query));
        // Send GET request
        boost::asio::write(socket, boost::asio::buffer(buf));

        // Used in while (...) below
        size_t offs(0), prev_offs(0), size(0);
        // Get reply
        while (size=wait_for_reply(socket))
        {
            // Resize if not enough
            if (buf.size()-offs < size) buf.resize((buf.size()+size)*2);
            // Get reply and save to string with offset - offs
            boost::asio::read(socket,
                              boost::asio::buffer(&buf[offs], size));
            offs+=size;

            // Have I received any?
            if (offs!=prev_offs) prev_offs=offs;
            else break;
        }

        // and parse it
        parse_for_comm(buf, comm);

    })); // !Get comments

    // Waiting...
    descr_task.wait(); comm_task.wait();

    //
    if (descr.size())
    {
        descr+="<a href=\"http://books.imhonet.ru/element/";
        descr+=id;
        descr+="\">Посмотреть книгу на сайте</a>";
        descr+=comm;
    }
    // Set item_info string
    it_inf->str = QString::fromUtf8(descr.c_str(), descr.size());

    // Display received
    emit book_info_updated(item);

    // If description or a comments task have exception
    // exception will be thrown and stored in tasks_list
    descr_task.get(); comm_task.get();
} // !void IUNB::get_book_info()

// Run get_book_info() asynchronously
void IUNB::async_get_book_info(QListWidgetItem *item)
{
    tasks_list.emplace_back(std::async(std::launch::async,
                                       &IUNB::get_book_info, this, item));
} // !void IUNB::async_get_book_info()

// Get book's description from reply
bool IUNB::parse_for_descr(std::string &out_src, size_t beg_serch)
{
    // All the necessary information is found
    auto end_pos = out_src
            .find("data-content=\"Похожие книги\">",
                   beg_serch);
    if (end_pos == out_src.npos) return false;
    auto beg_pos = out_src.rfind("<span class=\"fn\">", end_pos);
    if (beg_pos == out_src.npos) return false;

    //
    emit status_prepared("Book info: Parsing description");

    // Book name
    std::string temp = "<center><h1>";
    add_by_tag(out_src, temp, "<span class=\"fn\">", beg_pos, 0)
    += "</h1></center>";

    // Average rating
    temp+="Оценка: ";
    add_by_tag(out_src, temp, "<span class=\"average\">", beg_pos, 0)
    += "<br>";

    // Votes
    temp+="Проголосовавших: ";
    add_by_tag(out_src, temp, "<span class=\"votes\">", beg_pos, 0)
    += "<br>";

    // Description
    add_by_tag(out_src, temp, "<p class=\"summary\">", beg_pos, 1)
    += "<br>";

    out_src = std::move(temp); return true;
} // !bool IUNB::parse_for_descr(...)

// Append to string html node with tag if "with" == true
std::string &IUNB::add_by_tag(const std::string &src, std::string &out_str,
                              std::string o_tag, size_t &in_beg_pos,
                              bool with)
{
    // Find string with full tag
    auto beg_pos = src.find(o_tag, in_beg_pos);
    if (beg_pos == src.npos) return out_str;

    // Make short tag from full
    o_tag = std::string(o_tag, 0, o_tag.find_first_of(" >"));
    // and close tag
    std::string c_tag(o_tag);
    // <tag -> </tag>
    c_tag.insert(1,1, '/')+='>';

    // Find correct position of close tag
    auto end_pos = beg_pos;
    for (size_t count(1); count; )
    {
        end_pos = src.find('<', ++end_pos);
        if (end_pos == src.npos) return out_str;

        // It is close tag
        if (!memcmp(src.data()+end_pos,
                    c_tag.data(), c_tag.size()))
        {
            --count;
        }
        // It is open tag
        else if (!memcmp(src.data()+end_pos,
                         o_tag.data(), o_tag.size()))
        {
            ++count;
        }
    } // !for (...)

    // With tag
    if (with)
    {
        end_pos = src.find('>', end_pos);
        out_str.append(src, beg_pos, ++end_pos - beg_pos);
    }
    // without
    else
    {
        beg_pos = src.find('>', beg_pos);
        out_str.append(src, ++beg_pos, end_pos - beg_pos);
    }
    //
    in_beg_pos = end_pos;
    return out_str;
} // !std::string &IUNB::add_by_tag(...)

// Get book's best comments from reply
void IUNB::parse_for_comm(const std::string &src, std::string &out_str)
{

    auto comm_pos = src.find("<div class=\"m-comments-item-body\">");
    auto beg_pos = comm_pos;
    auto end_pos = comm_pos;

    //
    emit status_prepared("Book info: Parsing comments");

    while (comm_pos != src.npos)
    {
        //
        out_str+="<hr><hr>";
        // Rating
        beg_pos = src.find("<span class=\"m-comments-rating\">", beg_pos);
        if (beg_pos == src.npos) beg_pos = comm_pos;
        else
        {
            out_str+="Оценка: ";
            while (!isdigit(src[++beg_pos])); end_pos = beg_pos;
            while (isdigit(src[++end_pos]));
            out_str.append(src, beg_pos, end_pos - beg_pos) += "<br>";
        }

        // Comment
        out_str+="<blockquote>";
        add_by_tag(src, out_str, "<div class=\"m-comments-content\"",
                   beg_pos, 0) += "</blockquote>";



        comm_pos = src.find("<div class=\"m-comments-item-body\">",
                            beg_pos);
    }
} // !void IUNB::parse_for_comm(...)

// !IUNB Private functions
///////////////////////////////////////////////////////////////////////////////


// IUNB Publick functions
///////////////////////////////////////////////////////////////////////////////

//
IUNB::IUNB(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::IUNB),
    excl_lists(nullptr)
{

    ui->setupUi(this);
} // !IUNB::IUNB(...)

//
IUNB::~IUNB()
{
    wait_for_tasks();
    delete ui;
} // !IUNB::~IUNB()

// !IUNB Publick functions
///////////////////////////////////////////////////////////////////////////////


// IUNB Slots
///////////////////////////////////////////////////////////////////////////////

// Authorize Action
void IUNB::on_A_Authorize_triggered()
{
    // Get password and login from user
    log_pass lp(this); lp.exec();
    username             = lp.ui->Login->text().toStdString();
    std::string password = lp.ui->Password->text().toStdString();
    //
    load_settings();
    //
    async_authorize(username, password);
}// !void IUNB::on_A_Authorize_triggered()

// Get Unread Action
void IUNB::on_A_Get_Unread_triggered()
{
    // Load settings if not yet
    if (xml_pref.empty()) load_settings();
    // Unload new exclude book's id, if any
    if (new_excl_id.size()) unload_new_excl_id();
    //
    ui->W_unread_list->clear();
    //
    async_get_unread();
} // !void IUNB::on_A_Get_Unread_triggered()

// Load old or new info about book
void IUNB::on_W_unread_list_itemClicked(QListWidgetItem *item)
{
    // Item info inside item
    pit_inf it_inf = item->data(Qt::UserRole).value<pit_inf>();
    // Load book info
    if (it_inf->str.isNull())
    {
        it_inf->str = "<center><h1>Processing...</h1></center>";
        ui->TB_book_info->setText(it_inf->str);
        async_get_book_info(item);
    }
    else // or display if exist already
    {
        ui->TB_book_info->setText(it_inf->str);
    }
} // !void IUNB::on_W_unread_list_itemClicked(...)


// Show message in status bar and write to log
void IUNB::on_IUNB_status_prepared(QString status)
{
    //
    static std::ofstream log("./rsrc/iunb.txt", std::ios::app);
    log << status.toStdString() << std::endl;
    //
    ui->SB_status->showMessage(status);
} // !void IUNB::on_IUNB_status_prepared(...)

// Add book to list widget
void IUNB::on_IUNB_book_found(QListWidgetItem *item)
{
    //
    ui->W_unread_list->addItem(item);
} // !void IUNB::on_IUNB_book_found(...)

// Update cookie
void IUNB::on_IUNB_cookie_updated(QByteArray new_cookie)
{
    //
    emit status_prepared("Cookie: Updating");

    //
    wait_for_tasks();
    cookie = new_cookie.data();

    //
    emit status_prepared("Cookie: Updated");
} // !void IUNB::on_IUNB_cookie_updated(...)

// Update book's info
void IUNB::on_IUNB_book_info_updated(QListWidgetItem *item)
{
    // Load new info only if it is current item
    emit status_prepared("Book info: OK");
    if (ui->W_unread_list->currentItem() == item)
    {
        pit_inf it_inf = item->data(Qt::UserRole).value<pit_inf>();
        ui->TB_book_info->setText(it_inf->str);
    }
} // !void IUNB::on_IUNB_book_info_updated(...)

// Add book to exclude list
void IUNB::add_exclude_book(QAction *action)
{
    // Exclude file list related to this action
    pofstr shp_ofstr = action->data().value<pofstr>();
    // List with selected items, if any
    auto sel_items = ui->W_unread_list->selectedItems();
    //
    unsigned long long id;
    // Add to related exclude list new book id and delete book from list widget
    for (QListWidgetItem * i : sel_items)
    {
        id = i->data(Qt::UserRole).value<pit_inf>()->id;
       *shp_ofstr << id
                  << ';'
                  << i->text().toStdString()
                  << std::endl;
        new_excl_id.emplace(id);
        delete i;
    }

} // !void IUNB::add_ecxlude_book(...)

// !IUNB Slots
///////////////////////////////////////////////////////////////////////////////
