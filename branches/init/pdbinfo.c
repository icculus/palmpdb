/*

  Palm Database (PDB) Access Library
  Utility for reading and dumping PDB files.
 
  This software was written by John R. Hall <kg4ruo@arrl.net>,
  but it is in the public domain. I believe that free software
  should be truly free and not encumbered by license hassles.
  There is no warranty of any sort pertaining to this code.
 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "palmpdb.h"

static void ShowPDBInfo(PDB* pdb)
{
    unsigned int r;

    printf("Title:         %s\n", pdb->name);
    printf("Attributes:    %Xh (%s, %s, %s, %s, %s, %s)\n", pdb->attributes,
	   pdb->attributes & PDB_ATTR_READONLY ? "read only": "read-write",
	   pdb->attributes & PDB_ATTR_DIRTY_APPINFO ? "dirty appinfo": "clean appinfo",
	   pdb->attributes & PDB_ATTR_BACKUP ? "back up": "don't back up",
	   pdb->attributes & PDB_ATTR_OVERWRITE ? "overwrite older": "don't overwrite older",
	   pdb->attributes & PDB_ATTR_FORCE_RESET ? "reset after install": "don't reset after install",
	   pdb->attributes & PDB_ATTR_COPY_PREVENT ? "copy prevent": "no copy prevent");
    printf("Records:       %u\n", pdb->num_records);
    printf("Version:       %u\n", pdb->version);
    printf("Creation time: %u\n", pdb->creation_time);
    printf("Mod time:      %u\n", pdb->modification_time);
    printf("Backup time:   %u\n", pdb->backup_time);
    printf("Type ID:       %s\n", pdb->type);
    printf("Creator ID:    %s\n", pdb->creator);
    printf("App info:      %u bytes\n", pdb->app_info_length);

    for (r = 0; r < pdb->num_records; r++) {
	printf("  Record %i:\n", r);
	printf("    Length: %i\n", pdb->records[r].length);
	printf("    Attr:   %Xh (%s, %s, %s, %s)\n", pdb->records[r].attributes,
	       pdb->records[r].attributes & PDB_REC_SECRET ? "secret" : "not secret",
	       pdb->records[r].attributes & PDB_REC_BUSY ? "busy" : "not busy",
	       pdb->records[r].attributes & PDB_REC_DELETED ? "deleted" : "not deleted",
	       pdb->records[r].attributes & PDB_REC_DIRTY ? "dirty" : "not dirty");
    }
}

static void DumpPDB(PDB* pdb)
{
    FILE *f;
    unsigned int i;

    if (pdb->app_info_length > 0) {
	f = fopen("appinfo", "wb");
	if (f == NULL) {
	    printf("WARNING: unable to open 'appinfo'.\n");
	} else {
	    fwrite(pdb->app_info_block, pdb->app_info_length, 1, f);
	    fclose(f);
	}
    }

    for (i = 0; i < pdb->num_records; i++) {
	char name[13];
	snprintf(name, 12, "record%i", i); name[12] = '\0';
	f = fopen(name, "wb");
	if (f == NULL) {
	    printf("WARNING: unable to open '%s'.\n", name);
	} else {
	    fwrite(pdb->records[i].data, pdb->records[i].length, 1, f);
	    fclose(f);
	}
    }
}

int main(int argc, char *argv[])
{
    PDB pdb;

    if (argc < 3) {
	printf("Usage: %s command filename.pdb\n\n", argv[0]);
	printf("  command is one of the following:\n"
	       "    show   Shows all available info about the database.\n"
	       "    dump   Dumps all records to files.\n\n"
	       "This program has no warranty.\n"
	       "Please report bugs to John R. Hall <kg4ruo@arrl.net>.\n");
	       
	return EXIT_FAILURE;
    }

    if (PDB_ReadFile(&pdb, argv[2]) < 0) {
	printf("Unable to read '%s'.\n", argv[2]);
	return EXIT_FAILURE;
    }

    if (!strcmp(argv[1], "show")) {
	ShowPDBInfo(&pdb);
    } else if (!strcmp(argv[1], "dump")) {
	DumpPDB(&pdb);
    } else {
	printf("'%s'? You speak nonsense.\n", argv[1]);
	PDB_Free(&pdb);
	return EXIT_FAILURE;
    }

    PDB_Free(&pdb);

    return EXIT_SUCCESS;
}
