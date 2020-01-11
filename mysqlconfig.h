#ifndef _MYSQLCONFIG_H_
#define _MYSQLCONFIG_H_

#include "qtlconfig.h"
#include <qtl_mysql.hpp>

struct mysql_connection : public connection_config
{
	std::string _host;
	uint16_t _port;
	std::string _user;
	std::string _password;

	qtl::mysql::database _db;

	mysql_connection() : _port(3306) { }

	virtual bool open() override;
	std::string template_file() const override;
	virtual void scan_tables(std::vector<std::unique_ptr<qtlstmt>>& queries) override;
	virtual std::string escape_token(const std::string& text) override;
};

struct mysql_stmt : public qtlstmt
{
	explicit mysql_stmt(mysql_connection& connection) : _connection(connection) { }

	virtual void init_fields() override;
	virtual void init_primaries() override;

private:
	mysql_connection& _connection;

	static std::string convert_type(const std::string& data_type, uint32_t length, bool is_unsigned);
	static std::string convert_type(MYSQL_FIELD& field);
};

#endif //_MYSQLCONFIG_H_
