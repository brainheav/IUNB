#ifndef IUNB_H
#define IUNB_H

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/asio.hpp>
#include <string>
#include <memory>

class IUNB
{
private:
    // Needed for BOOST IO operations
    boost::asio::io_service io_service;

    // Saving pref from XML
    boost::property_tree::ptree     xml_pref;
    std::string                     xml_filename;

    // Save auth across function call
    std::string cookie;

    // Unique ptr only because query doesn't have default ctor
    // Query store something like www.example.com:80
    std::unique_ptr<boost::asio::ip::tcp::resolver::query> pquery;
    // Machine-like endpoint from Query
    boost::asio::ip::tcp::resolver resolver;

private:
    // Waiting either 1 second or until socket buffer is full
    static size_t wait_for_reply (const boost::asio::ip::tcp::socket * socket);
    // Add to *out_str sep and sequence from src,
    // that begin with beg_req and end at end_req
    static void IUNB::add_cookie(std::string * out_str,
                                 const std::string & src,
                                 char sep,
                                 const std::string & beg_req,
                                 char end_req);
    // Parse string for unread books
    static void parse_for_unread(std::string * out_str,
                                 const std::string & src, size_t beg_search,
                                 size_t * out_count, size_t num);
public:
    // Connect, send auth inf, save parsed reply to cookie to be auth.
    IUNB(std::string login, std::string password, std::string in_xml_filename);
    // Send GET with cookie
    // GET reply and parse it for unread books
    // Repeat unil count(unread books) < num
    std::string get_unread();
};// !class IUNB

#endif // IUNB_H
