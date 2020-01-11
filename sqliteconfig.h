#ifndef _SQLITECONFIG_H_
#define _SQLITECONFIG_H_

#include "qtlconfig.h"
#include <qtl_sqlite.hpp>

struct sqlite_connection : public connection_config
{
	std::string _filename;

	qtl::sqlite::database _db;

	sqlite_connection() { }

	virtual bool open() override;
	std::string template_file() const override;
	virtual void scan_tables(std::vector<std::unique_ptr<qtlstmt>>& queries) override;
	virtual std::string escape_token(const std::string& text) override;
};

struct sqlite_stmt : public qtlstmt
{
	explicit sqlite_stmt(sqlite_connection& connection) : _connection(connection) { }

	virtual void init_fields() override;
	virtual void init_primaries() override;

private:
	sqlite_connection& _connection;

	static std::string convert_type(const char* data_type);
	static std::string convert_type(int field);
};

#endif //_SQLITECONFIG_H_
