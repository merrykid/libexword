/* exword - program for transfering files to Casio-EX-Word dictionaries
 *
 * Copyright (C) 2010 - Brian Johnson <brijohn@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *
 */

#include <popt.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <locale.h>
#include <libgen.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "exword.h"

#define CMD_NONE	0x0
#define CMD_LIST	0x1
#define CMD_MODEL	0x2
#define CMD_SEND	0x4
#define CMD_DELETE	0x8
#define CMD_CAPACITY	0x10
#define CMD_DISCONNECT	0x20
#define CMD_CONNECT	0x40
#define CMD_GET		0x80
#define CMD_FORMAT	0x100
#define CMD_SETPATH	0x200

#define CMD_MASK	0x3ff
#define REQ_CON_MASK	(CMD_LIST      | \
			CMD_MODEL      | \
			CMD_SEND       | \
			CMD_DELETE     | \
			CMD_CAPACITY   | \
			CMD_DISCONNECT | \
			CMD_GET        | \
			CMD_FORMAT     | \
			CMD_SETPATH)

#define CMD_INTERACTIVE 0x8000

uint8_t  debug_level;
uint16_t command;
uint16_t open_opts;
char    *filename;
char    *sd_path;
char    *mem_path;



struct poptOption options[] = {
        { "sd", 0, POPT_ARG_STRING, &sd_path, 0,
        "access external sd card" },
        { "internal", 0, POPT_ARG_STRING, &mem_path, 0,
        "access internal memory (default)" },
        { "list", 0, POPT_BIT_SET, &command, CMD_LIST,
        "list files on device" },
        { "send", 0, POPT_BIT_SET, &command, CMD_SEND,
        "send file to device" },
        { "get", 0, POPT_BIT_SET, &command, CMD_GET,
        "get file from device" },
        { "delete", 0, POPT_BIT_SET, &command, CMD_DELETE,
        "delete file from device" },
        { "model", 0, POPT_BIT_SET, &command, CMD_MODEL,
        "get device model" },
        { "capacity", 0, POPT_BIT_SET, &command, CMD_CAPACITY,
        "get device capacity" },
        { "format", 0, POPT_BIT_SET, &command, CMD_FORMAT,
        "format SD Card" },
        { "interactive", 0, POPT_BIT_SET, &command, CMD_INTERACTIVE,
        "interactive mode" },
        { "file", 0, POPT_ARG_STRING, &filename, 0,
        "file name to transfer/delete" },
        { "debug", 0, POPT_ARG_INT, &debug_level, 0,
        "debug level (0..5)" },
        POPT_AUTOHELP
        { NULL, 0, 0, NULL, 0 }
   };


void usage(poptContext optCon, int exitcode, char *error) {
	poptPrintUsage(optCon, stderr, 0);
	if (error) fprintf(stderr, "%s\n", error);
	poptFreeContext(optCon);
	exit(exitcode);
}

int read_file(char* filename, char **buffer, int *len)
{
	int fd;
	struct stat buf;
	*buffer = NULL;
	if (filename == NULL)
		return 0x44;
	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return 0x44;
	if (fstat(fd, &buf) < 0) {
		close(fd);
		return 0x44;
	}
	*buffer = malloc(buf.st_size);
	if (*buffer == NULL) {
		close(fd);
		return 0x50;
	}
	*len = read(fd, *buffer, buf.st_size);
	if (*len < 0) {
		free(buffer);
		*buffer = NULL;
		close(fd);
		return 0x50;
	}
	close(fd);
	return 0x20;
}

int write_file(char* filename, char *buffer, int len)
{
	int fd, ret;
	struct stat buf;
	fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0)
		return 0x43;
	ret = write(fd, buffer, len);
	if (ret < 0) {
		close(fd);
		return 0x50;
	}
	close(fd);
	return 0x20;
}

void print_entry(directory_entry_t *entry)
{
	int len;
	char * name;
	if (entry->flags & LIST_F_UNICODE) {
		name = utf16_to_locale(&name, &len, entry->name, entry->size - 3);
	} else {
		name = entry->name;
	}
	if (entry->flags & LIST_F_DIR)
		printf("<%s>\n", name);
	else
		printf("%s\n", name);
}

