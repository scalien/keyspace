#include "Config.h"

class ConfigVar
{
public:
	union var 
	{
		const char *string;
		int intval;
	};
};
