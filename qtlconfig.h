#ifndef _DBCONFIG_H_
#define _DBCONFIG_H_

#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>

struct qtlstmt;

struct connection_config
{
	virtual ~connection_config() { }
	virtual bool open()=0;
	virtual std::string template_file() const =0;
	virtual void scan_tables(std::vector<std::unique_ptr<qtlstmt>>& queries)=0;
	virtual std::string escape_token(const std::string& text) =0;

	std::string _database;

protected:
	static bool find_table(const std::vector<std::unique_ptr<qtlstmt>>& queries, const std::string& name);
};

struct qtlfield
{
	std::string _name, _type;
	int32_t _length { 0 };
	bool _is_carray { false };
	bool _primary {false };
	bool _auto_increment { false };
};

struct qtlfunctions
{
	bool _for_each;
	bool _get;
	bool _insert;
	bool _update;
	bool _erase;
};

struct case_less
{
	bool operator()(const std::string& lhs, const std::string& rhs) const
	{
		return boost::algorithm::ilexicographical_compare(lhs, rhs);
	}
};

typedef std::map<std::string, std::string, case_less> fields_type;
typedef std::vector<std::pair<std::string, std::string>> params_type;

struct qtlstmt
{
	enum { table, query } _type;
	std::string _text;
	std::string _classname;
	fields_type _fields;

	qtlfunctions _functions;

	params_type _params;
	std::vector<qtlfield> _fieldlist;

	qtlstmt() = default;
	virtual ~qtlstmt() { }
	void as_table();
	void as_query();
	virtual void init_fields() = 0;
	virtual void init_primaries() = 0;
};

struct qtlconfig
{
	std::unique_ptr<connection_config> _connection;
	std::string _filename;
	std::string _namespace;
	bool _all_tables { true };
	bool _generate_pool { false };
	std::vector<std::unique_ptr<qtlstmt>> _query_list;

	qtlconfig() = default;
	~qtlconfig() = default;
	void load(const char* filename);
	bool prepare();
};


#endif //_DBCONFIG_H_