int list_files(exword_t *d)
{
	int rsp, i;
	directory_entry_t *entries;
	uint16_t len;
	rsp = exword_list(d, &entries, &len);
	if (rsp != 0x20)
		goto fail;
	for (i = 0; i < len; i++) {
		print_entry(entries + i);
	}
	exword_free_list(entries);
fail:
	return rsp;
}

int display_capacity(exword_t *d)
{
	int rsp;
	exword_capacity_t cap;
	rsp = exword_get_capacity(d, &cap);
	if (rsp != 0x20)
		goto fail;
	printf("Capacity: %d / %d\n", cap.total, cap.used);
fail:
	return rsp;
}

int send_file(exword_t *d, char *filename)
{
	int rsp, len;
	char *buffer;
	rsp = read_file(filename, &buffer, &len);
	if (rsp != 0x20)
		goto fail;
	rsp = exword_send_file(d, basename(filename), buffer, len);
fail:
	free(buffer);
	return rsp;
}

int get_file(exword_t *d, char *filename)
{
	int rsp, len;
	char *buffer = NULL, *fname_copy = NULL;
	fname_copy = strdup(filename);
	rsp = exword_get_file(d, basename(fname_copy), &buffer, &len);
	if (rsp != 0x20)
		goto fail;
	rsp = write_file(filename, buffer, len);
fail:
	free(fname_copy);
	free(buffer);
	return rsp;
}

int set_path(exword_t *d, char* device, char *pathname)
{
	char *path, *p;
	int rsp, i = 0, j;
	path = malloc(strlen(device) + strlen(pathname) + 2);
	if (!path)
		return 0x50;
	strcpy(path, device);
	strcat(path, "\\");
	strcat(path, pathname);
	p = path;
	while(p[0] != '\0') {
		while((p[0] == '/' || p[0] == '\\') &&
		      (p[1] == '/' || p[1] == '\\')) {
			char *t = p;
			while(t[0] != '\0') {
				t++;
				t[-1] = t[0];
			}
		}
		if (p[0] == '/')
			p[0] = '\\';
		p++;
	}
	rsp = exword_setpath(d, path, SETPATH_NOCREATE);
fail:
	free(path);
	return rsp;
}

int delete_file(exword_t *d, char *filename)
{
	int rsp;
	if (filename == NULL)
		return 0x40;
	rsp = exword_remove_file(d, basename(filename));
fail:
	return rsp;
}

int display_model(exword_t *d)
{
	int rsp;
	exword_model_t model;
	rsp = exword_get_model(d, &model);
	if (rsp != 0x20)
		goto fail;
	printf("Model: %s\nSub: %s\n", model.model, model.sub_model);
fail:
	return rsp;
}

int sd_format(exword_t *d)
{
	int rsp;
	rsp = exword_sd_format(d);
	return rsp;
}

int connect(exword_t *d)
{
	int rsp;
	rsp = exword_connect(d);
	return rsp;
}

int disconnect(exword_t *d)
{
	int rsp;
	rsp = exword_disconnect(d);
	return rsp;
}

