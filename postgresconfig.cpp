#include "stdafx.h"
#include <fmt/printf.h>
#include <iostream>
#include "postgresconfig.h"

using namespace std;

namespace mbr {
	template<typename It, typename T, typename M, typename X>
	inline It find(It first, It last, M T::*m, const X& v)
	{
		return std::find_if(first, last, [m, &v](const T& e) {
			return e.*m == v;
		});
	}
}

bool postgres_connection::open()
{
	bool ok = _db.open(_host.data(), _user.data(), _password.data(), _port, _database.data());
	if (!ok)
	{
		qtl::postgres::error e(_db.handle());
		cerr << e.what() << endl;
	}
	else
	{
		std::vector<Oid> classids;
		_db.query("select oid, typname, typtype, typcategory, typarray from pg_type", [this, &classids](Oid oid, const std::string& name, char type, char category, Oid arrayid) {
			if (type=='b')
			{
				_types.emplace_back(pgtype{ oid, name, type, arrayid });
			}
			else if (type == 'c')
			{
				_types.emplace_back(pgtype{ oid, name, type, arrayid });
				classids.push_back(oid);
			}
		});
		ostringstream oss;
		std::copy(classids.begin(), classids.end(), std::ostream_iterator<Oid>(oss, ","));
		string idlist = oss.str();
		std::map<Oid, Oid> class_map;
		idlist.pop_back();
		oss = ostringstream();
		_db.query(fmt::sprintf("select oid, reltype from pg_class where relkind = 'c' and reltype in (%s);", idlist), [&class_map, &oss](Oid oid, Oid type) {
			class_map.emplace(oid, type);
			oss << type << ',';
		});
		idlist = oss.str();
		idlist.pop_back();
		_db.query(fmt::sprintf("select attname, atttypid, attrelid from pg_attribute where attrelid in (select oid from pg_class where reltype in (%s));", idlist), [this, &class_map](const std::string& name, Oid typid, Oid classid) {
			Oid clsss_typid = class_map[classid];
			auto it = mbr::find(_classes.begin(), _classes.end(), &pgclass::_id, clsss_typid);
			if (it == _classes.end())
			{
				auto it_type = mbr::find(_types.begin(), _types.end(), &pgtype::_id, clsss_typid);
				if (it_type != _types.end())
				{
					pgclass the_class;
					the_class._id = clsss_typid;
					the_class._name = it_type->_name;
					it = _classes.insert(_classes.end(), std::move(the_class));
				}
			}
			if (it != _classes.end())
			{
				it->_fields.emplace_back(pgfield{ name, convert_type(typid, 0) });
			}
		});
	}
	return ok;
}

std::string postgres_connection::template_file() const
{
	return "postgres.h";
}

void postgres_connection::scan_tables(std::vector<std::unique_ptr<qtlstmt>>& queries)
{
	_db.query("select table_name, is_insertable_into from information_schema.tables where table_schema=$1", _schema, [this, &queries](const string& name, const std::string& insertable) {
		if (!find_table(queries, name)) {
			unique_ptr<qtlstmt> stmt(new postgres_stmt(*this));
			stmt->_type = qtlstmt::table;
			stmt->_text = name;
			stmt->_classname = name;
			stmt->as_table();
			stmt->_functions._insert = insertable == "YES";
			stmt->_functions._update = insertable == "YES";
			stmt->_functions._erase = insertable == "YES";
			queries.push_back(move(stmt));
		}
	});
}

std::string postgres_connection::escape_token(const std::string& text)
{
	return "\\\"" + text + "\\\"";
}

std::string postgres_connection::find_type(Oid typid) const
{
	auto it = mbr::find(_types.begin(), _types.end(), &pgtype::_id, typid);
	return (it != _types.end()) ? it->_name : std::string();
}

const pgtype* postgres_connection::find_type(const std::string& name) const
{
	auto it = mbr::find(_types.begin(), _types.end(), &pgtype::_name, name);
	return (it != _types.end()) ? &*it : nullptr;
}

const pgclass* postgres_connection::find_class(const std::string& name) const
{
	auto it = mbr::find(_classes.begin(), _classes.end(), &pgclass::_name, name);
	return (it != _classes.end()) ? &*it : nullptr;
}

