/*

  Palm Database (PDB) Access Library
  This library provides routines and structures for reading and writing
  Palm database files.
 
  This software was written by John R. Hall <kg4ruo@arrl.net>,
  but it is in the public domain. I believe that free software
  should be truly free and not encumbered by license hassles.
  There is no warranty of any sort pertaining to this code.
 
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "palmpdb.h"

#if (BYTE_ORDER == LITTLE_ENDIAN)
#define SWAP_BE_16(u16) (((u16 & 0xFF00) >> 8) + ((u16 & 0xFF) << 8))
#define SWAP_LE_16(u16) (u16)
#define SWAP_BE_32(u32) (((u32 & 0xFF000000) >> 24) + ((u32 & 0x00FF0000) >> 8) + ((u32 & 0x0000FF00) << 8) + ((u32 & 0x000000FF) << 24))
#define SWAP_LE_32(u32) (u32)
#else
#define SWAP_LE_16(u16) (((u16 & 0xFF00) >> 8) + ((u16 & 0xFF) << 8))
#define SWAP_BE_16(u16) (u16)
#define SWAP_LE_32(u32) (((u32 & 0xFF000000) >> 24) + ((u32 & 0x00FF0000) >> 8) + ((u32 & 0x0000FF00) << 8) + ((u32 & 0x000000FF) << 24))
#define SWAP_BE_32(u32) (u32)
#endif


int PDB_WriteFile(struct PDB* pdb, const char* filename)
{
    FILE* f;
    uint32_t val32;
    uint16_t val16;
    unsigned int data_start, cur_start;
    unsigned int i;

    f = fopen(filename, "wb");
    if (f == NULL)
	return -1;

    /* Write the header. */

    /* name: 32 bytes */
    fwrite(pdb->name, 32, 1, f);

    /* attributes: 16-bit big endian */
    val16 = pdb->attributes; val16 = SWAP_BE_16(val16);
    fwrite(&val16, 2, 1, f);

    /* version: 16-bit big endian */
    val16 = pdb->version; val16 = SWAP_BE_16(val16);
    fwrite(&val16, 2, 1, f);

    /* creation date: 32-bit big endian */
    val32 = pdb->creation_time; val32 = SWAP_BE_32(val32);
    fwrite(&val32, 4, 1, f);

    /* modification date: 32-bit big endian */
    val32 = pdb->modification_time; val32 = SWAP_BE_32(val32);
    fwrite(&val32, 4, 1, f);

    /* backup date: 32-bit big endian */
    val32 = pdb->backup_time; val32 = SWAP_BE_32(val32);
    fwrite(&val32, 4, 1, f);
    
    /* modification number: 32-bit big endian, always set to zero */
    val32 = 0;
    fwrite(&val32, 4, 1, f);

    /* appinfo area: 32-bit big endian location
       the appinfo area is always the first item of data, if it exists */
    data_start = 78 + pdb->num_records * 8;
    val32 = data_start; val32 = SWAP_BE_32(val32);
    fwrite(&val32, 4, 1, f);

    /* sortinfo area: 32-bit big endian location
       we don't use this */
    val32 = 0;
    fwrite(&val32, 4, 1, f);    
 
    /* database type: 4 bytes, not terminated */
    fwrite(pdb->type, 4, 1, f);

    /* creator ID: 4 bytes, not terminated */
    fwrite(pdb->creator, 4, 1, f);    
    
    /* unique ID seed: 32-bit big endian
       we don't use this */
    val32 = 0;
    fwrite(&val32, 4, 1, f);

    /* next record list ID: 32-bit big endian
       only for the in memory representation */
    val32 = 0;
    fwrite(&val32, 4, 1, f);

    /* number of records: 16-bit big endian */
    val16 = pdb->num_records; val16 = SWAP_BE_16(val16);
    fwrite(&val16, 2, 1, f);

    /* Write the record headers. */
    cur_start = data_start + pdb->app_info_length;
    for (i = 0; i < pdb->num_records; i++) {
	uint8_t attr;

	/* record data offset: 32-bit big endian */
	val32 = cur_start; val32 = SWAP_BE_32(val32);
	fwrite(&val32, 4, 1, f);

	/* attributes: 1 byte */
	attr = pdb->records[i].attributes;
	fwrite(&attr, 1, 1, f);

	/* unique ID: 3 bytes, set to zero */
	val32 = 0;
	fwrite(&val32, 3, 1, f);

	cur_start += pdb->records[i].length;
    }

    /* Write the data. */
    if (pdb->app_info_length != 0) {
	fwrite(pdb->app_info_block, pdb->app_info_length, 1, f);
    }

    for (i = 0; i < pdb->num_records; i++) {
	fwrite(pdb->records[i].data, pdb->records[i].length, 1, f);
    }

    fclose(f);
    return 0;
}

