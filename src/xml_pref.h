#pragma  once
#include <fstream>
#include <string>
#include <iterator>
namespace Brainheav
{
	class xml_pref
	{
	public:
		struct xml_err:std::exception
		{
			xml_err(const char * what):exception(what){}
		};
		class xml_res 
		{
		private:
			std::string & res;
			size_t ibeg;
			size_t iend;
			size_t ilen;
		public:
			bool err;
		public:
			xml_res(std::string & ref, const char * o_tag, const char * c_tag):res(ref), err(true)
			{
				ibeg=res.find(o_tag);
				if(ibeg==res.npos) return;

				ibeg+=strlen(o_tag);

				iend=res.find(c_tag, ibeg);
				if(ibeg==res.npos) return;

				ilen=iend-ibeg;
				err=false;
			}
			std::string get_string ()
			{
				if (err) throw xml_err(__FUNCTION__);
				return std::string(res, ibeg, ilen);
			}
			long long get_ll () // waiting for atoll from M$
			{
				if (err) throw xml_err(__FUNCTION__);
				return atol(res.c_str()+ibeg);
			}
		};// !class xml_res
	private:
		std::fstream file;
		std::string param;
	public:
		xml_pref(std::string filename):
			file(filename),
			param(std::istreambuf_iterator<char> (file), std::istreambuf_iterator<char>())
		{
			file.seekp(0);
		}
		xml_res get(const char * o_tag, const char * c_tag)
		{
			return xml_res(param, o_tag, c_tag);
		}
		~xml_pref()
		{
			file.write(param.c_str(), param.size());
		}
	};// !class xml_pref
}// !namespace Brainheav