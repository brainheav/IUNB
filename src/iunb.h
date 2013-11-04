#ifndef IUNB_H
#define IUNB_H

// Headers
///////////////////////////////////////////////////////////////////////////////

#include <QMainWindow>

// Qt MOC conflicts with boost
#ifndef Q_MOC_RUN

// Network:
#include <boost/asio.hpp>

// Application preferences
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#endif // Q_MOC_RUN

// Holds exception from worker threads in list<future<void>>
#include <future>
// QListWidgetItem has QVariant with shared_ptr
// that contains some info about book
#include <memory>
// Load\save settings, lists, etc.
#include <fstream>
// Stores book's id to exclude
#include <unordered_set>

// !Headers
///////////////////////////////////////////////////////////////////////////////


// Forward declarations
///////////////////////////////////////////////////////////////////////////////

class QListWidgetItem;
class QActionGroup;

// !Forward declarations
///////////////////////////////////////////////////////////////////////////////

namespace Ui
{
    class IUNB;
}

class IUNB : public QMainWindow
{
    Q_OBJECT
public:
    // Qt::UserRole data in QListWidgetItem's QVariant
    struct item_info
    {
    public:
        item_info(unsigned long long in_id):id(in_id){}
    public:
        // Book's description and best comments from server
        QString str;
        // Book's id
        unsigned long long id;
    }; // !struct item_info

    // QVariant likes to copy everything everytime
    // but I - don't
    typedef std::shared_ptr<item_info> pit_inf;
    typedef std::shared_ptr<std::ofstream> pofstr;

private:
    // All configuration file's names are made from this string
    std::string username;

    // Needed for boost IO operations
    boost::asio::io_service io_service;

    // Stores preferences from $username.pref.xml
    boost::property_tree::ptree xml_pref;

    // Stores auth
    std::string cookie;

    // Holds exception from worker threads, if any
    // also I can wait for threads to finish
    std::list<std::future<void>> tasks_list;

    // Stores book's id to exclude
    std::unordered_set<unsigned long long> excl_id;

    // Stores new book's id to exclude
    // unloads to main (excl_id) list when necessary
    std::unordered_set<unsigned long long> new_excl_id;

    // Actions related to exclude lists
    QActionGroup * excl_lists;

    //
    Ui::IUNB *ui;

private:
    // Wait for threads to finish
    // This makes possible to change settings, cookie, etc without reload
    void wait_for_tasks();

    // Load settings or create default
    void load_settings();

    // Load exclude lists
    void load_lists();

    //Unload new exclude book's id
    void unload_new_excl_id();

    // Connect, send auth info, save parsed reply to cookie to be auth.
    void authorize (const std::string &login,
                    const std::string &password);

    // Run authorize (...) asynchronously
    void async_authorize (const std::string &login,
                          const std::string &password);

    // Waiting either 1 second or until socket buffer is full
    // and return size of available bytes
    static size_t wait_for_reply (const boost::asio::ip::tcp::socket &socket);

    // Add to out_str cookie sequence from src,
    // that begins with beg_req and end with ';'
    static void add_cookie(std::string &out_str,
                           const std::string &src,
                           const std::string &beg_req);

    // Send GET with cookie
    // Get reply and parse it for unread books
    // Repeat until count(unread books) < num from xml settings
    void get_unread();

    // Run get_unread() asynchronously
    void async_get_unread();

    // Parse string for unread books
    void parse_for_unread(const std::string &src, size_t beg_search,
                          size_t &out_count);

    // Get book's description and best comments
    void get_book_info(QListWidgetItem *item);

    // Run get_book_info() asynchronously
    void async_get_book_info(QListWidgetItem *item);

    // Get book's description from reply
    bool parse_for_descr(std::string &out_src, size_t beg_serch);

    // Append to string html node with tag if "with" == true
    static std::string &add_by_tag(const std::string &src,
                                   std::string &out_str,
                                   std::string o_tag, size_t &in_beg_pos,
                                   bool with);

    // Get book's best comments from reply
    void parse_for_comm(const std::string &src,
                        std::string &out_str);

public:
    //
    explicit IUNB(QWidget *parent = 0);
    //
    ~IUNB();

signals:
    // Signal to change status in status bar
    void status_prepared(QString status);
    // Signal to add unread book in list widget
    void book_found (QListWidgetItem *item);
    // Signal to update cookie
    void cookie_updated(QByteArray new_cookie);
    // Signal to display new info about book
    void book_info_updated(QListWidgetItem *item);

private slots:
    // Authorize Action
    void on_A_Authorize_triggered();
    // Get Unread Action
    void on_A_Get_Unread_triggered();
    // Load item's book best comments and description
    void on_W_unread_list_itemClicked(QListWidgetItem *item);
    // Update status bar and log to file
    void on_IUNB_status_prepared(QString status);
    // Add book to list widget
    void on_IUNB_book_found(QListWidgetItem *item);
    // Update cookie
    void on_IUNB_cookie_updated(QByteArray new_cookie);
    // Update book's info
    void on_IUNB_book_info_updated(QListWidgetItem *item);
    // Add book to exclude list
    void add_exclude_book (QAction *action);
};

// QVariant is too proud of himself to be really Variant
// so this allows QVariant to contain shared_ptr
// to struct with book's id, description and best comments
Q_DECLARE_METATYPE(IUNB::pit_inf)
Q_DECLARE_METATYPE(IUNB::pofstr)

#endif // IUNB_H