int PDB_ReadFile(struct PDB* pdb, const char* filename)
{
    FILE* f;
    uint32_t val32;
    uint16_t val16;
    unsigned int i;
    unsigned int app_info_offset;
    unsigned int pos;
    unsigned int num_records;
    unsigned int* record_offsets = NULL;

    f = fopen(filename, "rb");
    if (f == NULL)
	return -1;

    memset(pdb, 0, sizeof (struct PDB));

    /* database name */
    fread(pdb->name, 32, 1, f);
    pdb->name[31] = '\0';         /* don't trust that there's already one there */
    
    /* attributes */
    fread(&val16, 2, 1, f);
    pdb->attributes = SWAP_BE_16(val16);

    /* version */
    fread(&val16, 2, 1, f);
    pdb->version = SWAP_BE_16(val16);
    
    /* creation date */
    fread(&val32, 4, 1, f);
    pdb->creation_time = SWAP_BE_32(val32);
    
    /* modification date */
    fread(&val32, 4, 1, f);
    pdb->modification_time = SWAP_BE_32(val32);

    /* last backup date */
    fread(&val32, 4, 1, f);
    pdb->backup_time = SWAP_BE_32(val32);

    /* modification number; don't care */
    fread(&val32, 4, 1, f);

    /* appinfo offset */
    fread(&val32, 4, 1, f);
    app_info_offset = SWAP_BE_32(val32);

    /* sortinfo offset; don't care */
    fread(&val32, 4, 1, f);
    
    /* type and creator */
    fread(pdb->type, 4, 1, f);
    fread(pdb->creator, 4, 1, f);

    /* unique ID; don't care */
    fread(&val32, 4, 1, f);

    /* next record list ID; don't care */
    fread(&val32, 4, 1, f);

    /* number of records */
    fread(&val16, 2, 1, f);
    num_records = SWAP_BE_16(val16);

    /* Allocate the record list. */
    if (PDB_SetNumRecords(pdb, num_records) != 0)
	goto error;
    
    /* Read in the record headers. */
    record_offsets = (unsigned int *)malloc((num_records + 1) * sizeof (unsigned int));
    if (record_offsets == NULL)
	goto error;

    /* Figure out the length of the file. */
    pos = ftell(f);
    fseek(f, 0, SEEK_END);    
    record_offsets[num_records] = ftell(f);
    fseek(f, pos, SEEK_SET);

    /* Read record headers. */
    for (i = 0; i < num_records; i++) {
	uint8_t attr;

	/* offset */
	fread(&val32, 4, 1, f);
	record_offsets[i] = SWAP_BE_32(val32);
	
	/* attributes */
	fread(&attr, 1, 1, f);
	pdb->records[i].attributes = attr;

	/* unique ID; don't care */
	fread(&val32, 3, 1, f);
    }

    /* Read AppInfo area. */
    if (app_info_offset != 0) {
	pdb->app_info_length = record_offsets[0] - app_info_offset;
	pdb->app_info_block = malloc(pdb->app_info_length);
	if (pdb->app_info_block == NULL)
	    goto error;
	fseek(f, app_info_offset, SEEK_SET);
	fread(pdb->app_info_block, pdb->app_info_length, 1, f);
    }

    /* Read records. */
    for (i = 0; i < num_records; i++) {
	pdb->records[i].length = record_offsets[i+1] - record_offsets[i];
	pdb->records[i].data = malloc(pdb->records[i].length);
	if (pdb->records[i].data == NULL)
	    goto error;
	fseek(f, record_offsets[i], SEEK_SET);
	fread(pdb->records[i].data, pdb->records[i].length, 1, f);
    }

    free(record_offsets);

    return 0;

 error:
    fclose(f);
    free(record_offsets);
    PDB_Free(pdb);
    return -1;
}


