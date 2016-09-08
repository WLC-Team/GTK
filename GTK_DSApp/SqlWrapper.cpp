// SqlWrapper.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"

#include <stdio.h>
#include <iostream>
#include "sqlite3.h"
#include <string>
#include <vector>
#include "HPSQLiteConnection.h"
#include "HPSQLiteCommand.h"

using namespace std;

char* column_value = 0;

//writing a seperate callback for ExecuteScalar()
static int callback_executescalar(void *data, int argc, char **argv, char **azColName)
{
   column_value = argv[0];
   printf("%s\n",column_value);
   return 0;
}



	HPSQLiteConnection :: HPSQLiteConnection(char* _SqlDBPath)
	{
		SqlDBPath = _SqlDBPath;
	}
	void HPSQLiteConnection :: Open()
	{
		if (sqlite3_open(SqlDBPath, &SqlDB) != SQLITE_OK)
		{
			fprintf(stderr, "SQL error\n");

		}
		else
		{
			fprintf(stderr, "Opened database successfully\n");
			HPSQLiteConnection :: State = "Open";
		}
	}

	void HPSQLiteConnection :: Close()
	{
		int r = sqlite3_close(SqlDB);
	}

	sqlite3* HPSQLiteConnection :: GetDatabase()
	{
		return SqlDB;
	}



	HPSQLiteCommand :: HPSQLiteCommand(char* _SqlDBCmdStr, HPSQLiteConnection* conn)
	{
		SqlDBCmdStr = _SqlDBCmdStr;
		connection = conn;
		szErrMsg = 0;
	}
	int HPSQLiteCommand :: ExecuteNonQuery()
	{
		int rc = 0;
		rc = sqlite3_exec(HPSQLiteCommand::connection->GetDatabase(), SqlDBCmdStr, callback, 0, &szErrMsg);
		
		if (rc != SQLITE_OK)
		{
		fprintf(stderr, "SQL error: %s\n", szErrMsg);
		sqlite3_free(szErrMsg);
		}
		else
		{
		fprintf(stdout, "Operation done successfully\n");
		}
		return rc;
		
	}

	char* HPSQLiteCommand :: ExecuteScalar()
	{
		int r = 0;		
		r = sqlite3_exec( HPSQLiteCommand::connection->GetDatabase(), SqlDBCmdStr, callback_executescalar, 0, &szErrMsg);
		if( r != SQLITE_OK )
		{
		  fprintf(stderr, "SQL error: %s\n", szErrMsg);
		  sqlite3_free(szErrMsg);
		}
		else
		{
		  fprintf(stdout, "Operation Successful");
		  fprintf(stdout, "%s\n", column_value);
		}
		
		return column_value;
	   
         }



/*int main(int argc, char* argv[])
{
	HPSQLiteConnection *con = new HPSQLiteConnection("DemoT.db");
	con->Open();
	//char* sql = "CREATE TABLE IF NOT EXISTS users (firstname varchar(20), lastname varchar(20), email varchar(40) PRIMARY KEY , phonenum UInt64)";
	//HPSQLiteCommand *sql_cmd = new HPSQLiteCommand(sql, con);
	//int success1 = sql_cmd->ExecuteNonQuery();
	//if (success1 == SQLITE_OK)
	//	cout << "Sucessfully executed table";
	//char* sqlquery = "insert into  users (firstname, lastname, email, phonenum) values ('Soum','Bhowmick', 'soum@gmail.com', '9876543219' )";
	//HPSQLiteCommand *cmd = new HPSQLiteCommand(sqlquery, con);
	//int success = cmd->ExecuteNonQuery();
	//char* sqlquery2 = "insert into  users (firstname, lastname, email, phonenum) values ('tamy','y', 'tamy@gmail.com', '9879543219' )";
	//cmd = new HPSQLiteCommand(sqlquery2, con);
	//cmd->ExecuteNonQuery();
	//char* sqlquery3 = "insert into  users (firstname, lastname, email, phonenum) values ('tamy','k', 'tamy.k@gmail.com', '9879540219' )";
	//cmd = new HPSQLiteCommand(sqlquery3, con);
	//cmd->ExecuteNonQuery();
	//if (success == SQLITE_OK)
	//	cout << "Sucessfully executed command";
	//char* sqlsrchquery = "SELECT firstname,lastname, email, phonenum FROM users WHERE firstname = 'rupa'";
	char* sqlsrchquery = "SELECT COUNT(*) FROM users WHERE firstname = 'rupa'";
	HPSQLiteCommand *acmd = new HPSQLiteCommand(sqlsrchquery, con);
	//acmd->ExecuteNonQuery();// ExecuteScalar();
	char* s = acmd->ExecuteScalar();
	cout << s;
	//char *datatable[50][50];
	//HPSQLiteDataAdapter* da = new HPSQLiteDataAdapter(sqlsrchquery,con);
	//da->Fill(datatable);

	return 0;
}

*/
