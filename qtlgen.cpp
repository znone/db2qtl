#include "stdafx.h"
#include <boost/filesystem.hpp>
#include "qtlgen.h"
#include "mysqlconfig.h"
#include "sqliteconfig.h"
#include <leech/model.hpp>
#include <leech/json.hpp>
#include <inja/inja.hpp>

using namespace std;

#ifdef STRUCT_MODEL_FIELD_PREFIX
#undef STRUCT_MODEL_FIELD_PREFIX
#endif //STRUCT_MODEL_FIELD_PREFIX
#define STRUCT_MODEL_FIELD_PREFIX _

STRUCT_MODEL(connection_config, database)
STRUCT_MODEL(qtlstmt, type, text, classname, fieldlist, functions, params)
STRUCT_MODEL(qtlfunctions, for_each, get, insert, update, erase)
STRUCT_MODEL(qtlfield, name, type, primary, length, is_carray, auto_increment)
STRUCT_MODEL(qtlconfig, connection, filename, namespace, generate_pool, query_list)
STRUCT_MODEL_INHERIT(mysql_connection, (connection_config), host, port, user, password)
STRUCT_MODEL_INHERIT(sqlite_connection, (connection_config), filename)

namespace leech
{
}

namespace nlohmann {
	template <>
	struct adl_serializer<std::unique_ptr<connection_config>> {
		static void to_json(json& j, const std::unique_ptr<connection_config>& config) {
			if (config == nullptr) {
				j = nullptr;
			}
			else {
				if (dynamic_cast<mysql_connection*>(config.get())) {
					j=move(leech::put(leech::json::document(), dynamic_cast<mysql_connection&>(*config)).root());
					j["type"] = "mysql";
				}
				else if (dynamic_cast<sqlite_connection*>(config.get())) {
					j = move(leech::put(leech::json::document(), dynamic_cast<sqlite_connection&>(*config)).root());
					j["type"] = "sqlite";
				}
			}
		}
	};

	template <>
	struct adl_serializer<std::unique_ptr<qtlstmt>> {
		static void to_json(json& j, const std::unique_ptr<qtlstmt>& stmt) {
			if (stmt == nullptr) {
				j = nullptr;
			}
			else {
				j=move(leech::put(leech::json::document(), *stmt).root());
			}
		}
	};

	template <>
	struct adl_serializer<qtlfield> {
		static void to_json(json& j, const qtlfield& field) {
			j = move(leech::put(leech::json::document(), field).root());
		}
	};

	template <>
	struct adl_serializer<params_type::value_type> {
		static void to_json(json& j, const params_type::value_type& param) {
			j["name"] = param.first;
			j["type"] = param.second;
		}
	};
}

void generate(const qtlconfig& config, const std::string& template_path, const std::string& output_path)
{
	leech::json::document doc;
	leech::put(doc, config);

	nlohmann::json::value_type& data=doc.root();
	string str = data.dump(4);

	boost::filesystem::path output_file;
	output_file = data["filename"].get<string>();
	output_file.replace_extension(".h");

	inja::Environment env(template_path, output_path);
	env.set_trim_blocks(true);
	env.set_lstrip_blocks(true);
	env.add_callback("primary", 1, [](inja::Arguments& args) {
		const nlohmann::json* fieldlist = args.at(0);
		nlohmann::json result;
		for (const nlohmann::json& field : *fieldlist) {
			if (field["primary"].get<bool>())
				result.push_back(field);
		}
		return result;
	});
	env.add_callback("not_primary", 1, [](inja::Arguments& args) {
		const nlohmann::json* fieldlist = args.at(0);
		nlohmann::json result;
		for (const nlohmann::json& field : *fieldlist) {
			if (!field["primary"].get<bool>())
				result.push_back(field);
		}
		return result;
	});
	env.add_callback("not_id", 1, [](inja::Arguments& args) {
		const nlohmann::json* fieldlist = args.at(0);
		nlohmann::json result;
		for (const nlohmann::json& field : *fieldlist) {
			if (!field["auto_increment"].get<bool>())
				result.push_back(field);
		}
		return result;
	});
	env.add_callback("id_name", 1, [](inja::Arguments& args) {
		const nlohmann::json* fieldlist = args.at(0);
		nlohmann::json result;
		for (const nlohmann::json& field : *fieldlist) {
			if (field["auto_increment"].get<bool>())
				return field["name"];
		}
		return result;
	});
	env.add_callback("escape_token", 1, [&config](inja::Arguments& args) {
		const nlohmann::json* j = args.at(0);
		nlohmann::json result = config._connection->escape_token(j->get<string>());
		return result;
	});
	env.write(config._connection->template_file(), data, output_file.string());
}
