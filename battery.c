#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include "common.h"

#define INTERVAL		3
#define FORMAT			"%s %i"
#define TOKSEP			"=\n"
#define BAT_PATH		"/sys/class/power_supply/BAT%i/uevent"
#define BAT_INDEX		0
#define KEY_PREFIX		"POWER_SUPPLY_"
#define STATUS_KEY		KEY_PREFIX "STATUS"
#define CAPACITY_KEY	KEY_PREFIX "CAPACITY"

#define PMATCH_LIMIT	16
#define STR_REGEX		"%(\\.?[0-9]+)?s"		// TODO: Add support for all relevant parameters, flags etc.
#define INT_REGEX		"%(\\.?[0-9]+)?[id]"	// ---------------------------//---------------------------

static regex_t str_regex;
static regex_t int_regex;
static regmatch_t pmatch[PMATCH_LIMIT];

int put_infos(char *path, char *format)
{
	FILE *bf = fopen(path, "r");
	if (bf == NULL)
	{
		fprintf(stderr, "Can't open '%s'\n", path);
		return EXIT_FAILURE;
	}

	char line[MAXLEN] = {0};
	char status[MAXLEN] = {0};
	int capacity = -1;
	bool found_status = false;
	bool found_capacity = false;
	
	while (fgets(line, sizeof(line), bf) != NULL)
	{
		char *key = strtok(line, TOKSEP);
		if (key != NULL)
		{ 
			if (!found_status && strcmp(key, STATUS_KEY) == 0)
			{
				strncpy(status, strtok(NULL, TOKSEP), sizeof(status)); 
				found_status = true;
			}
			else if (capacity == -1 && strcmp(key, CAPACITY_KEY) == 0)
			{
				capacity = atoi(strtok(NULL, TOKSEP));
				found_capacity = true;
			}
		}
	}
	fclose(bf);
	
	if (!found_capacity || !found_status)
	{
		fprintf(stderr, "Missing battery information\n");
		return EXIT_FAILURE;
	}
	else
	{
		if (regexec(&str_regex, format, PMATCH_LIMIT, pmatch, 0) == REG_NOMATCH)
		{
			printf(format, capacity);
		}
		else if (regexec(&int_regex, format, PMATCH_LIMIT, pmatch, 0) == REG_NOMATCH)
		{
			printf(format, status);
		}
		else
		{
			printf(format, status, capacity);
		}
		printf("\n");
		fflush(stdout);
	}
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	char *path = BAT_PATH;
	char *format = FORMAT;
	int index = BAT_INDEX;
	int interval = INTERVAL;
	bool snoop = false;

	int rc;
	if ((rc = regcomp(&str_regex, STR_REGEX, REG_EXTENDED)) != 0)
	{
		fprintf(stderr, "Failed to compile str_regex: %d\n", rc);
		exit(EXIT_FAILURE);
	}
	if ((rc = regcomp(&int_regex, INT_REGEX, REG_EXTENDED)) != 0)
	{
		fprintf(stderr, "Failed to compile int_regex: %d\n", rc);
		exit(EXIT_FAILURE);
	}

	int opt;
	while ((opt = getopt(argc, argv, "hsf:i:p:n:")) != -1)
	{
		switch (opt)
		{
			case 'h':
				printf("battery [-h|-s|-f FORMAT|-i INTERVAL|-p PATH|-n INDEX]\n");
				exit(EXIT_SUCCESS);
				break;
			case 's':
				snoop = true;
				break;
			case 'f':
				format = optarg;
				break;
			case 'p':
				path = optarg;
				break;
			case 'i':
				interval = atoi(optarg);
				break;
			case 'n':
				index = atoi(optarg);
				break;
		}
	}

	char real_path[MAXLEN] = {0};
	snprintf(real_path, sizeof(real_path), path, index);

	int exit_code;

	if (snoop)
	{
		while ((exit_code = put_infos(real_path, format)) != EXIT_FAILURE)
		{
			sleep(interval);
		}
	}
	else
	{
		exit_code = put_infos(real_path, format);
	}

	regfree(&str_regex);
	regfree(&int_regex);

	return exit_code;
}
