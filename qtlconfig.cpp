#include "stdafx.h"
#include "qtlconfig.h"
#include "mysqlconfig.h"
#include "sqliteconfig.h"

namespace leech
{
	template<typename Document>
	void get(const Document& ar, const typename Document::element_type& element, std::unique_ptr<connection_config>& v, const char* /*name*/ = nullptr);
}

#include <leech/model.hpp>
#include <leech/yaml.hpp>
#include <functional>
#include <iostream>
#include <fmt/printf.h>

#ifdef STRUCT_MODEL_FIELD_PREFIX
#undef STRUCT_MODEL_FIELD_PREFIX
#endif //STRUCT_MODEL_FIELD_PREFIX
#define STRUCT_MODEL_FIELD_PREFIX _

STRUCT_MODEL(connection_config, database)
STRUCT_MODEL(qtlstmt, classname, params, fields, functions)
STRUCT_MODEL(qtlfunctions, for_each, get, insert, update, erase)
STRUCT_MODEL(qtlconfig, connection, filename, namespace, all_tables, generate_pool, query_list)
STRUCT_MODEL_INHERIT(mysql_connection, (connection_config), host, port, user, password)
STRUCT_MODEL_INHERIT(sqlite_connection, (connection_config), filename)

using namespace std;

std::function<qtlstmt*()> create_query;

namespace leech
{

template<typename Document>
void get(const Document& ar, const typename Document::element_type& element, std::unique_ptr<connection_config>& v, const char* /*name*/)
{
	std::string type;
	ar.get(element["type"], type);
	v = nullptr;
	if (type == "mysql")
	{
		mysql_connection* connection = new mysql_connection;
		leech::get(leech::yaml::document(element), *connection);
		v.reset(connection);
		create_query = [connection=v.get()]() { 
			return new mysql_stmt(dynamic_cast<mysql_connection&>(*connection));
		};
	}
	else if (type == "sqlite")
	{
		sqlite_connection* connection = new sqlite_connection;
		leech::get(leech::yaml::document(element), *connection);
		v.reset(connection);
		create_query = [connection = v.get()]() {
			return new sqlite_stmt(dynamic_cast<sqlite_connection&>(*connection));
		};
	}
}

}

namespace YAML
{

template <>
struct as_if<std::unique_ptr<qtlstmt>, void>
{
	explicit as_if(const Node& node_) : node(node_) {}
	const Node& node;

	std::unique_ptr<qtlstmt> operator()() const
	{
		std::unique_ptr<qtlstmt> query = nullptr;
		if (node.Type() != NodeType::Map)
			throw TypedBadConversion<std::string>(node.Mark());

		query.reset(create_query());
		std::string type;
		if (node["table"].IsDefined())
		{
			type = "table";
			query->as_table();
		}
		else if (node["query"].IsDefined())
		{
			type = "query";
			query->as_query();
		}
		leech::get(leech::yaml::document(node[type]), query->_text);
		leech::get(leech::yaml::document(node), *query);
		return query;
	}
};

template <>
struct as_if<params_type, void>
{
	explicit as_if(const Node& node_) : node(node_) {}
	const Node& node;

	params_type operator()() const
	{
		params_type v;
		if (node.Type() != NodeType::Map)
			throw TypedBadConversion<std::string>(node.Mark());

		for (auto i : node)
		{
			v.emplace_back(i.first.as<string>(), i.second.as<string>());
		}
		return v;
	}
};

template <>
struct as_if<fields_type, void>
{
	explicit as_if(const Node& node_) : node(node_) {}
	const Node& node;

	fields_type operator()() const
	{
		fields_type v;
		if (node.Type() != NodeType::Map)
			throw TypedBadConversion<std::string>(node.Mark());

		for (auto i : node)
		{
			v.emplace(i.first.as<string>(), i.second.as<string>());
		}
		return v;
	}
};

}

bool connection_config::find_table(const std::vector<std::unique_ptr<qtlstmt>>& queries, const std::string& name)
{
	for (const auto& query : queries)
	{
		if (query->_type == qtlstmt::table && query->_text == name)
			return true;
	}
	return false;
}


void qtlconfig::load(const char* filename)
{
	STRUCT_MODEL_SET_OPTIONAL(qtlconfig, all_tables);
	STRUCT_MODEL_SET_OPTIONAL(qtlconfig, generate_pool);
	STRUCT_MODEL_SET_OPTIONAL(qtlstmt, params);
	STRUCT_MODEL_SET_OPTIONAL(qtlstmt, fields);
	STRUCT_MODEL_SET_OPTIONAL(qtlfunctions, for_each);
	STRUCT_MODEL_SET_OPTIONAL(qtlfunctions, get);
	STRUCT_MODEL_SET_OPTIONAL(qtlfunctions, insert);
	STRUCT_MODEL_SET_OPTIONAL(qtlfunctions, update);
	STRUCT_MODEL_SET_OPTIONAL(qtlfunctions, erase);
	STRUCT_MODEL_SET_OPTIONAL(mysql_connection, port);

	leech::yaml::document doc = leech::yaml::load_file(filename);
	leech::get(doc, *this);
}

bool qtlconfig::prepare()
{
	assert(_connection);
	if (!_connection->open()) {
		cerr << "Can't connect to database: " << _connection->_database << endl;
		return false;
	}
	if (_all_tables)
	{
		_connection->scan_tables(_query_list);
	}
	for (auto it= _query_list.begin(); it!=_query_list.end();)
	{
		auto& query = *it;
		query->init_fields();
		query->init_primaries();
		if (query->_fieldlist.empty())
			it = _query_list.erase(it);
		else
			++it;

		for (auto& field : query->_fieldlist)
		{
			if (field._type == "array" && field._length > 0)
			{
				field._type = fmt::sprintf("std::array<char, %d>", field._length);
			}
			else if (field._type == "char")
				field._is_carray = true;
		}
	}
	return true;
}

void qtlstmt::as_table()
{
	_type = qtlstmt::table;
	_functions._for_each = true;
	_functions._get = true;
	_functions._insert = true;
	_functions._update = true;
	_functions._erase = true;
}

void qtlstmt::as_query()
{
	_type = qtlstmt::query;
	_functions._for_each = true;
	_functions._get = false;
	_functions._insert = false;
	_functions._update = false;
	_functions._erase = false;
}
