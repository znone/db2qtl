#ifndef _POSTGRESCONFIG_H_
#define _POSTGRESCONFIG_H_

#include "qtlconfig.h"
#include <qtl_postgres.hpp>

struct pgtype
{
	Oid _id;
	std::string _name;
	char category;
	Oid _arrayid;
};
struct pgfield
{
	std::string _name;
	std::string _type;
};
struct pgclass
{
	Oid _id;
	std::string _name;
	std::vector<pgfield> _fields;
};

struct postgres_connection : public connection_config
{
	std::string _host;
	uint16_t _port;
	std::string _user;
	std::string _password;
	std::string _schema;

	qtl::postgres::database _db;

	postgres_connection() : _port(5432) { }

	virtual bool open() override;
	std::string template_file() const override;
	virtual void scan_tables(std::vector<std::unique_ptr<qtlstmt>>& queries) override;
	virtual std::string escape_token(const std::string& text) override;

	std::string find_type(Oid typid) const;
	const pgtype* find_type(const std::string& name) const;
	std::string convert_type(Oid typid, uint32_t length);
	const pgclass* find_class(const std::string& name) const;

public:
	std::vector<pgtype> _types;
	std::vector<pgclass> _classes;
};

struct postgres_stmt : public qtlstmt
{
	explicit postgres_stmt(postgres_connection& connection) : _connection(connection) { }

	virtual void init_fields() override;
	virtual void init_primaries() override;

private:
	postgres_connection& _connection;

	std::string convert_type(const std::string& type_name, uint32_t length);
};

#endif //_POSTGRESCONFIG_H_
