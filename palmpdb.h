/*

  Palm Database (PDB) Access Library
  This library provides routines and structures for reading and writing
  Palm database files.
 
  This software was written by John R. Hall <kg4ruo@arrl.net>,
  but it is in the public domain. I believe that free software
  should be truly free and not encumbered by license hassles.
  There is no warranty of any sort pertaining to this code.
 
*/


#ifndef PALMPDB_H
#define PALMPDB_H

/* PDB header attributes. */

#define PDB_ATTR_READONLY       2   /* database is read-only */
#define PDB_ATTR_DIRTY_APPINFO  4   /* dirty AppInfo area */
#define PDB_ATTR_BACKUP         8   /* backup this database (no conduit exists) */
#define PDB_ATTR_OVERWRITE     16   /* ok to overwrite older versions when installing */
#define PDB_ATTR_FORCE_RESET   32   /* reset unit after installing this database */
#define PDB_ATTR_COPY_PREVENT  64   /* impede beaming of this database */

/* PDB record attributes. */

#define PDB_REC_SECRET         16   /* secret record */
#define PDB_REC_BUSY           32   /* record in use */
#define PDB_REC_DIRTY          64   /* record modified */
#define PDB_REC_DELETED       128   /* purge on next HotSync */

/* In-memory PDB database representation.
   Not byte compatible with the file structure. */

typedef struct PDB {

    char name[32];
    /* 32 byte, null terminated database name. */

    unsigned int attributes;
    /* File attributes. See attribute constants above.
       Stored as 2-byte big endian in the actual file. */

    unsigned int version;
    /* App-defined. Stored as 2-byte big endian. */

    unsigned int creation_time;
    /* Creation time in seconds since Jan 1, 1904.
       Stored as 4-byte big endian. Do not set this to zero. */

    unsigned int modification_time;
    /* Last modification time in seconds since Jan 1, 1904.
       Stored as 4-byte big endian. Do not set this to zero. */

    unsigned int backup_time;
    /* Last backup time in seconds since Jan 1, 1904.
       Stored as 4-byte big endian. This MAY be set to zero. */

    char type[5];
    /* Database type ID. Stored with a null terminator for
       convenience, but the file does not contain one. */

    char creator[5];
    /* Application creator ID. Stored just like the type field. */

    void *app_info_block;
    unsigned int app_info_length;
    /* AppInfo block. For compatibility, it wise to make this
       exactly 512 byte, or not have an AppInfo block at all. */

    struct PDB_Record* records;
    unsigned int num_records;

} PDB;


typedef struct PDB_Record {

    unsigned int attributes;
    /* Attributes. See constants above. */

    unsigned int length;
    /* Length in bytes. I think 64k is the max; haven't verified. */

    void *data;
    /* Raw data. */

} PDB_Record;


int PDB_WriteFile(struct PDB* pdb, const char* filename);
/* Outputs a PDB file with the given contents.
   Returns 0 on success, -1 on failure. */

int PDB_ReadFile(struct PDB* pdb, const char* filename);
/* Reads a PDB file into memory.
   Returns 0 on success, -1 on failure. */

void PDB_Init(struct PDB* pdb, const char* name, unsigned int version, const char* type, const char* creator);
/* Initializes the basic fields in a PDB structure.
   Pass the type and creator ID as ordinary strings.
   Starts with zero records and no AppInfo block. */

void PDB_Free(struct PDB* pdb);
/* Frees the contents of a PDB structure. */

int PDB_SetAppInfoBlock(struct PDB* pdb, const void* block, unsigned int bytes);
/* Sets an AppInfo block for this PDB.
   Frees the previous AppInfo block, if any.
   If block is NULL, simply removes the previous block.
   Returns 0 on success, -1 on failure. */

int PDB_SetNumRecords(struct PDB* pdb, unsigned int num);
/* Sets the number of records in a PDB. If there are already records in the database,
   resizes the record list to accomodate. Returns 0 on success, -1 on failure. */

int PDB_SetRecord(struct PDB* pdb, unsigned int rec, const void *data, unsigned int length, unsigned int attr);
/* Sets the given record to the provided data. Makes a local copy of the data.
   Frees the previous contents of the record, if any.
   To simply wipe out a record, pass NULL data.   
   Returns 0 on success, -1 on failure. */

int PDB_LoadRecordFromFile(struct PDB* pdb, unsigned int rec, const char* filename, int terminate, unsigned int attr);
/* Loads the first 64k of a file into a record. Applies the given attributes
   to the record. If terminate is nonzero, null-terminates the data. (Useful for
   loading text files into records, for instance.)
   Returns 0 on success, -1 on failure. */

#endif