int parse_commandline(char *cmdl)
{
	int ret = 0, r1;
	char *cmd = NULL;
	command &= ~CMD_MASK;
	sscanf(cmdl, "%ms", &cmd);

	if (strcmp(cmd, "help") == 0) {
		printf("Commands:\n");
		printf("connect [type] [locale]   - connect to attached dictionary\n");
		printf("disconnect                - disconnect to dictionary\n");
		printf("model                     - print model\n");
		printf("setpath <sd|mem>://<path> - switch storage medium\n");
		printf("list                      - list files\n");
		printf("capacity                  - display medium capacity\n");
		printf("format                    - format SD card\n");
		printf("delete  <filename>        - delete filename\n");
		printf("send    <filename>        - upload filename\n");
		printf("debug   <number>          - set debug level (0-5)\n");
	} else if (strcmp(cmd, "connect") == 0) {
		char type[10];
		char locale[3];
		int error = 0;
		open_opts = 0;
		r1 = sscanf(cmdl, "%*s %10s %2s", type, locale);
		if (r1 <= 0) {
			open_opts = OPEN_LIBRARY|LOCALE_JA;
		} else {
			if (strcmp(type, "library") == 0) {
				open_opts |= OPEN_LIBRARY;
			} else if (strcmp(type, "text") == 0) {
				open_opts |= OPEN_TEXT;
			} else if (strcmp(type, "cd") == 0) {
				open_opts |= OPEN_CD;
			} else {
				printf("Unknown 'type': %s\n", type);
				error = 1;
			}
			if (!error && r1 > 1) {
				if (strcmp(locale, "ja") == 0) {
					open_opts |= LOCALE_JA;
				} else if (strcmp(locale, "kr") == 0) {
					open_opts |= LOCALE_KR;
				} else if (strcmp(locale, "cn") == 0) {
					open_opts |= LOCALE_CN;
				} else if (strcmp(locale, "de") == 0) {
					open_opts |= LOCALE_DE;
				} else if (strcmp(locale, "es") == 0) {
					open_opts |= LOCALE_ES;
				} else if (strcmp(locale, "fr") == 0) {
					open_opts |= LOCALE_FR;
				} else if (strcmp(locale, "ru") == 0) {
					open_opts |= LOCALE_RU;
				} else {
					printf("Unknown 'locale': %s\n", locale);
					error = 1;
				}
			} else {
				open_opts |= LOCALE_JA;
			}
				
		}
		if (!error)
			command |= CMD_CONNECT;
	} else if (strcmp(cmd, "setpath") == 0) {
		free(sd_path);
		sd_path = NULL;
		free(mem_path);
		mem_path = NULL;
		if (sscanf(cmdl, "%*s sd://%ms", &sd_path) > 0 || 
		    sscanf(cmdl, "%*s mem://%ms", &mem_path) > 0) {
			command |= CMD_SETPATH;
		} else {
			printf("Invalid argument. Format (sd|mem)://<path>\n");
		}
	} else if(strcmp(cmd, "delete") == 0 ||
		  strcmp(cmd, "send") == 0   ||
		  strcmp(cmd, "get") == 0) {
		free(filename);
		filename = NULL;
		sscanf(cmdl, "%*s %ms", &filename);
		if (filename == NULL) {
			printf("Requires filename\n");
		} else {
			if (strcmp(cmd, "delete") == 0)
				command |= CMD_DELETE;
			else if (strcmp(cmd, "send") == 0)
				command |= CMD_SEND;
			else
				command |= CMD_GET;
		}
	} else if(strcmp(cmd, "debug") == 0) {
		if (sscanf(cmdl, "debug %hhu", &debug_level) < 1)
			printf("Requires debug level\n");
		else if (debug_level > 5)
			printf("Value should be between 0 and 5\n");
		else
			exword_set_debug(debug_level);
	} else if(strcmp(cmd, "exit") == 0) {
		ret = 1;
	} else if (strcmp(cmd, "disconnect") == 0) {
		command |= CMD_DISCONNECT;
	} else if (strcmp(cmd, "model") == 0) {
		command |= CMD_MODEL;
	} else if (strcmp(cmd, "capacity") == 0) {
		command |= CMD_CAPACITY;
	} else if (strcmp(cmd, "list") == 0) {
		command |= CMD_LIST;
	} else if (strcmp(cmd, "format") == 0) {
		command |= CMD_FORMAT;
	} else {
		ret = -1;
	}
	free(cmd);
	return ret;
}