void PDB_Init(struct PDB* pdb, const char* name, unsigned int version, const char* type, const char* creator)
{
    memset(pdb, 0, sizeof (struct PDB));
    strncpy(pdb->name, name, 31);
    strncpy(pdb->type, type, 4);
    strncpy(pdb->creator, creator, 4);
    /* Times are in seconds since January 1904.
       This probably isn't exactly right, but close enough. */
    pdb->creation_time = time(NULL) + 2082801600;
    pdb->modification_time = time(NULL) + 2082801600;
    pdb->version = version;
}

void PDB_Free(struct PDB* pdb)
{
    PDB_SetAppInfoBlock(pdb, NULL, 0);
    PDB_SetNumRecords(pdb, 0);
    memset(pdb, 0, sizeof (struct PDB));
}

int PDB_SetAppInfoBlock(struct PDB* pdb, const void* block, unsigned int bytes)
{
    void* old_block = pdb->app_info_block;
    if (block != NULL && bytes > 0) {
	pdb->app_info_block = malloc(bytes);	
	if (pdb->app_info_block == NULL) {
	    pdb->app_info_block = old_block;
	    return -1;
	}
	pdb->app_info_length = bytes;
    } else {
	pdb->app_info_block = NULL;
	pdb->app_info_length = 0;
    }
    free(old_block);
    return 0;
}

int PDB_SetNumRecords(struct PDB* pdb, unsigned int num)
{
    PDB_Record* new_data;

    if (num < pdb->num_records) {
	unsigned int i;
	for (i = num; i < pdb->num_records; i++) {
	    free(pdb->records[i].data);
	}
	if (num != 0) {
	    new_data = (PDB_Record*)realloc(pdb->records, num * sizeof (struct PDB_Record));
	    pdb->records = (new_data == NULL ? pdb->records : new_data);
	} else {
	    pdb->records = NULL;
	}	
	pdb->num_records = num;
    } else if (num > pdb->num_records) {
	unsigned int i;
	new_data = (PDB_Record*)realloc(pdb->records, num * sizeof (struct PDB_Record));
	if (new_data == NULL)
	    return -1;
	pdb->records = new_data;
	for (i = pdb->num_records; i < num; i++) {
	    memset(&pdb->records[i], 0, sizeof (struct PDB_Record));
	}
	pdb->num_records = num;
    }
    return 0;
}

int PDB_SetRecord(struct PDB* pdb, unsigned int rec, const void *data, unsigned int length, unsigned int attr)
{
    void* new_data;
    if (rec >= pdb->num_records)
	return -1;

    new_data = realloc(pdb->records[rec].data, length);
    if (new_data == NULL)
	return -1;

    pdb->records[rec].data = new_data;
    memcpy(pdb->records[rec].data, data, length);
    pdb->records[rec].attributes = attr;
    pdb->records[rec].length = length;

    return 0;
}

int PDB_LoadRecordFromFile(struct PDB* pdb, unsigned int rec, const char* filename, int terminate, unsigned int attr)
{
    FILE* f;
    void* data = NULL;
    unsigned int length;
    
    f = fopen(filename, "rb");
    if (f == NULL)
	return -1;
    
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    if (length > 0xFFFF) {
	length = 0xFFFF;
    }
    
    fseek(f, 0, SEEK_SET);    
    data = malloc(length + (terminate ? 1 : 0));
    if (data == NULL) {
	fclose(f);
	return -1;
    }
    fread(data, length, 1, f);
    fclose(f);
    if (terminate) {
	((char *)data)[length] = '\0';
    }
    if (PDB_SetRecord(pdb, rec, data, length + (terminate ? 1 : 0), attr) < 0) {
	fclose(f);
	free(data);
	return -1;
    }

    free(data);

    return 0;
}
