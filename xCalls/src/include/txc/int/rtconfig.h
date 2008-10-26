#ifndef _TXC_CONFIG_H
#define _TXC_CONFIG_H

#include <txc/int/result.h>

#define VALIDVAL0	{} 
#define VALIDVAL2(val1, val2)	{val1, val2} 

#define FOREACH_RUNTIME_CONFIG_OPTION(ACTION)																\
	ACTION(printconf, string, char *, "disabled", 														\
																VALIDVAL2("enabled", "disabled"), 2)				\
	ACTION(statistics, string, char *, "disabled", 														\
																VALIDVAL2("enabled", "disabled"), 2)				\
	ACTION(privlogger, string, char *, "disabled", 														\
																VALIDVAL2("enabled", "disabled"), 2)				\
	ACTION(privlogger_fileprefix, string, char *, "privlog", VALIDVAL0, 0)		\
	ACTION(globlogger, string, char *, "disabled", 														\
																VALIDVAL2("enabled", "disabled"), 2)				\
	ACTION(globlogger_fileprefix, string, char *, "globlog", VALIDVAL0, 0)

#define CONFIG_OPTION_ENTRY(name, typename, type, defvalue, validvalues,		\
																													num_validvalues)	\	
	type name;

typedef struct txc_runtime_settings_s txc_runtime_settings_t;

struct txc_runtime_settings_s {
	FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_ENTRY)  
};
 
extern txc_runtime_settings_t txc_runtime_settings;

/* Interface functions */
txc_result_t txc_rtconfig_runtime_settings_init ();

#endif /* _TXC_CONFIG_H */
