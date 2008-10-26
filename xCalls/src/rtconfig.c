#include <stdio.h>
#include <string.h>
#include <txc/int/transaction.h>
#include <txc/int/rtconfig.h>
#include <txc/int/debug.h>
#include <txc/int/result.h>


#define CONFIG_OPTION_DEFVALUE(name, typename, type, defvalue, validvalues,	\
																													num_validvalues) 	\
	defvalue,

#define DEFVALUES																														\
	{	FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_DEFVALUE)	}

#define CONFIG_OPTION_VALID_VALUES_ENTRY(name, typename, type, defvalue,		\
																							validvalues, num_validvalues)	\
	type name[num_validvalues]; 

#define CONFIG_OPTION_VALID_VALUES(name, typename, type, defvalue, 					\
																							validvalues, num_validvalues)	\
	validvalues, 

#define VALIDVALUES																													\
	{ FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_VALID_VALUES) }

txc_runtime_settings_t txc_runtime_settings = DEFVALUES;
static char *txc_init_filename = "txc.ini";

#define CONFIG_OPTION_KEY(name, typename, type, defvalue, validvalues,			\
																									num_validvalues)						\
	{#name, typename##_data, &(txc_runtime_settings.name),										\
																		 &(valid_values.name), num_validvalues},

typedef enum {
	integer_data, integer_range_data, string_data	
} option_valuetype_t;

typedef struct option_s option_t;

struct option_s {
	char *name;
	option_valuetype_t type;
	void *value_ptr;
	void *validvalues_ptr;
	int num_validvalues;
};

static struct {
		FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_VALID_VALUES_ENTRY)
} valid_values = VALIDVALUES;  


static 
int 
parse_line(FILE *fin, char *option, char *value) 
{
	char buf[1024];
	char *subtoken, *ret;
	ret = fgets(buf, 1024, fin);
	if (ret == NULL) {
		return 1; /* end of file */ 
	}
	subtoken = strtok(buf, "=");
	if (subtoken == NULL) {
		return 2;	/* parse error */ 
	}
	if (subtoken[0] == '#') {
		return 3;	/* ignore parameter */
	}
	strcpy(option, subtoken);
	subtoken = strtok(NULL, "\n");
	if (subtoken == NULL) {
		return 2;	/* parse error */ 
	}
	strcpy(value, subtoken);
	return 0;
}  


txc_result_t
txc_rtconfig_runtime_settings_init ()
{
	int i, j, value_is_valid;
	int int_value;
	int known_parameter;
	FILE *fin;
	char buf_option[128];
	char buf_value[128];
	int done;

	option_t options[] = {
		FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_KEY)
	};

	if ((fin = fopen(txc_init_filename, "r")) == NULL) {
		return TXC_R_INVALIDFILE;
	} 
	done = 0;
	while (!done) {
		switch(parse_line(fin, buf_option, buf_value)) {
			case 1: /* end of file */
				done = 1;
				continue;
			case 2: /* parse error */
				TXC_ERROR("Error while parsing file %s.", txc_init_filename);
				txc_assert(0); /* never returns */
			case 3: /* ignore parameter */
				continue;
		}
		known_parameter = 0;
		for (i=0; i < sizeof (options) / sizeof(option_t); i++) {
			if (strcmp(options[i].name, buf_option) == 0) {
				switch (options[i].type) {
					case integer_range_data: 
						if (options[i].num_validvalues != 2) {
							TXC_INTERNALERROR("config.h has wrong range definition");
						}
						int_value = atoi(buf_value);
						value_is_valid = 1;
						if (int_value < ((int *) options[i].validvalues_ptr)[0]
									|| int_value > ((int *) options[i].validvalues_ptr)[1]) {
							value_is_valid = 0;
							TXC_WARNING(
								"Value given for parameter '%s' is out of range. Using default value.", 
									options[i].name); 	
						}
						if (value_is_valid) {
							*((int *) options[i].value_ptr) = int_value;
						}
						break;
					case string_data: 
						if (options[i].num_validvalues > 0) {
							value_is_valid = 0;
							for (j=0; j < options[i].num_validvalues; j++) {
								if (strcmp(buf_value, ((char **) options[i].validvalues_ptr)[j]) == 0) {
									value_is_valid = 1; 
								}
							} 
						} else {
							value_is_valid = 1;
						}
						if (value_is_valid == 0) {
							TXC_WARNING(
								"Value given for parameter '%s' is not recognized. Using default value.", 
									options[i].name); 	
						} else {
							char *bufp;
							bufp = (char *) malloc(strlen(buf_value)+1);
							strcpy(bufp, buf_value);
							*( (char **) options[i].value_ptr) = bufp;
						}
						break;
					default:
						txc_assert(0);
				}	
				known_parameter = 1;
				break;
			}  
		}		
		if (known_parameter == 0) {
			TXC_WARNING(
						"Ignoring unknown parameter '%s'.", buf_option); 	
		} 
	}

	/* Print the configuration if asked */
	if (strcmp(txc_runtime_settings.printconf, "enabled") == 0) {
		fprintf(TXC_DEBUG_OUT, "Configuration parameters\n");
		fprintf(TXC_DEBUG_OUT, "========================\n");
		for (i=0; i < sizeof (options) / sizeof(option_t); i++) {
			fprintf(TXC_DEBUG_OUT, "%s=", options[i].name);
			switch(options[i].type) {
				case integer_range_data:
				case integer_data:
					fprintf(TXC_DEBUG_OUT, "%d\n", *((int *) options[i].value_ptr));
					break;
				case string_data:
					fprintf(TXC_DEBUG_OUT, "%s\n", *((char **) options[i].value_ptr));
					break;
				default:
					txc_assert(0);
			} 
		}
	}
		
	return TXC_R_SUCCESS; 	
}
