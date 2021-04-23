#ifndef _{{upper(filename)}}_H_
#define _{{upper(filename)}}_H_

#include <qtl_{{connection.type}}.hpp>
{% if generate_pool %}
#include <qtl_{{connection.type}}_pool.hpp>
#include <leech/model.hpp>
{% endif %}

namespace {{namespace}}
{
{% if generate_pool %}
class database_pool : public qtl::{{connection.type}}::database_pool
{
public:
	database_pool()
	{
		m_host = "{{connection.host}}";
		m_port = {{connection.port}};
		m_database = "{{connection.database}}";
#if !defined(NDEBUG)
		m_user = "{{connection.user}}";
		m_password = "{{connection.password}}";
#endif 
		m_schema = "{{connection.schema}}";
	}
	
	template<typename Document>
	void load_config(const Document& config)
	{
		leech::get(config, "host", m_host);
		leech::get(config, "port", m_port);
		leech::get(config, "database", m_database);
		leech::get(config, "user", m_user);
		leech::get(config, "password", m_password);
		leech::get(config, "schema", m_schema);
	}
	
	static database_pool& instance()
	{
		static database_pool pool;
		return pool;
	}
	
protected:
	std::string m_schema;
};
{% endif %}

{% for type in connection.classes %}
struct {{type.name}}
{
{% for field in type.fields %}
	{{field.type}} {{field.name}};
{% endfor%}
};
const Oid {{upper(type.name)}}OID = {{type.id}};
{% endfor %}

{% for query in query_list %}
class {{query.classname}}
{
public:
	struct record
	{
		{% for field in query.fieldlist %}
		{{field.type}} {{field.name}}{%if field.is_carray%}[{{field.length}}]{%endif%};
		{% endfor %}
	};
	
	{% if query.functions.for_each %}
	// F as
	//	 void f(const {{query.classname}}::record& rec);
	template<typename F>
	static void for_each(qtl::{{connection.type}}::database& db, {%if length(query.params) %}{% for param in query.params%}const {{param.type}}& {{param.name}}, {%endfor%}{% endif %}F&& f);
	{% if query.type == 0 %}
	template<typename F>
	static void query(qtl::{{connection.type}}::database& db, const char* additional, F&& f);
	{% endif %}
	{% endif %}	
	{% if query.functions.get %}
	static std::pair<record, bool> get(qtl::{{connection.type}}::database& db, {% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}const {{field.type}}& {{field.name}} {%endfor%});
	{% endif %}
	{% if query.functions.insert %}
	static bool insert(qtl::{{connection.type}}::database& db, record& rec);
	static bool put(qtl::{{connection.type}}::database& db, record& rec);
	{% endif %}
	{% if query.functions.erase %}
	static bool clear(qtl::{{connection.type}}::database& db);
	static bool erase(qtl::{{connection.type}}::database& db, {% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}const {{field.type}}& {{field.name}} {%endfor%});
	static bool erase_if(qtl::{{connection.type}}::database& db, const char* condition);
	{% endif %}
	{% if query.functions.update %}
	static bool update(qtl::{{connection.type}}::database& db, const record& rec);
	{% endif %}	
};

{% endfor %}
}

/* IMPLEMENT */

namespace qtl
{

{% for query in query_list %}
template<>
inline void bind_record<qtl::{{connection.type}}::statement, {{namespace}}::{{query.classname}}::record>(qtl::{{connection.type}}::statement& command, {{namespace}}::{{query.classname}}::record&& record)
{	
	qtl::bind_fields(command {% for field in query.fieldlist %}, record.{{field.name}}{% endfor %});	
}

{% endfor %}
}

