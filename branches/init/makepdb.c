/*

  Palm Database (PDB) Access Library
  Utility for constructing PDB databases from arbitrary files.
 
  This software was written by John R. Hall <kg4ruo@arrl.net>,
  but it is in the public domain. I believe that free software
  should be truly free and not encumbered by license hassles.
  There is no warranty of any sort pertaining to this code.
 
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "palmpdb.h"

#define DEFAULT_ATTRIBUTES     0
#define DEFAULT_REC_ATTRIBUTES 0

int main(int argc, char *argv[])
{
    unsigned int opt_rec_attributes = 0;
    int opt_terminate = 0, opt_sticky_terminate = 0;
    int arg;
    int rec = 0;
    PDB pdb;

    if (argc < 2) {
	goto usage;
    }

    PDB_Init(&pdb, "None", 0, "NoNE", "NONE");

    for (arg = 2; arg < argc; arg++) {
	if (!strcmp(argv[arg], "--ctime")) {
	    if (arg >= argc-1) goto usage;
	    arg++;
	    pdb.creation_time = strtoul(argv[arg], NULL, 10);
	} else if (!strcmp(argv[arg], "--mtime")) {
	    if (arg >= argc-1) goto usage;
	    arg++;
	    pdb.modification_time = strtoul(argv[arg], NULL, 10);
	} else if (!strcmp(argv[arg], "--btime")) {
	    if (arg >= argc-1) goto usage;
	    arg++;
	    pdb.backup_time = strtoul(argv[arg], NULL, 10);
	} else if (!strcmp(argv[arg], "--name")) {
	    if (arg >= argc-1) goto usage;
	    arg++;
	    if (strlen(argv[arg]) > 31)
		printf("WARNING: database name '%s' is too long; truncating to 31 chars.\n", argv[arg]);
	    strncpy(pdb.name, argv[arg], 31);
	} else if (!strcmp(argv[arg], "--creator")) {
	    if (arg >= argc-1) goto usage;
	    arg++;
	    memset(pdb.creator, 0, 4);
	    strncpy(pdb.creator, argv[arg], 4);
	} else if (!strcmp(argv[arg], "--type")) {
	    if (arg >= argc-1) goto usage;
	    arg++;
	    memset(pdb.type, 0, 4);
	    strncpy(pdb.type, argv[arg], 4);
	} else if (!strcmp(argv[arg], "--appinfo")) {
	    FILE* f;
	    void* data;
	    unsigned int length;

	    if (arg >= argc-1) goto usage;
	    arg++;
	    
	    f = fopen(argv[arg], "rb");
	    if (f == NULL) {
		printf("WARNING: unable to open '%s'; not adding AppInfo block.\n", argv[arg]);
		continue;
	    }

	    fseek(f, 0, SEEK_END);
	    length = ftell(f);
	    if (length > 0xFFFF) {
		printf("WARNING: truncating AppInfo block to 64k.\n");
		length = 0xFFFF;
	    }
	    fseek(f, 0, SEEK_SET);
	    data = malloc(length);
	    if (data == NULL) {
		printf("WARNING: unable to alloc %u bytes; not adding AppInfo block.\n", length);
		fclose(f);
		continue;
	    }
	    fread(data, length, 1, f);
	    fclose(f);
	    if (PDB_SetAppInfoBlock(&pdb, data, length) < 0) {
		printf("WARNING: unable to set AppInfo block.\n");
		free(data);
		continue;
	    }

	    printf("AppInfo block loaded from '%s', %u bytes.\n", argv[arg], length);
	    
	    free(data);

	} else if (!strcmp(argv[arg], "--readonly")) {
	    pdb.attributes |= PDB_ATTR_READONLY;
	} else if (!strcmp(argv[arg], "--dirty-appinfo")) {
	    pdb.attributes |= PDB_ATTR_DIRTY_APPINFO;
	} else if (!strcmp(argv[arg], "--backup")) {
	    pdb.attributes |= PDB_ATTR_BACKUP;
	} else if (!strcmp(argv[arg], "--reset")) {
	    pdb.attributes |= PDB_ATTR_FORCE_RESET;
	} else if (!strcmp(argv[arg], "--copy-prevent")) {
	    pdb.attributes |= PDB_ATTR_COPY_PREVENT;
	} else if (!strcmp(argv[arg], "--terminate")) {
	    opt_sticky_terminate = 1;
	    opt_terminate = 1;
	} else if (!strcmp(argv[arg], "+s")) {
	    opt_rec_attributes |= PDB_REC_SECRET;
	} else if (!strcmp(argv[arg], "+b")) {
	    opt_rec_attributes |= PDB_REC_BUSY;
	} else if (!strcmp(argv[arg], "+d")) {
	    opt_rec_attributes |= PDB_REC_DIRTY;
	} else if (!strcmp(argv[arg], "+x")) {
	    opt_rec_attributes |= PDB_REC_DELETED;
	} else if (!strcmp(argv[arg], "+t")) {
	    opt_terminate = 1;
	} else {
	    if (PDB_SetNumRecords(&pdb, rec+1) < 0) {
		printf("WARNING: unable to resize record list. Skipping '%s'.\n", argv[arg]);
		goto add_done;
	    }
	    if (PDB_LoadRecordFromFile(&pdb, rec, argv[arg], opt_terminate, opt_rec_attributes) < 0) {
		printf("WARNING: unable to load record from '%s'.\n", argv[arg]);
		goto add_done;
	    }
	    rec++;

	add_done:
	    opt_rec_attributes = DEFAULT_REC_ATTRIBUTES;
	    opt_terminate = opt_sticky_terminate;
	}
    }

    if (PDB_WriteFile(&pdb, argv[1]) < 0) {
	printf("ERROR: unable to write PDB file.\n");
	PDB_Free(&pdb);
	return EXIT_FAILURE;
    }

    PDB_Free(&pdb);
    return EXIT_SUCCESS;

 usage:

    printf("Usage: %s filename.pdb args files ...\n\n"
	   "  Generates Palm PDB databases from arbitrary files.\n"
	   "  Any number of arguments can be interleaved with any number of files.\n"
	   "  Each file will be added to the database as a separate record.\n"
	   "  Files longer than 65535 bytes will be truncated to fit.\n"
	   "  At the very least, you should probably provide the name, creator,\n"
	   "  and type attributes; most of the others have sane defaults.\n\n"
	   "Arguments:\n"
	   "\n  Basic per-database info:\n"
	   "    --name <string>     database name (31 chars max)\n"
	   "    --creator <string>  creator ID (4 chars max)\n"
	   "    --type <string>     type ID (4 chars max)\n"
	   "\n  Optional overrides:\n"
	   "    --ctime <seconds>   creation time (seconds since Jan 1, 1904)\n"
	   "                        If no time is given, the current time is used.\n"
	   "    --mtime <seconds>   time of last modification\n"
	   "    --btime <seconds>   time of last backup (often zero)\n"
	   "\n  AppInfo block:\n"
	   "    --appinfo <file>    reads an AppInfo block from the given file\n"
	   "\n  Database attributes:\n"
	   "    --readonly          makes the database read-only\n"
	   "    --dirty-appinfo     flags the AppInfo block as modified\n"
	   "    --backup            requests HotSync to routinely back up this database\n"
	   "    --reset             asks the PDA to restart when this database is installed\n"
	   "    --copy-prevent      marks this database as copy protected (not very secure)\n"
	   "\n  Null termination:\n"
	   "    --terminate         adds a null terminator (\\0) to every record in this database\n"
	   "\n  Per-record attributes (cleared to defaults between every file):\n"
	   "    +s         secret record\n"
	   "    +b         record is busy (not usually set)\n"
	   "    +d         record has been changed\n"
	   "    +x         mark record as deleted\n"
	   "    +t         add a null terminator (\\0) to this record only (see --terminate)\n\n"
	   "Example:\n"
	   "  To create a Memo Pad database from a collection of text files, use this:\n"
	   "    %s MemoDB.pdb --name MemoDB --type DATA --creator memo --terminate *.txt\n"
	   "  Note that Memo Pad doesn't support files larger than 4k. You may get odd results\n"
	   "  with larger files.\n\n"
	   "This program has no warranty.\n"
	   "Please report bugs to John R. Hall <kg4ruo@arrl.net>.\n",
	   argv[0], argv[0]);

    return EXIT_SUCCESS;
}
