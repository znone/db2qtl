#include "stdafx.h"
#include "mysqlconfig.h"
#include <fmt/printf.h>
#include <iostream>

using namespace std;

bool mysql_connection::open()
{
	bool ok = _db.open(_host.data(), _user.data(), _password.data(), _database.data(), 0, _port);
	if (!ok)
	{
		qtl::mysql::error e(_db);
		cerr << e.what() << endl;
	}
	else
	{
		_db.charset_name("utf8");
	}
	return ok;
}

std::string mysql_connection::template_file() const
{
	return "mysql.h";
}

void mysql_connection::scan_tables(std::vector<std::unique_ptr<qtlstmt>>& queries)
{
	_db.query("show tables", [this, &queries](const string& name) {
		if (!find_table(queries, name)) {
			unique_ptr<qtlstmt> stmt(new mysql_stmt(*this));
			stmt->_type = qtlstmt::table;
			stmt->_text = name;
			stmt->_classname = name;
			stmt->as_table();
			queries.push_back(move(stmt));
		}
	});
}

std::string mysql_connection::escape_token(const std::string& text)
{
	return '`' + text + '`';
}

void mysql_stmt::init_fields()
{
	if (_type == qtlstmt::table)
	{
		string sql = fmt::sprintf(R"(select column_name, data_type, column_type, column_key, character_octet_length, extra from information_schema.columns
			where table_schema = '%s' and TABLE_NAME = '%s')", _connection._database, _text);
		_connection._db.query(sql,
			[this](const string& name, const string& type, const string& column, const string& key, uint32_t length, const string& extra) {
			qtlfield field;
			auto it = _fields.find(name);
			field._name = name;
			field._length = length+1;
			if (it != _fields.end())
				field._type = it->second;
			else
				field._type = convert_type(type, length, column.find("unsigned") !=string::npos);
			if (key == "PRI")
				field._primary=true;
			if (extra == "auto_increment")
				field._auto_increment = true;
			_fieldlist.push_back(move(field));
		});
	}
	else
	{
		qtl::mysql::statement stmt=_connection._db.open_command(_text);
		auto result_deleter = [](MYSQL_RES* result) {
			mysql_free_result(result);
		};

		bool v_bool=false;
		int8_t v_i8 = 0;
		int8_t v_u8 = 0;
		int16_t v_i16 = 0;
		int16_t v_u16 = 0;
		int32_t v_i32 = 0;
		int32_t v_u32 = 0;
		int64_t v_i64 = 0;
		int64_t v_u64 = 0;
		float v_f = 0;
		double v_d = 0;
		string v_str;
		qtl::mysql::time v_time;

		stmt.execute_custom([&](qtl::mysql::statement& stmt) {
			size_t count = stmt.get_parameter_count();
			if (_params.size() != count)
			{
				throw std::logic_error(fmt::sprintf("need %lu parameters, but got %lu.", count, _params.size()));
			}
			if (count > 0)
			{
				for (size_t i = 0; i != _params.size(); i++)
				{
					if (_params[i].second == "bool")
						qtl::bind_param(stmt, i, v_bool);
					else if (_params[i].second == "int8_t")
						qtl::bind_param(stmt, i, v_i8);
					else if (_params[i].second == "uint8_t")
						qtl::bind_param(stmt, i, v_u8);
					else if (_params[i].second == "int16_t")
						qtl::bind_param(stmt, i, v_i16);
					else if (_params[i].second == "uint16_t")
						qtl::bind_param(stmt, i, v_u16);
					else if (_params[i].second == "int32_t")
						qtl::bind_param(stmt, i, v_i32);
					else if (_params[i].second == "uint32_t")
						qtl::bind_param(stmt, i, v_u32);
					else if (_params[i].second == "int64_t")
						qtl::bind_param(stmt, i, v_i64);
					else if (_params[i].second == "int64_t")
						qtl::bind_param(stmt, i, v_u64);
					else if (_params[i].second == "float")
						qtl::bind_param(stmt, i, v_f);
					else if (_params[i].second == "double")
						qtl::bind_param(stmt, i, v_d);
					else if (_params[i].second == "string")
						qtl::bind_param(stmt, i, v_str);
					else if (_params[i].second == "time")
						qtl::bind_param(stmt, i, v_time);
					else
						throw std::logic_error(fmt::sprintf("unknown parameter type: %s", _params[i].second));
				}
			}
		});
		
		unique_ptr<MYSQL_RES, decltype(result_deleter)> result(mysql_stmt_result_metadata(stmt), result_deleter);
		for (unsigned int i = 0; i != result->field_count; i++)
		{
			qtlfield field;
			auto it = _fields.find(result->fields[i].name);
			field._name = result->fields[i].name;
			field._length = result->fields[i].length+1;
			if (it != _fields.end())
				field._type = it->second;
			else
				field._type = convert_type(result->fields[i]);
			_fieldlist.push_back(move(field));
		}
	}
}

void mysql_stmt::init_primaries()
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

std::string mysql_stmt::convert_type(const std::string& data_type, uint32_t length, bool is_unsigned)
{
	string cpp_type = "std::string";
	static map<string, string> type_map = {
		{ "int", "int32_t" },
		{ "tinyint", "int8_t" },
		{ "smallint", "int16_t" },
		{ "bigint", "int64_t" },
		{ "float", "float" },
		{ "double", "double" },
		{ "time", "qtl::mysql::time" },
		{ "date", "qtl::mysql::time" },
		{ "datetime", "qtl::mysql::time" },
		{ "timestamp", "qtl::mysql::time" }
	};
	auto it = type_map.find(data_type);
	if(it!=type_map.end()) cpp_type = it->second;
	if (is_unsigned && cpp_type.compare(0, 3, "int", 3) == 0)
		cpp_type.insert(0, "u");
	if (cpp_type == "std::string" && length>0 && length <= 128)
		cpp_type = "char";
	return cpp_type;
}

std::string mysql_stmt::convert_type(MYSQL_FIELD& field)
{
	string cpp_type="std::string";
	static map<enum_field_types, string> type_map = {
		{ MYSQL_TYPE_BIT, "bool" },
		{ MYSQL_TYPE_TINY, "int8_t" },
		{ MYSQL_TYPE_SHORT, "int16_t" },
		{ MYSQL_TYPE_LONG, "int32_t" },
		{ MYSQL_TYPE_LONGLONG, "int64_t" },
		{ MYSQL_TYPE_DATE, "qtl::mysql::time" },
		{ MYSQL_TYPE_TIME, "qtl::mysql::time" },
		{ MYSQL_TYPE_DATETIME, "qtl::mysql::time" },
		{ MYSQL_TYPE_TIME2, "qtl::mysql::time" },
		{ MYSQL_TYPE_DATETIME2, "qtl::mysql::time" },
		{ MYSQL_TYPE_TIMESTAMP, "qtl::mysql::time" },
		{ MYSQL_TYPE_TIMESTAMP2, "qtl::mysql::time" }
	};
	auto it = type_map.find(field.type);
	if (it != type_map.end()) cpp_type = it->second;
	if (cpp_type == "std::string" && field.length>0 && field.length <= 128)
		cpp_type = "char";
	return cpp_type;
}