static map<Oid, string> type_map = {
	{ BOOLOID, "bool" },
	{ INT4OID, "int32_t" },
	{ CHAROID, "char" },
	{ INT2OID, "int16_t" },
	{ INT8OID, "int64_t" },
	{ FLOAT4OID, "float" },
	{ FLOAT8OID , "double" },
	{ TEXTOID, "std::string" },
	{ TIMESTAMPOID, "qtl::postgres::timestamp" },
	{ INTERVALOID, "qtl::postgres::interval" },
	{ DATEOID, "qtl::postgres::date" },
	{ NUMERICOID, "qtl::postgres::numeric" },
	{ OIDOID, "qtl::postgres::large_object" },
	{ BYTEAOID, "std::vector<uint8_t>" },
	{ 1000, "std::vector<bool>" },
	{ 1002, "std::vector<char>" },
	{ INT2ARRAYOID, "std::vector<int16_t>" },
	{ INT4ARRAYOID, "std::vector<int32_t>" },
	{ 1016, "std::vector<int32_t>" },
	{ FLOAT4ARRAYOID, "std::vector<float>"},
	{ 1022, "std::vector<double>"},
};

std::string postgres_connection::convert_type(Oid typid, uint32_t length)
{
	string cpp_type = "std::string";
	bool is_array = false;
	auto it_type = mbr::find(_types.begin(), _types.end(), &pgtype::_id, typid);
	if (it_type==_types.end())
		return std::string();
	auto it = type_map.find(typid);
	if (it == type_map.end() && it_type->_arrayid == 0)
	{
		it_type = mbr::find(_types.begin(), _types.end(), &pgtype::_arrayid, it_type->_id);
		if (it_type == _types.end())
			return std::string();
		is_array = true;
		it = type_map.find(it_type->_id);
	}

	if (it != type_map.end())
	{
		if (is_array)
			cpp_type = "std::vector<" + it->second + ">";
		else
			cpp_type = it->second;
	}
	if (cpp_type == "std::string" && length > 0 && length <= 128)
		cpp_type = "char";
	return cpp_type;
}

void postgres_stmt::init_fields()
{
	if (_type == qtlstmt::table)
	{
		_connection._db.query(R"(select column_name, udt_name, character_octet_length, is_identity from information_schema.columns
			where table_schema = $1 and table_name = $2)",
			std::forward_as_tuple(_connection._schema, _text),
			[this](const string& name, const string& type, int32_t length, const std::string& is_identity) {
			qtlfield field;
			auto it = _fields.find(name);
			field._name = name;
			field._length = length + 1;
			if (it != _fields.end())
				field._type = it->second;
			else
				field._type = convert_type(type, length);
			if (is_identity=="YES")
				field._auto_increment = true;
			_fieldlist.push_back(move(field));
		});
		_connection._db.query(R"(select column_name from information_schema.key_column_usage 
			where table_schema=$1 and table_name=$2 and position_in_unique_constraint is null order by ordinal_position)",
			std::forward_as_tuple(_connection._schema, _text),
			[this](const string& column_name) {
			auto it = mbr::find(_fieldlist.begin(), _fieldlist.end(), &qtlfield::_name, column_name);
			if (it != _fieldlist.end())
				it->_primary = true;
		});
	}
	else
	{
		qtl::postgres::statement stmt = _connection._db.open_command(_text);
		stmt.execute();
		qtl::postgres::result& res = stmt.get_result();
		unsigned int count = res.get_column_count();
		for (unsigned int i = 0; i != count; i++)
		{
			Oid typid = res.get_column_type(i);
			std::string type_name = _connection.find_type(typid);
			if (type_name.empty())
			{
				return;
			}
			qtlfield field;
			field._name = res.get_column_name(i);
			field._length = res.get_column_length(i) + 1;
			field._type = convert_type(type_name, res.get_column_length(i));
			_fieldlist.push_back(move(field));
		}
	}
}

void postgres_stmt::init_primaries()
{
	if (find_if(_fieldlist.begin(), _fieldlist.end(), [](const qtlfield& field) {
		return field._primary;
	}) == _fieldlist.end())
	{
		_functions._get = false;
		_functions._update = false;
		_functions._erase = false;
	}
}

std::string postgres_stmt::convert_type(const std::string& type_name, uint32_t length)
{
	string cpp_type = "std::string";

	bool is_array = false;
	const pgtype* type = _connection.find_type(type_name);
	if (type == nullptr)
		return std::string();
	auto it = type_map.find(type->_id);
	if (it == type_map.end() && type->_arrayid == 0 && type_name[0] == '_')
	{
		type = _connection.find_type(type_name.substr(1));
		if (type == nullptr)
			return std::string();
		is_array = true;
		it = type_map.find(type->_id);
	}

	if (_connection.find_class(type_name))
	{
		if (is_array)
			cpp_type = "std::vector<" + type_name + ">";
		else
			cpp_type = type_name;
	}
	else if (it != type_map.end())
	{
		if (is_array)
			cpp_type = "std::vector<" + it->second + ">";
		else
			cpp_type = it->second;
	}
	if (cpp_type == "std::string" && length > 0 && length <= 128)
		cpp_type = "char";
	return cpp_type;
}


