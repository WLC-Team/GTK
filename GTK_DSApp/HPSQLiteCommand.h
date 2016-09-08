#ifndef HPSQLITECOMMAND_H
#define HPSQLITECOMMAND_H

#include "HPSQLiteConnection.h"



//callback to print the fetched data in console
static int callback(void *data, int argc, char **argv, char **azColName)
{
int i;
//fprintf(stderr, "%s: ", (const char*)data);
for (i = 0; i<argc; i++){
printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
}
printf("\n");
return 0;
}


class HPSQLiteCommand
{
 public:
	//char* column_value;
	char* SqlDBCmdStr;
	char *szErrMsg;
	HPSQLiteConnection* connection;

 public:
	HPSQLiteCommand(char* _SqlDBCmdStr, HPSQLiteConnection* conn);
	int ExecuteNonQuery();
	char* ExecuteScalar();

};

#endif

