/* libexword - library for transffering files to Casio-EX-Word dictionaries
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
#ifndef EXWORD_H
#define EXWORD_H

#include <stdint.h>


typedef struct _exword exword_t;

#define SD_CARD		"\\_SD_00"
#define INTERNAL_MEM	"\\_INTERNAL_00"
#define ROOT		""


#define SETPATH_NOCREATE 0x02

#define LIST_F_DIR     1
#define LIST_F_UNICODE 2

#define LOCALE_JA      0x20
#define LOCALE_KR      0x40
#define LOCALE_CN      0x60
#define LOCALE_DE      0x80
#define LOCALE_ES      0xa0
#define LOCALE_FR      0xc0
#define LOCALE_RU      0xe0

#define OPEN_LIBRARY   0x0000
#define OPEN_TEXT      0x0100
#define OPEN_CD        0x0200

#pragma pack(1)
typedef struct {
	uint16_t size;   //size of structure
	uint8_t  flags;  //flags 0 = file 1 = directory 2 = unicode
	uint8_t  *name;  //name of entry
} directory_entry_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	uint32_t total;
	uint32_t used;
} exword_capacity_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	char model[15];
	char sub_model[6];
} exword_model_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	char name[17];
} exword_userid_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	char challenge[20];
} exword_authchallenge_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	unsigned char cdkey[16];
	unsigned char username[24];
	char challenge[20];
} exword_authinfo_t;
#pragma pack()

#pragma pack(1)
typedef struct {
	unsigned char username[16];
	unsigned char directory[12];
	unsigned char key[12];
} exword_cryptkey_t;
#pragma pack()

typedef void (*file_cb)(char *, uint32_t, uint32_t, void *);

char * utf16_to_locale(char **dst, int *dstsz, const char *src, int srcsz);
char * locale_to_utf16(char **dst, int *dstsz, const char *src, int srcsz);
char *exword_response_to_string(int rsp);
void exword_set_debug(int level);
void exword_register_callbacks(exword_t *self, file_cb get, file_cb put, void *userdata);
void exword_free_list(directory_entry_t *entries);
exword_t * exword_open();
exword_t * exword_open2(uint16_t options);
void exword_close(exword_t *self);
int exword_connect(exword_t *self);
int exword_send_file(exword_t *self, char* filename, char *buffer, int len);
int exword_get_file(exword_t *self, char* filename, char **buffer, int *len);
int exword_remove_file(exword_t *self, char* filename);
int exword_get_model(exword_t *self, exword_model_t * model);
int exword_get_capacity(exword_t *self, exword_capacity_t *cap);
int exword_sd_format(exword_t *self);
int exword_setpath(exword_t *self, uint8_t *path, uint8_t flags);
int exword_list(exword_t *self, directory_entry_t **entries, uint16_t *count);
int exword_userid(exword_t *self, exword_userid_t id);
int exword_cryptkey(exword_t *self, exword_cryptkey_t *key);
int exword_cname(exword_t *self, char *name, char* dir);
int exword_unlock(exword_t *self);
int exword_lock(exword_t *self);
int exword_authchallenge(exword_t *self, exword_authchallenge_t challenge);
int exword_authinfo(exword_t *self, exword_authinfo_t *info);
int exword_disconnect(exword_t *self);
#endif