void interactive() {
	char * line = NULL;
	int rsp, ret = 0;
	exword_t *handle = NULL;

	printf("exword interactive mode\n");
	while (ret != 1) {
		free(line);
		line = readline(">> ");
		if (line == NULL || *line == '\0')
			continue;

		add_history(line);
		ret = parse_commandline(line);

		if (ret < 0) {
			printf("Invalid command\n");
			continue;
		}

		if (handle == NULL &&
		    command & REQ_CON_MASK) {
			printf("Not connected\n");
			continue;
		}

		switch(command & CMD_MASK) {
		case CMD_CONNECT:
			printf("connecting to device...");
			handle = exword_open2(open_opts);
			if (handle == NULL) {
				printf("device not found\n");
			} else {
				exword_set_debug(debug_level);
				if(connect(handle) != 0x20) {
					printf("connect failed\n");
					exword_close(handle);
					handle = NULL;
				} else {
					rsp = set_path(handle, INTERNAL_MEM, "/");
					printf("done\n");
				}
			}
			break;
		case CMD_DISCONNECT:
			printf("disconnecting...");
			disconnect(handle);
			exword_close(handle);
			handle = NULL;
			printf("done\n");
			break;
		case CMD_SETPATH:
			if (sd_path)
				rsp = set_path(handle, SD_CARD, sd_path);
			else 
				rsp = set_path(handle, INTERNAL_MEM, mem_path);
			if (rsp != 0x20)
				printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_MODEL:
			rsp = display_model(handle);
			if (rsp != 0x20)
				printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_LIST:
			rsp = list_files(handle);
			if (rsp != 0x20)
				printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_CAPACITY:
			rsp = display_capacity(handle);
			if (rsp != 0x20)
				printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_DELETE:
			printf("deleting...");
			rsp = delete_file(handle, filename);
			printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_SEND:
			printf("uploading...");
			rsp = send_file(handle, filename);
			printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_GET:
			printf("downloading...");
			rsp = get_file(handle, filename);
			printf("%s\n", exword_response_to_string(rsp));
			break;
		case CMD_FORMAT:
			printf("formatting...");
			rsp = sd_format(handle);
			printf("%s\n", exword_response_to_string(rsp));
			break;
		}
	}
	if (handle != NULL) {
		disconnect(handle);
		exword_close(handle);
	}
	free(line);
	free(filename);
	free(sd_path);
	free(mem_path);
}

int main(int argc, const char** argv)
{
	poptContext optCon;
	exword_t *device;
	int rsp;
	optCon = poptGetContext(NULL, argc, argv, options, 0);

	setlocale(LC_ALL, "");

	if (poptGetNextOpt(optCon) < -1)
		usage(optCon, 1, NULL);

	if (command & CMD_INTERACTIVE) {
		interactive();
		exit(0);
	}

	if ((command & CMD_MASK) == 0)
		usage(optCon, 1, NULL);

	if (sd_path && mem_path)
		usage(optCon, 1, "--sd and --internal options mutually exclusive");

	device = exword_open();
	if (device == NULL) {
		fprintf(stderr, "Failed to open device or no device found\n");
	} else {
		exword_set_debug(debug_level);
		rsp = connect(device);
		if (rsp == 0x20) {
			if (sd_path)
				rsp = set_path(device, SD_CARD, sd_path);
			else if (mem_path)
				rsp = set_path(device, INTERNAL_MEM, mem_path);
			else
				rsp = set_path(device, INTERNAL_MEM, "\\");
		}
		if (rsp == 0x20) {
			switch(command & CMD_MASK) {
			case CMD_LIST:
				rsp = list_files(device);
				break;
			case CMD_MODEL:
				rsp = display_model(device);
				break;
			case CMD_CAPACITY:
				rsp = display_capacity(device);
				break;
			case CMD_SEND:
				rsp = send_file(device, filename);
				break;
			case CMD_GET:
				rsp = get_file(device, filename);
				break;
			case CMD_DELETE:
				rsp = delete_file(device, filename);
				break;
			case CMD_FORMAT:
				rsp = sd_format(device);
				break;
			default:
				usage(optCon, 1, "No such command");
			}
		}
		disconnect(device);
		printf("%s\n", exword_response_to_string(rsp));
		exword_close(device);
	}
	poptFreeContext(optCon);
	return 0;
}
