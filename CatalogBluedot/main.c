#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "sqlite3.h"

#define BUFFER_SIZE 4096
sqlite3 *ppDb;
sqlite3_stmt *stmt;
char sSQL[BUFFER_SIZE] = "\0";
void do_dir( DIR *dp, const char *path, size_t omit );
int count = 0;

int main (int argc, const char * argv[]) {
    // insert code here...
    if (argc != 3) {
        fprintf( stderr, "CatalogBluedot requires two parameters:\n%s [path to database] [path to start scanning]\n", argv[0]);
        return 0;
    }

	int sql_err = 0;
	char * sErrMsg = 0;
    const char *tail = 0;
    sql_err = sqlite3_open( argv[1], &ppDb );
    
    if (sql_err != 0) {
        fprintf(stderr, "Could not open live.sqlite database file: %s: %d\n", argv[1], sql_err);
        return sql_err;
    }
    
	sql_err = sqlite3_exec(ppDb, "DROP TABLE IF EXISTS PATHS;", NULL, NULL, &sErrMsg);
    
    if (sql_err != 0) {
        fprintf(stderr, "Could not drop PATH table: %d\n%s\n", sql_err, sErrMsg);
        sqlite3_close( ppDb );
        return sql_err;
    }
    
	sql_err = sqlite3_exec(ppDb, "CREATE TABLE IF NOT EXISTS PATHS (id INTEGER PRIMARY KEY, path TEXT, name TEXT);", NULL, NULL, &sErrMsg);

    if (sql_err != 0) {
        fprintf(stderr, "Could not create PATH table");
        sqlite3_close( ppDb );
        return sql_err;
    }
    
    sprintf(sSQL, "INSERT INTO PATHS VALUES (NULL, @path, @name);");
    sql_err = sqlite3_exec(ppDb, "BEGIN TRANSACTION", NULL, NULL, &sErrMsg);

    sql_err = sqlite3_prepare_v2(ppDb, sSQL, BUFFER_SIZE, &stmt, &tail);
    
    if (stmt == NULL) {
        fprintf(stderr,"Prepared failed\n" );
        return -1;
    }

	DIR *dp;
    dp = opendir (argv[2]);
	do_dir( dp, argv[2], strlen( argv[2] ) );

	(void) closedir (dp);
    sqlite3_exec(ppDb, "END TRANSACTION", NULL, NULL, &sErrMsg);
    sqlite3_finalize(stmt);
    sqlite3_close( ppDb );
    
    printf( "CatalogBluedot file count: %d", count );
    
	return 0;
}

void do_dir( DIR *dp, const char *path, size_t omit )
{
	struct dirent *ep;     

	if (dp != NULL)
	{
		while ((ep = readdir (dp)))
		{
			if (ep->d_name[0] == '.') {
				continue;
			}
			
			if (ep->d_type == DT_DIR) {
				/* do dir */
				
                size_t newPathLen = strlen(path) + ep->d_namlen + 1 + 1;
                char *newPath = malloc( newPathLen );
                strlcpy(newPath, path, newPathLen);
                strlcat(newPath, "/", newPathLen);
                size_t result = strlcat(newPath, ep->d_name, newPathLen);

                if (result < newPathLen) {
                    DIR *stack_dp = opendir(newPath);
                    do_dir( stack_dp, newPath, omit );
                    (void) closedir(stack_dp);
                }

                free( newPath );
				continue;
			}
			
			if (ep->d_type != DT_REG) {
				continue;
			}
			
			/* do reg file */
            sqlite3_bind_text(stmt, 1, path + omit + 1, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, ep->d_name, -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_clear_bindings(stmt);
            sqlite3_reset(stmt);
            count++;
		}
	}
	else
		fprintf(stderr, "Couldn't open the directory: %s\n", path);
	
}