namespace qtl
{
	
namespace postgres
{

{% for type in connection.classes %}
template<>
struct object_traits<{{namespace}}::{{type.name}}> : public base_object_traits<{{namespace}}::{{type.name}}, {{namespace}}::{{upper(type.name)}}OID>
{
	typedef typename base_object_traits<{{namespace}}::{{type.name}}, {{namespace}}::{{upper(type.name)}}OID>::value_type value_type;
	static const char* get(value_type& result, const char* data, const char* end)
	{
		auto t = std::tie({% for field in type.fields %}{%if not loop.is_first%}, {%endif%}result.{{field.name}}{% endfor %});
		return tuple_traits<decltype(t), type_id>::get(t, data, end);
	}
	static std::pair<const char*, size_t> data(const value_type& v, std::vector<char>& buffer)
	{
		auto t = std::make_tuple({% for field in type.fields %}{%if not loop.is_first%}, {%endif%}v.{{field.name}}{% endfor %});
		return tuple_traits<decltype(t), type_id>::data(t, buffer);
	}
};
{% endfor %}

}
	
}

namespace {{namespace}}
{

{% for query in query_list %}
// {{query.classname}}
{% if query.functions.for_each %}
template<typename F>
inline void {{query.classname}}::for_each(qtl::{{connection.type}}::database& db, {%if length(query.params) %}{% for param in query.params%}const {{param.type}}& {{param.name}}, {%endfor%}{% endif %}F&& f)
{
	{% if query.type == 0 %}
	db.query("select {% for field in query.fieldlist %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}} {%endfor%} from {{escape_token(query.text)}}", std::forward<F>(f));
	{% else %}
	db.query("{{query.text}}", {%if length(query.params) %}std::forward_as_tuple({% for param in query.params%}{%if not loop.is_first%}, {%endif%}{{param.name}}{%endfor%}), {% endif %}std::forward<F>(f));
	{% endif %}
}
{% if query.type == 0 %}
template<typename F>
inline void {{query.classname}}::query(qtl::{{connection.type}}::database& db, const char* additional, F&& f)
{
	std::string sql="select {% for field in query.fieldlist %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}} {%endfor%} from {{escape_token(query.text)}}";
	if(additional) sql+=additional;
	db.query(sql, std::forward<F>(f));
}
{% endif %}
{% endif %}
{% if query.functions.get %}
inline std::pair<{{query.classname}}::record, bool> {{query.classname}}::get(qtl::{{connection.type}}::database& db, {% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}const {{field.type}}& {{field.name}} {%endfor%})
{
	{{query.classname}}::record rec;
	bool ok = db.query_first("SELECT {% for field in query.fieldlist %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}} {%endfor%} FROM {{escape_token(query.text)}} WHERE {% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}}=${{loop.index1}} {%endfor%}", std::forward_as_tuple({% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{field.name}} {%endfor%}), std::forward<{{query.classname}}::record>(rec));
	return std::make_pair(std::move(rec), ok);
}
{% endif %}
{% if query.functions.insert %}
inline bool {{query.classname}}::insert(qtl::{{connection.type}}::database& db, record& rec)
{
{% if id_name(query.fieldlist)!=null %}	
	rec.{{id_name(query.fieldlist)}}=0;
	bool ok = db.query_first("INSERT INTO {{escape_token(query.text)}} ({% for field in not_id(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}} {%endfor%}) VALUES ({% for field in not_id(query.fieldlist) %} {%if not loop.is_first%},{%endif%}${{loop.index1}} {%endfor%}) returning {{escape_token(id_name(query.fieldlist))}}", 
		std::forward_as_tuple({% for field in not_id(query.fieldlist) %}{%if not loop.is_first%}, {%endif%}rec.{{field.name}} {%endfor%}), rec.{{id_name(query.fieldlist)}});
	return ok;
{% else %}
	uint64_t affected = 0;
	db.execute_direct("INSERT INTO {{escape_token(query.text)}} ({% for field in not_id(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}} {%endfor%}) VALUES ({% for field in not_id(query.fieldlist) %} {%if not loop.is_first%},{%endif%}${{loop.index1}} {%endfor%})", &affected,
		{% for field in not_id(query.fieldlist) %}{%if not loop.is_first%}, {%endif%}rec.{{field.name}} {%endfor%});
	if(affected>0)
	{
		{% if id_name(query.fieldlist)!=null %}
		rec.{{id_name(query.fieldlist)}}=db.insert_id(); 
		{% endif %}
		return true;
	}
	return false;
{% endif %}
}
inline bool {{query.classname}}::put(qtl::{{connection.type}}::database& db, record& rec)
{
{% if id_name(query.fieldlist)!=null %}	
	rec.{{id_name(query.fieldlist)}}=0;
	bool ok = db.query_first("INSERT INTO {{escape_token(query.text)}} ({% for field in not_id(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}} {%endfor%}) VALUES ({% for field in not_id(query.fieldlist) %} {%if not loop.is_first%},{%endif%}${{loop.index1}} {%endfor%}) "
	 "ON CONFLICT DO UPDATE {% for field in not_primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}}= EXCLUDED.{{escape_token(field.name)}} {%endfor%} returning {{escape_token(id_name(query.fieldlist))}}", 
		std::forward_as_tuple({% for field in query.fieldlist %}{%if not loop.is_first%}, {%endif%}rec.{{field.name}} {%endfor%}), rec.{{id_name(query.fieldlist)}});
	return ok;
{% else %}
	uint64_t affected = 0;
	db.execute_direct("INSERT INTO {{escape_token(query.text)}} ({% for field in query.fieldlist %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}} {%endfor%}) VALUES ({% for field in query.fieldlist %} {%if not loop.is_first%},{%endif%}{{loop.index1}} {%endfor%}) "
	 "ON CONFLICT DO UPDATE {% for field in not_primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}}= EXCLUDED.{{escape_token(field.name)}} {%endfor%} returning {{escape_token(id_name(query.fieldlist))}}", affected,
		{% for field in query.fieldlist %}{%if not loop.is_first%}, {%endif%}rec.{{field.name}} {%endfor%});
	if(affected>0)
	{
		{% if id_name(query.fieldlist)!=null %}
		uint64_t id=db.insert_id(); 
		if(id>0) rec.{{id_name(query.fieldlist)}}=id;
		{% endif %}
		return true;
	}
	return false;
{% endif %}
}
{% endif %}
{% if query.functions.erase %}
inline bool {{query.classname}}::clear(qtl::{{connection.type}}::database& db)
{
	uint64_t affected = 0;
	db.execute_direct("TRUNCATE {{escape_token(query.text)}}", &affected);
	return affected>0;
}
inline bool {{query.classname}}::erase(qtl::{{connection.type}}::database& db, {% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}const {{field.type}}& {{field.name}} {%endfor%})
{
	uint64_t affected = 0;
	db.execute("DELETE FROM {{escape_token(query.text)}} WHERE {% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}}=${{loop.index1}} {%endfor%}", 0, {% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{field.name}} {%endfor%}, &affected);
	return affected>0;
}
inline bool {{query.classname}}::erase_if(qtl::{{connection.type}}::database& db, const char* condition)
{
	uint64_t affected = 0;
	if(condition)
		db.execute_direct(std::string("DELETE FROM {{escape_token(query.text)}} WHERE ")+condition, &affected);
	else
		clear(db);
	return affected>0;
}
{% endif %}
{% if query.functions.erase %}
inline bool {{query.classname}}::update(qtl::{{connection.type}}::database& db, const record& rec)
{
	uint64_t affected = 0;
	db.execute_direct("UPDATE {{escape_token(query.text)}} SET {% for field in not_primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{escape_token(field.name)}}={{loop.index1}} {%endfor%} WHERE {% for field in primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}{{field.name}}=${{loop.index1}} {%endfor%}", &affected,
		{% for field in not_primary(query.fieldlist) %} {%if not loop.is_first%},{%endif%}rec.{{field.name}} {%endfor%} {% for field in primary(query.fieldlist) %}, rec.{{field.name}} {%endfor%});
	return affected>0;
}
{% endif %}

{% endfor %}
	
}

#endif //_{{upper(filename)}}_H_
