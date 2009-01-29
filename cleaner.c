#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define VERSION "Version: $Revision: 1.4 $ Julian T. J. Midgley <jtjm@xenoclast.org>"
#define DEFAULT_EXPIRY 86400


int noop = 0, verbose = 0;
int indent_level = 0;

int
is_dir (char *name)
{
    struct stat statbuf;
    int ret;

    ret = stat(name, &statbuf);
    if (ret == -1) {
        return 0;
    }
    if (S_ISDIR(statbuf.st_mode)) {
        return 1;
    }
    return 0;
}


void
indent() 
{
    int i;
    if (verbose) 
        for (i = 0; i < indent_level; i++) {
            printf(" ");
        }
}

void
disp_cwd()
{
    char * cwd;

    if (verbose > 1) {
        cwd = getcwd(NULL,0);
        if (cwd == NULL) 
            perror("getcwd failed");
        else {
            fprintf(stderr, "CWD = %s: ", cwd);
            /* free (cwd); */
        }
    }
}


/****************************************************************************
 * cleandir(dirname, expiry)                                                *
 *                                                                          *
 * recurses through <dirname>, deleting files that are older than           *
 * <expiry> seconds.  It deletes subdirectories only if they are            *
 * empty, and never deletes <dirname> itself.                               *
 ****************************************************************************/
        
int
cleandir(char *dirname, time_t expiry)
{
    DIR * directory; 
    struct stat buf;
    struct dirent *entry;
    int retcode, num_ents;
    char *filename, *cwd;
    time_t now;

    if (verbose > 1)
        fprintf(stderr, "Expiry set to: %d\n", (int)expiry);

    num_ents = 0; /* Number of entries left in current directory */
    indent_level++;
            
    /* Open target directory */
    directory = opendir(dirname);
    if (directory == NULL) {
        fprintf(stderr, "%s: ", dirname);
        perror ("Unable to read directory");
        return -1;
    }
    if ((chdir(dirname) == -1)) {
        fprintf(stderr, "%s: ", dirname);
        perror("chdir failed");
        return -1;
    }
  
    /* Process directory contents, deleting all regular files with
     * mtimes more than expiry seconds in the past */

    now = time(NULL);  
    while ((entry = readdir(directory))) {
        filename = entry->d_name;

        /* Ignore '.' and '..' */
        if (! strcmp(filename,".")  ) { continue; }
        if (! strcmp(filename,"..") ) { continue; }
    
        num_ents ++; /* New entry, count it */
    
        retcode = lstat(filename, &buf);
        if (retcode == -1) {
            if (verbose > 1) {
                cwd = getcwd(NULL,0);
                if (cwd == NULL) 
                    perror("getcwd failed");
                else {
                    fprintf(stderr, "CWD = %s: ", cwd);
                    free (cwd);
                }
            }
            fprintf(stderr, "%s: ", filename);
            perror("stat failed");
            continue;
        }

        if (S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) {
            /* File or symlink- check last modification time */
            if ((now - expiry) > buf.st_mtime) {
                if (verbose) {
                    indent(); printf ("Deleting %s\n", filename); 
                }
                if (! noop) 
                    unlink (filename); 
                num_ents --;
            }
        }
        else if (S_ISDIR(buf.st_mode)) {
                /* Directory */
            if (verbose) { 
                indent(); printf ("Recursing into %s\n", filename); 
            }
            if (cleandir(filename, expiry) == 0) {
                    /* No entries left in a child directory, delete it */
                if (verbose) { 
                    indent (); printf ("rmdir'ing %s\n", filename); 
                }
                if (! noop) 
                    rmdir(filename); 
                num_ents --;
            }
        }
    }
    closedir(directory);
    indent_level--;
    if (verbose) { 
        indent(); printf("Leaving %s\n", dirname); 
    }
    if (verbose > 2) {
        disp_cwd();
        fprintf(stderr, "going up... ");
    }
    chdir("..");
    if (verbose > 2) {
        disp_cwd(); 
        fprintf(stderr, "\n");
    }
    return num_ents;
}

void
usage(int full) {
   printf( "Usage: cleaner <dirname> [<expiry>]\n");
   if (full) {
       printf( " -v : verbose\n"
               " -t : test only - don't delete any files\n"
               VERSION "\n");
   }
}
/*---------------------------------------------------------------------------*/
int 
main (int argc, char **argv) {
    char *dirname;
    char c;
    time_t expiry;
    int help = 0;

    /* Argument processsing */
    while ((--argc > 0 ) && ((*++argv)[0] == '-')) {
        while ((c = *++argv[0])) {
            switch (c) {
            case 'v':           /* Verbose mode */
                verbose ++;
                break;
            case 't':           /* Test only */
                noop = 1;
                fprintf(stderr, "** Test Mode - no files will be deleted **\n");
                break;
            case 'h':
                help = 1;
                break;
            default:
                printf("cleaner: illegal option %c\n", c);
                argc = 0;
                break;
            }
        }
    }
    
    if (verbose > 2)
        fprintf(stderr, "argc = %d\n", argc) ;

    if (argc < 1) {
        usage(help);
        return -1;
    }
    
    dirname = argv[0];
    if (argc > 1) {
        expiry = (time_t)atoi(argv[1]);
    } else {
        expiry = (time_t)DEFAULT_EXPIRY;
    }
    if (expiry < 1) {
        printf ("Must have a positive expiry time\n");
        return -1;
    }

    /* Race condition */
/*     if (! is_dir(dirname)) {
        printf ("%s is not a directory\n", dirname);
        return 1;
        } 
*/
    cleandir(dirname, expiry);
    return 0;
}
