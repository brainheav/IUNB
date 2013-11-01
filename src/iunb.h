#ifndef IUNB_H
#define IUNB_H

#include <QMainWindow>

#ifndef Q_MOC_RUN

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#endif // Q_MOC_RUN

#include <future>
#include <memory>

class QListWidgetItem;

namespace Ui {
class IUNB;
}

class IUNB : public QMainWindow
{
    Q_OBJECT
public:
    // User-role data in QListWidgetItem
    struct item_info
    {
    public:
        item_info(unsigned long long in_id):id(in_id){}
    public:
        // Inf about book from server
        QString str;
        // id of book
        unsigned long long id;
    }; // !struct item_info

    //
    typedef std::shared_ptr<item_info> pit_inf;

private:
    // All settings files are made from this string
    std::string username;

    // Needed for BOOST IO operations
    boost::asio::io_service io_service;

    // Saving pref from XML
    boost::property_tree::ptree xml_pref;

    // Save auth across function call
    std::string cookie;

    // Hold exception from worker threads, if any
    std::list<std::future<void>> tasks_list;

    // Interface
    Ui::IUNB *ui;

private:
    // Waiting for finising other threads
    // This makes possible to change settings, cookie, etc without reload
    void wait_for_tasks();

    // Load settings or create default
    void load_settings();

    // Connect, send auth inf, save parsed reply to cookie to be auth.
    void authorize (const std::string &login,
                    const std::string &password);

    // Run authorize (...) asynchronously
    void async_authorize (const std::string &login,
                          const std::string &password);

    // Waiting either 1 second or until socket buffer is full
    // and return aize of available bytes
    static size_t wait_for_reply (const boost::asio::ip::tcp::socket & socket);

    // Add to *out_str cookie sequence from src,
    // that begin with beg_req
    static void add_cookie(std::string & out_str,
                           const std::string & src,
                           const std::string & beg_req);

    // Send GET with cookie
    // GET reply and parse it for unread books
    // Repeat until count(unread books) < num from xml settings
    void get_unread();

    // Run get_unread() asynchronously
    void async_get_unread();

    // Parse string for unread books
    void parse_for_unread(const std::string & src, size_t beg_search,
                          size_t & out_count);

    // Get info (description, comments)
    void get_book_info(QListWidgetItem *item);

    // Run get_book_info() asynchronously
    void async_get_book_info(QListWidgetItem *item);

    // Parse description
    bool parse_for_descr(std::string & out_src, size_t beg_serch);

    // Append to string html node
    // include tag if with == true
    static std::string &add_by_tag(const std::string &src,
                                   std::string &out_str,
                                   std::string o_tag, size_t &in_beg_pos,
                                   bool with);

    // Parse comments
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
    void book_found (QListWidgetItem * item);
    // Signal to change cookie to new
    void cookie_updated(QByteArray new_cookie);
    // Slot to update book info
    void book_info_updated(QListWidgetItem * item);

private slots:
    // Authorize Action
    void on_A_Authorize_triggered();
    // Get Unread Action
    void on_A_Get_Unread_triggered();
    // Load info about book in item
    void on_W_unread_list_itemClicked(QListWidgetItem *item);
    // Show message in status bar and write to log
    void on_IUNB_status_prepared(QString status);
    // Log message from status bar
    void on_IUNB_book_found(QListWidgetItem *item);
    // Update cookie
    void on_IUNB_cookie_updated(QByteArray new_cookie);
    // Update book_info
    void on_IUNB_book_info_updated(QListWidgetItem * item);
};

Q_DECLARE_METATYPE(std::shared_ptr<IUNB::item_info>)

#endif // IUNB_H
