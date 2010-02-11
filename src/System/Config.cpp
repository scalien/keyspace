#include "Config.h"
#include "Buffer.h"
#include "Log.h"
#include "Containers/Queue.h"


class ConfigVar
{
public:
	ConfigVar(const char* name_)
	{
		name.Clear();
		name.Append(name_, strlen(name_));
		value.Clear();
		numelem = 0;
		next = NULL;
	}

	void Append(const char *value_)
	{
		numelem++;
		value.Append(value_, strlen(value_) + 1);
	}

	bool NameEquals(const char *name_)
	{
		if (strncmp(name.buffer, name_, name.length) == 0)
			return true;
		return false;
	}

	int GetIntValue(int)
	{
		int			ret;
		unsigned	nread;
		char		last;
		
		ret = (int) strntoint64(value.buffer, value.length, &nread);

		// value is zero-terminated so we need the second character from backwards
		if (nread == value.length - 2)
		{
			last = value.buffer[value.length - 2];
			if (last == 'K')
				return ret * 1000;
			if (last == 'M')
				return ret * 1000 * 1000;
			if (last == 'G')
				return ret * 1000 * 1000 * 1000;
		}

		return ret;
	}

	const char *GetValue()
	{
		return value.buffer;
	}
	
	bool GetBoolValue(bool defval)
	{
		if (strcmp(value.buffer, "yes") == 0 ||
			strcmp(value.buffer, "true") == 0 ||
			strcmp(value.buffer, "on") == 0)
		{
			return true;
		}
		
		if (strcmp(value.buffer, "no") == 0 ||
			strcmp(value.buffer, "false") == 0 ||
			strcmp(value.buffer, "off") == 0)
		{
			return false;
		}
		
		return defval;
	}
	
	int GetListNum()
	{
		return numelem;
	}
	
	const char* GetListValue(int num, const char* defval)
	{
		const char* p;
		
		if (num >= numelem)
			return defval;
		
		p = value.buffer;
		for (int i = 0; i < num; i++)
		{
			p += strlen(p) + 1;
		}
		
		return p;
	}

	DynArray<256>	name;
	int				numelem;
	DynArray<256>	value;
	ConfigVar*		next;
};


Queue<ConfigVar, &ConfigVar::next> vars;


char* ParseToken(char* start, char* token, size_t)
{
	char* p;
	int len = 0;
	
	p = start;
	if (!p || !*p)
		return NULL;
	
	token[0] = '\0';
	
	// skip whitespace
skipwhite:
	while (*p <= ' ')
	{
		if (!*p || (*p == '\r' || *p == '\n'))
			return NULL;	// end of line
		p++;
	}
	
	// skip comments
	if (*p == '#')
	{
		p++;
		while (*p && (*p != '\r' || *p != '\n')) p++;
		goto skipwhite;
	}
	
	// quoted text is one token
	if (*p == '\"')
	{
		p++;
		while (true)
		{
			if (*p == '\"' || !*p)
			{
				token[len] = '\0';
				return p + 1;
			}
			token[len++] = *p++;
		}
	}
	
	if (*p == '=' || *p == ',')
	{
		token[len++] = *p;
		token[len] = '\0';
		return p + 1;
	}
		
	// regular word or number
	while (*p > ' ' && *p != ',' && *p != '#' && *p != '=')
	{
		token[len++] = *p++;
	}
	
	token[len] = '\0';
	return p;
}

bool Config::Init(const char* filename)
{
	FILE*		fp;
	char		buffer[1024];
	char		token[1024];
	char		name[1024];
	char*		p;
	int			nline;
	ConfigVar*	var;
	
	fp = fopen(filename, "r");
	if (!fp)
		return false;
	
	nline = 0;
	while (fgets(buffer, sizeof(buffer) - 1, fp))
	{
		buffer[sizeof(buffer) - 1] = '\0';
		nline++;
		p = ParseToken(buffer, token, sizeof(token));
		if (!p)
		{
			// empty or commented line
			continue;
		}
		
		strncpy(name, token, sizeof(name));
		name[sizeof(name) - 1] = '\0';
		
		p = ParseToken(p, token, sizeof(token));
		if (!p || token[0] != '=')
		{
			// syntax error
			Log_Message("syntax error at %s, line %d", filename, nline);
			fclose(fp);
			return false;
		}
		
		var = new ConfigVar(name);
		vars.Append(var);
		while (true)
		{
			p = ParseToken(p, token, sizeof(token));
			if (!p)
				break;
			if (token[0] == ',')
				continue;

			var->Append(token);
		}
	}
	
	fclose(fp);
	return true;
}

ConfigVar* GetVar(const char* name)
{
	ConfigVar* var;
	
	var = vars.Head();
	while (var)
	{
		if (var->NameEquals(name))
			return var;
		
		var = var->next;
	}
	
	return NULL;
}

int Config::GetIntValue(const char* name, int defval)
{
	ConfigVar* var;
	
	var = GetVar(name);
	if (!var)
		return defval;
	
	return var->GetIntValue(defval);
}

const char* Config::GetValue(const char* name, const char* defval)
{
	ConfigVar* var;
	
	var = GetVar(name);
	if (!var)
		return defval;
		
	return var->GetValue();
}

bool Config::GetBoolValue(const char* name, bool defval)
{
	ConfigVar* var;
	
	var = GetVar(name);
	if (!var)
		return defval;
	
	return var->GetBoolValue(defval);
}

int Config::GetListNum(const char* name)
{
	ConfigVar* var;
	
	var = GetVar(name);
	if (!var)
		return 0;

	return var->GetListNum();
}

const char* Config::GetListValue(const char* name, int num, const char* defval)
{
	ConfigVar* var;
	
	var = GetVar(name);
	if (!var)
		return defval;
	
	return var->GetListValue(num, defval);
}

