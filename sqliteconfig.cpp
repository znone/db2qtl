#include "stdafx.h"
#include "sqliteconfig.h"
#include <fmt/printf.h>
#include <iostream>

using namespace std;

bool sqlite_connection::open()
{
	_db.open(_filename.data());
	return true;
}

std::string sqlite_connection::template_file() const
{
	return "sqlite.h";
}

void sqlite_connection::scan_tables(std::vector<std::unique_ptr<qtlstmt>>& queries)
{
	_db.query("SELECT name FROM sqlite_master", [this, &queries](const string& name) {
		if (!find_table(queries, name)) {
			unique_ptr<qtlstmt> stmt(new sqlite_stmt(*this));
			stmt->_type = qtlstmt::table;
			stmt->_text = name;
			stmt->_classname = name;
			stmt->as_table();
			queries.push_back(move(stmt));
		}
	});
}

std::string sqlite_connection::escape_token(const std::string& text)
{
	return "`" + text + "`";
}

void sqlite_stmt::init_fields()
{
	if (_type == qtlstmt::table)
	{
		_connection._db.query(fmt::sprintf("PRAGMA table_info(%s)", _text), [this](int cid, const std::string& name, const std::string& type, int notnull, int dfk_value, int pk) {
			const char* field_type;
			const char* sequence_name;
			int not_null;
			int primary_key;
			int autoinc;
			int error = sqlite3_table_column_metadata(_connection._db.handle(), NULL, _text.data(), name.data(), &field_type, &sequence_name, &not_null, &primary_key, &autoinc);
			qtlfield field;
			auto it = _fields.find(name);
			field._name = name;
			field._length = 0;
			if (it != _fields.end())
				field._type = it->second;
			else
				field._type = convert_type(field_type);
			if (primary_key)
				field._primary = true;
			if (autoinc)
				field._auto_increment = true;
			_fieldlist.push_back(move(field));
		});
	}
	else
	{
		qtl::sqlite::statement stmt=_connection._db.open_command(_text);
		int v_i=0;
		int64_t v_i64 = 0;
		double v_d = 0;
		string v_str;
		qtl::const_blob_data v_blob;

		int count = stmt.get_parameter_count();
		if (_params.size() != count)
		{
			throw std::logic_error(fmt::sprintf("need %lu parameters, but got %lu.", count, _params.size()));
		}
		for (int i = 0; i != count; i++)
		{
			for (size_t i = 0; i != _params.size(); i++)
			{
				if (_params[i].second == "int")
					qtl::bind_param(stmt, i, v_i);
				else if (_params[i].second == "int64_t")
					qtl::bind_param(stmt, i, v_i64);
				else if (_params[i].second == "double")
					qtl::bind_param(stmt, i, v_d);
				else if (_params[i].second == "string")
					qtl::bind_param(stmt, i, v_str);
				else if (_params[i].second == "qtl::const_blob_data")
					qtl::bind_param(stmt, i, v_blob);
				else
					throw std::logic_error(fmt::sprintf("unknown parameter type: %s", _params[i].second));
			}
		}
		stmt.fetch();
		count = stmt.get_column_count();
		for (int i = 0; i != count; i++)
		{
			qtlfield field;
			field._name = stmt.get_column_name(i);
			field._length = 0;
			auto it = _fields.find(field._name);
			if (it != _fields.end())
				field._type = it->second;
			else
				field._type = convert_type(stmt.get_column_type(i));
			_fieldlist.push_back(move(field));
		}
	}
}

void sqlite_stmt::init_primaries()
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

std::string sqlite_stmt::convert_type(const char* data_type)
{
	string cpp_type = "std::string";
	static map<string, string> type_map = {
		{ "INTEGER", "int64_t" },
		{ "REAL", "float" },
		{ "TEXT", "std::string" },
		{ "BLOB", "qtl::const_blob_data" }
	};
	if (data_type)
	{
		auto it = type_map.find(data_type);
		if (it != type_map.end()) cpp_type = it->second;
	}
	return cpp_type;
}

std::string sqlite_stmt::convert_type(int field)
{
	string cpp_type="std::string";
	static map<int, string> type_map = {
		{ SQLITE_INTEGER, "int64_t" },
		{ SQLITE_FLOAT, "double" },
		{ SQLITE_TEXT, "std::string" },
		{ SQLITE_BLOB, "qtl::const_blob_data" }
	};
	auto it = type_map.find(field);
	if (it != type_map.end()) cpp_type = it->second;
	return cpp_type;
}

