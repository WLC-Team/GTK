#ifndef HPSQLITECONNECTION_H
#define HPSQLITECONNECTION_H



class HPSQLiteConnection
{
       public: char* SqlDBPath;
	       sqlite3*  SqlDB;
	       char* State;

        public:
	     HPSQLiteConnection(char* _SqlDBPath);
  	     void Open();
	     void Close();
	     sqlite3* GetDatabase();
};

#endif
