#include "iunb.h"
#include <thread>
#include <iostream>
#include <boost/algorithm/string.hpp>

// Connect, send auth inf, save parsed reply to cookie to be auth.
IUNB::IUNB(std::string login, std::string password, std::string in_xml_filename)
    :resolver(io_service)
{
    // Read XML preference file into XML container.
    boost::property_tree::read_xml(in_xml_filename, xml_pref);

    // Get from XML POST request
    // and replace $password and $login with user input if any
    std::string post_req = xml_pref.get<std::string>("pref.auth.POST");
    boost::replace_first(post_req, "$login", login);
    boost::replace_first(post_req, "$password", password);

    // Get from XML GET request
    // and replace $Content-Length with size of post request, if any
    std::string get_req = xml_pref.get<std::string>("pref.auth.GET");
    boost::replace_first(get_req, "$Content-Length",
                         std::to_string(post_req.size()));

    // Add to GET POST data
    get_req+=post_req;


    // Standart TCP socket
    boost::asio::ip::tcp::socket socket(io_service);
    // Query = imhonet.ru:80 if default.
    pquery.reset(
                new boost::asio::ip::tcp::resolver::query(
                    xml_pref.get<std::string>("pref.site.addr"),
                    xml_pref.get<std::string>("pref.site.port")));
    // Connect to machine-like endpoint range
    boost::asio::connect(socket,resolver.resolve(*pquery));
    // Send GET request with POST data from above
    boost::asio::write(socket, boost::asio::buffer(get_req));
    // Size of this string will be equal to size of data in socket buffer
    std::string reply(wait_for_reply(&socket), '\0');
    // Get reply from server with user id, user hash and phpsid
    boost::asio::read(socket, boost::asio::buffer(&reply[0], reply.size()));

    // Will be equal to "Cookie:" if default.
    cookie = xml_pref.get<std::string>("pref.auth.cookie");
    // Add to cookie user_id from reply if any
    add_cookie(&cookie, reply, ' ',
               xml_pref.get<std::string>("pref.auth.user_id"), ';');
    // Add to cookie user_hash from reply if any
    add_cookie(&cookie, reply, ' ',
               xml_pref.get<std::string>("pref.auth.user_hash"), ';');
    // Add to cookie PHPSID from reply if any
    add_cookie(&cookie, reply, ' ',
               xml_pref.get<std::string>("pref.auth.PHPSID"), ';');
}

// Send GET with cookie
// GET reply and parse it for unread books
// Repeat unil count(unread books) < num
std::string IUNB::get_unread()
{
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

    // This string will contain HTML-like list of unread books
    std::string result = "\xEF\xBB\xBF" // UTF-8 BOM
            "<html><head><meta charset charset=\"utf-8\"></head><body>";


    // Buffer for receiving from server
    std::string buf;

    // Start searching from this page
    size_t page_num = xml_pref.get<size_t>("pref.unread.start_page");
    // Desired number of unread books
    size_t num = xml_pref.get<size_t>("pref.unread.num");

    // Used in for(...) below
    size_t offs(0), prev_offs(0), size(0), beg_search(0);

    // Standart TCP socket
    boost::asio::ip::tcp::socket socket(io_service);
    // Connect to machine-like endpoint range
    boost::asio::connect(socket,resolver.resolve(*pquery));

    // count - unread books. num - desired.
    for (size_t count(0); count < num; )
    {
        std::cout << '\n' << page_num << " page processing, total books found: ";

        // Convert integer page_num to c-style string c_page_num,
        // and replace $pagenumber with it
        // Pos from above, if any
        if (pos!=get_req.npos)
        {
            _itoa_s(page_num++, c_page_num, 10);
            get_req.replace(pos, len, c_page_num);
            len=strlen(c_page_num);
        }

        // Send GET request with cookie data from above
        boost::asio::write(socket, boost::asio::buffer(get_req));

        // Get Reply and parse it
        while (size=wait_for_reply(&socket))
        {
            // Resize if not enough
            if (buf.size()-offs < size) buf.resize((buf.size()+size)*2);
            // Get reply and save to string with offset - offs
            boost::asio::read(socket,
                              boost::asio::buffer(&buf[offs], size));
            offs+=size;

            // 14 == strlen("data-rate=\"\"")
            parse_for_unread(&result, buf, beg_search, &count, num);
            beg_search=offs-14;

            // Have I received any?
            if (offs!=prev_offs) prev_offs=offs;
            else break;
        }// !while (size=wait_for_reply(&socket))
    }// !for (size_t count(0); count < num;)

    result+="</body></html>";
    return result;
}

// Add to *out_str sep and sequence from src,
// that begin with beg_req and end at end_req
void IUNB::add_cookie (std::string * out_str,
                       const std::string & src,
                       char sep,
                       const std::string & beg_req,
                       char end_req)
{

    auto beg_pos = src.find(beg_req);
    if (beg_pos!=src.npos)
    {
        auto end_pos = src.find(end_req, beg_pos);
        if (end_pos!=src.npos)
        {
            *out_str+=sep;
            out_str->append(src, beg_pos, ++end_pos-beg_pos);
        }
    }
}

// Parse string for unread books
void IUNB::parse_for_unread(std::string * out_str,
                            const std::string & src, size_t beg_search,
                            size_t * out_count, size_t num)
{
    auto unread_book_pos = src.find("data-rate=\"\"", beg_search);

    auto beg_pos = src.npos;
    auto end_pos = src.npos;

    while (unread_book_pos!=src.npos && *out_count < num)
    {
        beg_pos = src.rfind("<a href=\"", unread_book_pos);
        if (beg_pos!=src.npos)
        {
            end_pos = src.find("</a>", beg_pos);
            if (end_pos!=src.npos)
            {
                out_str->append(src, beg_pos, end_pos-beg_pos);
                *out_str+="</a><br>";

                std::cout << ++*out_count << ' ';
            }
        }
        unread_book_pos = src.find("data-rate=\"\"", ++unread_book_pos);
    }
}

// Waiting either 1 second or until socket buffer is full
size_t IUNB::wait_for_reply (const boost::asio::ip::tcp::socket * socket)
{
    boost::asio::ip::tcp::socket::receive_buffer_size buf_size;
    socket->get_option(buf_size);
    size_t size = socket->available();
    for (size_t wait_count(0), prev_size(0); wait_count<10; prev_size=size)
    {
        if (prev_size==size)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++wait_count;
        } else wait_count=0;
        size = socket->available();
        if (size==buf_size.value()) break;
    }
    return size;
}
