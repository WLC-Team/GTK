#include <stdio.h>
#include <iostream>
#include <ctype.h>
#include <gtk/gtk.h>
#include "sqlite3.h"
#include <string>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <boost/filesystem.hpp>
//#include <regex>
#include "HPSQLiteConnection.h"
#include "HPSQLiteCommand.h"

#ifdef _DEBUG
        #pragma comment(lib, "libcurl_a_debug.lib") //cURL Debug build
#else
        #pragma comment(lib, "libcurl_a.lib")
#endif
 
#pragma warning(disable: 4996)  //Disable Function or Variable may be unsafe warning

static const int CHARS= 76;     //Sending 54 chararcters at a time with \r , \n and \0 it becomes 57
static const int ADD_SIZE= 20;   // ADD_SIZE for TO,FROM,SUBJECT,CONTENT-TYPE,CONTENT-TRANSFER-ENCODING,CONETNT-DISPOSITION and \r\n
static const int SEND_BUF_SIZE= 54;
static char (*fileBuf)[CHARS] = NULL;
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

using namespace std;

HPSQLiteConnection* sql_con;
std::string email = "";

typedef struct
{
	GtkWidget *CreateUserBox;
	GtkWidget *SearchUserBox;
	GtkWidget *ShareFileBox;
	GtkWidget *CreateFirstNameText;
	GtkWidget *CreateLastNameText;
	GtkWidget *CreateEmailText;
	GtkWidget *CellText;
	GtkWidget *SearchFirstNameText;
	GtkWidget *SearchLastNameText;
	GtkWidget *SearchEmailText;
	GtkWidget *TypeEmailText;
	//GtkWidget *EmailToText;
	GtkWidget *BrowseFileText;
	GtkWidget *EmailBodyText;
	GtkWidget *searchresultlabel;
	GtkWidget *resultShowList;
	GtkWidget *listItemLabel;
	GtkWidget *SearchResultBox;
	GtkWidget *SelectEmailBox;
	//GtkWidget *EmailToBox;
	
	GtkWidget *SelectRadioButton1;
	GtkWidget *SelectRadioButton2;

} GDS;

bool LARGEFILE = false; /*For Percent*/
int status = 0;   /*For Percent*/
int percent2 = 0; /*For Percent*/
int percent3 = 0; /*For Percent*/
int percent4 = 0; /*For Percent*/
int percent5 = 0; /*For Percent*/

char* FILENAME;
char* FROM;
char* TO;
char* BODY;

static void LargeFilePercent(int rowcount)
{
        //This is for displaying the percentage
        //while encoding larger files
        int percent = rowcount/100;
 
        if(LARGEFILE == true) {
                status++;
                percent2++;
                if(percent2 == 18) {
                        percent3++;
                        percent2 = 0;
                }
                if(percent3 == percent){
                        percent4++;
                        percent3 = 0;
                }
                if(percent4 == 10){
                        system("cls");
                        cout << "Larger Files take longer to encode, Please be patient." << endl
                             << "Otherwise push X to exit program now." << endl << endl
                             << "(Push Anykey to Continue)" << endl
                             << endl << "Encoding " << FILENAME << " please be patient..." << endl;
                        cout << percent5 << "%";
                        percent5 += 10;
                        percent4 = 0;
                }
                if(status == 10000) {
                        if(percent5 == 0){cout << " 0%"; percent5 = 10;}
                        cout << ".";
                        status = 0;
                }
        }
}
 
static void encodeblock(unsigned char in[3], unsigned char out[4], int len)
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}
 
static void encode(FILE *infile, unsigned char *output_buf, int rowcount)
{
    unsigned char in[3], out[4];
        int i, len;
        *output_buf = 0;
 
    while(!feof(infile)) {
        len = 0;
        for(i = 0; i < 3; i++) {
            in[i] = (unsigned char) getc(infile);
            if(!feof(infile) ) {
                len++;
            }
            else {
                in[i] = 0;
            }
        }
        if(len) {
            encodeblock(in, out, len);
            strncat((char*)output_buf, (char*)out, 4);
        }
            LargeFilePercent(rowcount); //Display encoded file percent /*For Percent*/
        }
}
 
 
struct fileBuf_upload_status
{
  int lines_read;
};
 
static size_t read_file()
{
        FILE* hFile=NULL;
        size_t fileSize(0),len(0),buffer_size(0);
 
        //Open the file and make sure it exsits
        hFile = fopen(FILENAME,"rb");
        if(!hFile) {
                cout << "File not found!!!" << endl;
                exit (EXIT_FAILURE);
        }
 
        //Get filesize
        fseek(hFile,0,SEEK_END);
        fileSize = ftell(hFile);
        fseek(hFile,0,SEEK_SET);
   
        //Check Filesize
        if(fileSize > 256000/*bytes*/){
                cout << "Larger Files take longer to encode, Please be patient." << endl;
                LARGEFILE = true; /*For Percent*/
        }
        cout << endl << "Encoding " << FILENAME << " please be patient..." << endl;
 
        //Calculate the number of rows in Base64 encoded string
        //also calculate the size of the new char to be created
        //for the base64 encode string
        int no_of_rows = fileSize/SEND_BUF_SIZE + 1;
        int charsize = (no_of_rows*72)+(no_of_rows*2);
 
        //Base64 encode image and create encoded_buf string
        unsigned char* b64encode = new unsigned char[charsize];
        *b64encode = 0;
        encode(hFile, b64encode, no_of_rows /*For Percent*/);
        string encoded_buf = (char*)b64encode;
 
        if(LARGEFILE == true) cout << endl << endl; /*For Percent*/
 
        //Create structure of email to be sent
        fileBuf = new char[ADD_SIZE + no_of_rows][CHARS];  //ADD_SIZE for TO,FROM,SUBJECT,CONTENT-TYPE,CONTENT-TRANSFER-
                                                           //ENCODING,CONETNT-DISPOSITION and \r\n
        strcpy(fileBuf[len++],"To: ");
	strcpy(fileBuf[len++],TO);
	strcpy(fileBuf[len++],"\r\n");
        buffer_size += strlen(fileBuf[len-1]);
        strcpy(fileBuf[len++],"From: ");
	strcpy(fileBuf[len++],FROM);
 	strcpy(fileBuf[len++],"\r\n");
        buffer_size += strlen(fileBuf[len-1]);
        strcpy(fileBuf[len++],"Subject: Message from DSApp\r\n");
        buffer_size += strlen(fileBuf[len-1]);
	strcpy(fileBuf[len++],"Body: ");
	strcpy(fileBuf[len++],BODY);
	strcpy(fileBuf[len++],"\r\n");
	buffer_size += strlen(fileBuf[len-1]);
        strcpy(fileBuf[len++], "Content-Type: application/x-msdownload; name=\"");
	strcpy(fileBuf[len++], FILENAME);
 	strcpy(fileBuf[len++],"\"\r\n");
        buffer_size += strlen(fileBuf[len-1]);
        strcpy(fileBuf[len++],"Content-Transfer-Encoding: base64\r\n");
        buffer_size += strlen(fileBuf[len-1]);
        strcpy(fileBuf[len++], "Content-Disposition: attachment; filename=\"");
 	strcpy(fileBuf[len++],FILENAME);
 	strcpy(fileBuf[len++],"\"\r\n");
        buffer_size += strlen(fileBuf[len-1]);
        strcpy(fileBuf[len++],"\r\n");
        buffer_size += strlen(fileBuf[len-1]);
   
        //This part attaches the Base64 encoded string and
        //sets the Base64 linesize to 72 characters + \r\n
        int pos = 0;
        string sub_encoded_buf;
        for(int i = 0; i <= no_of_rows-1; i++)
        {
                sub_encoded_buf = encoded_buf.substr(pos*72,72);  //Reads 72 characters at a time
                sub_encoded_buf += "\r\n";                        //and appends \r\n at the end
                strcpy(fileBuf[len++], sub_encoded_buf.c_str());  //copy the 72 characters & \r\n to email
                buffer_size += sub_encoded_buf.size();            //now increase the buffer_size  
                pos++;                                            //finally increase pos by 1
        }

        delete[] b64encode;
        return buffer_size;
}
 
static size_t fileBuf_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
        struct fileBuf_upload_status *upload_ctx = (struct fileBuf_upload_status *)userp;
        const char *fdata;
 
        if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1))
        {
                return 0;
        }
 
        fdata = fileBuf[upload_ctx->lines_read];
 
        if(strcmp(fdata,""))
        {
                size_t len = strlen(fdata);
                memcpy(ptr, fdata, len);
                upload_ctx->lines_read++;
                return len;
        }
        return 0;
}

static int Send(char* FROM, char* TO, char* BODY)
{
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct fileBuf_upload_status file_upload_ctx;
	size_t file_size(0);
	
	file_upload_ctx.lines_read = 0;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	file_size = read_file();
	if(curl)
	{
	  curl_easy_setopt(curl, CURLOPT_USERNAME, "ta303924@wipro.com");
	  curl_easy_setopt(curl, CURLOPT_PASSWORD, "tmb@1995");
	  curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.office365.com:587");
	  curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
	  curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);
	  recipients = curl_slist_append(recipients, TO);
	  curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
	  curl_easy_setopt(curl, CURLOPT_INFILESIZE, file_size);
	  curl_easy_setopt(curl, CURLOPT_READFUNCTION, fileBuf_source);
	  curl_easy_setopt(curl, CURLOPT_READDATA, &file_upload_ctx);
	  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
	  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); 
	 
	  res = curl_easy_perform(curl);
	 
	  if(res != CURLE_OK)
	    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
	  curl_slist_free_all(recipients);
	  curl_easy_cleanup(curl);
	  curl_global_cleanup();
	}
	delete[] fileBuf;
	return (int)res;
}


static void CreateUserCallback(GtkWidget *widget,
	               GDS*   gds)
{
	gtk_widget_show(gds->CreateUserBox);
	gtk_widget_hide(gds->SearchUserBox);
	gtk_widget_hide(gds->ShareFileBox);
	gtk_entry_set_text(GTK_ENTRY(gds->CreateFirstNameText), "");
        gtk_entry_set_text(GTK_ENTRY(gds->CreateLastNameText), "");
        gtk_entry_set_text(GTK_ENTRY(gds->CreateEmailText), "");
        gtk_entry_set_text(GTK_ENTRY(gds->CellText), "");
}
static void SearchUserCallback(GtkWidget *widget,
	GDS*   gds)
{
 
	gtk_widget_hide(gds->CreateUserBox);
	gtk_widget_show(gds->SearchUserBox);
	gtk_widget_hide(gds->ShareFileBox);
	gtk_widget_hide(gds->searchresultlabel);
	gtk_widget_hide(gds->resultShowList);
	gtk_entry_set_text(GTK_ENTRY(gds->SearchFirstNameText), "");
        gtk_entry_set_text(GTK_ENTRY(gds->SearchLastNameText), "");
        gtk_entry_set_text(GTK_ENTRY(gds->SearchEmailText), "");
	
}
static void ShareFileCallback(GtkWidget *widget,
	GDS*   gds)
{
 
	gtk_widget_hide(gds->CreateUserBox);
	gtk_widget_hide(gds->SearchUserBox);
	gtk_widget_show(gds->ShareFileBox);
	//gtk_widget_hide(gds->SelectEmailBox);
	gtk_entry_set_text(GTK_ENTRY(gds->TypeEmailText), "");
	gtk_entry_set_text(GTK_ENTRY(gds->BrowseFileText), "");
	gtk_entry_set_text(GTK_ENTRY(gds->EmailBodyText), "");
	//gtk_frame_set_label(GTK_FRAME (gds->EmailToText), "" );
}

static void CLearCreateUserContent(GDS* gds)
{
 
    gtk_entry_set_text(GTK_ENTRY(gds->CreateFirstNameText), "");
    gtk_entry_set_text(GTK_ENTRY(gds->CreateLastNameText), "");
    gtk_entry_set_text(GTK_ENTRY(gds->CreateEmailText), "");
    gtk_entry_set_text(GTK_ENTRY(gds->CellText), "");

}

static void CLearSearchUserContent(GDS* gds)
{
    gtk_entry_set_text(GTK_ENTRY(gds->SearchFirstNameText), "");
    gtk_entry_set_text(GTK_ENTRY(gds->SearchLastNameText), "");
    gtk_entry_set_text(GTK_ENTRY(gds->SearchEmailText), "");
}

static void CLearShareFileContent(GDS* gds)
{
    gtk_entry_set_text(GTK_ENTRY(gds->TypeEmailText), "");
    gtk_entry_set_text(GTK_ENTRY(gds->BrowseFileText), "");
    gtk_entry_set_text(GTK_ENTRY(gds->EmailBodyText), "");
    //gtk_widget_hide(gds->SelectEmailBox);
    //gtk_frame_set_label(GTK_FRAME (gds->EmailToText), "" );
}

static bool isCharacter(char Character)
{
	return ( (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z'));
	//Checks if a Character is a Valid A-Z, a-z Character, based on the ascii value
}
/*bool isNumber(char Character)
{
	return ( Character >= '0' && Character <= '9');
	//Checks if a Character is a Valid 0-9 Number, based on the ascii value
}*/

static bool IsEmailvalid(std::string Email)
{
	char* EmailAddress = strdup(Email.c_str());
	if(!EmailAddress) // If cannot read the Email Address...
		return 0;
	if(!isCharacter(EmailAddress[0])) // If the First character is not A-Z, a-z
		return 0;
	int AtOffset = -1;
	int DotOffset = -1;
	unsigned int Length = strlen(EmailAddress); // Length = StringLength (strlen) of EmailAddress
	for(unsigned int i = 0; i < Length; i++)
	{
		if(EmailAddress[i] == '@') // If one of the characters is @, store it's position in AtOffset
			AtOffset = (int)i;
		else if(EmailAddress[i] == '.') // Same, but with the dot
			DotOffset = (int)i;
	}
	if(AtOffset == -1 || DotOffset == -1) // If cannot find a Dot or a @
		return 0;
	if(AtOffset > DotOffset) // If the @ is after the Dot
		return 0;
	return !(DotOffset >= ((int)Length-1)); //Chech there is some other letters after the Dot
}


static void LoadData()
{
    sql_con = new HPSQLiteConnection("Data Source=DemoT.db;Version=3;New=False;Compress=True;");
    sql_con->Open();
    char* sql = "CREATE TABLE IF NOT EXISTS users (firstname varchar(20), lastname varchar(20), email varchar(40) PRIMARY KEY , phonenum UInt64)";
    HPSQLiteCommand* sql_cmd = new HPSQLiteCommand(sql, sql_con);
    sql_cmd->ExecuteNonQuery();
    sql_con->Close();
}

static void CreateSubmitCallback(GtkWidget *widget,
	GDS*   gds)
{
    int phoneNo = 0;

    std::string firstNametxt(gtk_entry_get_text(GTK_ENTRY(gds->CreateFirstNameText)));
    std::string lastNametxt(gtk_entry_get_text(GTK_ENTRY(gds->CreateLastNameText)));
    std::string emailIDtxt(gtk_entry_get_text(GTK_ENTRY(gds->CreateEmailText)));
    std::string celltxt(gtk_entry_get_text(GTK_ENTRY(gds->CellText)));

    char* searchquery;
    
    if (!firstNametxt.empty() && !lastNametxt.empty() && !emailIDtxt.empty() && !celltxt.empty())
    {
       if(!celltxt.empty() && celltxt.length()<=10 && celltxt.length()>=8) 
       {
	phoneNo = atoi(celltxt.c_str());

        if (IsEmailvalid(emailIDtxt))
        {
            if (sql_con->State != "Open")
                sql_con->Open();

	    std::string select = "SELECT COUNT(*) FROM users WHERE email=";
	    std::string quotation = "'";
            std::string strsearchquery = select + quotation + emailIDtxt + quotation;
	    searchquery = strdup(strsearchquery.c_str());
 	    HPSQLiteCommand* cmd = new HPSQLiteCommand(searchquery, sql_con);
	    char* val = cmd->ExecuteScalar();
	    cout << val;
            std::string value(val);
	    cout << value;
	    int count = atoi(value.c_str());
	    cout << count;
            
            if ((count == 0) && IsEmailvalid(emailIDtxt))
            {
		std::string insert = "insert into  users (firstname, lastname, email, phonenum) values ('";
		std::string comma = "', '";
		std::string endbracket = "' )";
                std::string strSQLQuery = insert + firstNametxt + comma + lastNametxt + comma + emailIDtxt + comma + celltxt + endbracket;
		char* txtSQLQuery = strdup(strSQLQuery.c_str());
                HPSQLiteCommand* command = new HPSQLiteCommand(txtSQLQuery, sql_con);
                int success = command->ExecuteNonQuery();
                free(txtSQLQuery);
                if (success == 0)
                {
                   GtkDialogFlags flags1 = GTK_DIALOG_DESTROY_WITH_PARENT;
                    GtkWidget *dialog2 = gtk_message_dialog_new (NULL,
                                 flags1,
                                 GTK_MESSAGE_INFO,
                                 GTK_BUTTONS_OK,
                                 "User Successfully Created!");
                                
                   gtk_dialog_run (GTK_DIALOG (dialog2));
                   gtk_widget_destroy (dialog2); 
		  
		   CLearCreateUserContent(gds);
		   
               }

		char* HomePath;
  		HomePath = getenv ("HOME");
  		if (HomePath!=NULL)
    			cout << "The current path is:" << HomePath << "\n";
                
                std::string systemDrive = std::string(HomePath);
                std::string filename = "";

		int firstlength = emailIDtxt.find('@');
		filename = emailIDtxt.substr(0, firstlength);

		cout << filename << "\n";
                std::string folderLocation = systemDrive + "/" + filename;
                bool exists = boost::filesystem::is_directory(strdup(folderLocation.c_str()));
                
                if (!exists)
                {
                    bool creation_success = boost::filesystem::create_directory(strdup(folderLocation.c_str()));
		    
                }
               
            }
            else
            {
		GtkDialogFlags flags2 = GTK_DIALOG_DESTROY_WITH_PARENT;
                GtkWidget *dialog3 = gtk_message_dialog_new (NULL,
                                 flags2,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 "User Already Exists!");
                                
                gtk_dialog_run (GTK_DIALOG (dialog3));
                gtk_widget_destroy (dialog3); 

		CLearCreateUserContent(gds);	    

            }
        }
        else
        {
                GtkDialogFlags flags3 = GTK_DIALOG_DESTROY_WITH_PARENT;
                GtkWidget *dialog4 = gtk_message_dialog_new (NULL,
                                 flags3,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 "Email Address is Invalid!");
                                
                gtk_dialog_run (GTK_DIALOG (dialog4));
                gtk_widget_destroy (dialog4); 	 
                
                gtk_entry_set_text(GTK_ENTRY(gds->CreateEmailText), "");   

        }
       }
       else
       {
		GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
                GtkWidget *dialog1 = gtk_message_dialog_new (NULL,
                                 flags,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 "Please Enter Valid Phone Number!");
                                
               gtk_dialog_run (GTK_DIALOG (dialog1));
               gtk_widget_destroy (dialog1); 
	}
    }
    else
    {
                GtkDialogFlags flags4 = GTK_DIALOG_DESTROY_WITH_PARENT;
                GtkWidget *dialog5 = gtk_message_dialog_new (NULL,
                                 flags4,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 "Some Fields Missing!");
                                
                gtk_dialog_run (GTK_DIALOG (dialog5));
                gtk_widget_destroy (dialog5); 	    

    }
	
}


//callback for search function
static int callback_fill(void *data, int argc, char **argv, char **azColName)
{
   std::vector< std::vector<std::string> > *records = static_cast<std::vector< std::vector<std::string> >*>(data);
   std::vector<std::string> fields;
  
   for(int i=0; i<argc; i++)
  {
	std::string fieldvalue (argv[i]);
	fields.push_back(fieldvalue);
   }
   records->push_back(fields);
   return 0;
}

static void SelectedRowActivated(GtkListBox *box, GtkListBoxRow *row, GDS* gds)
{
     gtk_entry_set_text(GTK_ENTRY (gds->TypeEmailText), "");
     email = "";
     //gtk_frame_set_label(GTK_FRAME (gds->EmailToText), "" );
     if(gtk_list_box_get_selected_row(GTK_LIST_BOX (box) ))
     {
	int spacecount = 0;
    	row = gtk_list_box_get_selected_row(GTK_LIST_BOX (box) );
 	GtkWidget* selectedlabel = gtk_bin_get_child(GTK_BIN (row));
   	std::string selectedrow(gtk_label_get_text(GTK_LABEL (selectedlabel) ));
	for(int i=0; i<selectedrow.length(); i++)
	{
	  if(selectedrow[i] == ' ')
	    spacecount++;
	  if((spacecount == 40) && (selectedrow[i] != ' '))
	    email = email + selectedrow[i];
	}
	gtk_entry_set_text(GTK_ENTRY (gds->TypeEmailText), strdup(email.c_str()) );
	//gtk_frame_set_label(GTK_FRAME (gds->EmailToText), email.c_str() );
	gtk_widget_hide(gds->SearchUserBox);
	gtk_widget_show(gds->ShareFileBox);
	//gtk_widget_show(gds->SelectEmailBox);
	
     }
     else
     {
	cout << "\nfalse\n";
     }
}

static void SearchSubmitCallback(GtkWidget *widget,
	GDS*   gds)
{	
    gtk_container_remove(GTK_CONTAINER(gds->SearchResultBox), gds->resultShowList);
    
    gds->resultShowList = gtk_list_box_new ();
    gtk_list_box_set_selection_mode (GTK_LIST_BOX (gds->resultShowList), GTK_SELECTION_SINGLE);   //GTK_SELECTION_SINGLE	
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX (gds->resultShowList), FALSE);

    std::string firstNamesrchtxt(gtk_entry_get_text(GTK_ENTRY(gds->SearchFirstNameText)));
    std::string lastNamesrchtxt(gtk_entry_get_text(GTK_ENTRY(gds->SearchLastNameText)));
    std::string emailIDsrchtxt(gtk_entry_get_text(GTK_ENTRY(gds->SearchEmailText)));
    
    char* searchquery;
    std::string searchselect = "select firstname, lastname, email, phonenum from users where firstname = '";
    std::string andlastnamesearch = "' AND lastname = '";
    std::string orlastnamesearch = "' OR lastname = '";
    std::string emailsearch = "' OR email = '";
    std::string quoteend = "'";
    
    if (!firstNamesrchtxt.empty() && !lastNamesrchtxt.empty())
    {
        std::string search1 = searchselect + firstNamesrchtxt + andlastnamesearch + lastNamesrchtxt + quoteend;
	searchquery = strdup(search1.c_str());
    }
    else
    {
	std::string search2 = searchselect + firstNamesrchtxt + orlastnamesearch + lastNamesrchtxt + emailsearch + emailIDsrchtxt + quoteend;
        searchquery = strdup(search2.c_str());
    }
    
  
    if (sql_con->State != "Open")
        sql_con->Open();
    
    
    std::string eachfield="";
    char* insertvalue;
    std::vector< std::vector<std::string> > Records;
    char *errormsg;
    int fc = 0;
    fc = sqlite3_exec( sql_con->GetDatabase(), searchquery, callback_fill, &Records, &errormsg );

    if( fc != SQLITE_OK )
    {
	  fprintf(stderr, "SQL error %s\n", errormsg);
    }
    else
    {
	  fprintf(stdout, "Search Operation Successful\n");
          std::cerr << Records.size() << " records returned.\n";
	  if(Records.size() == 0)
	  {
		GtkDialogFlags flags5 = GTK_DIALOG_DESTROY_WITH_PARENT;
                GtkWidget *dialog6 = gtk_message_dialog_new (NULL,
                                 flags5,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 "No Records Found!");
                                
                gtk_dialog_run (GTK_DIALOG (dialog6));
                gtk_widget_destroy (dialog6); 	    
	  }
	  else
          {
	    gtk_widget_show_all(gds->searchresultlabel);
	    for(int i=0; i<Records.size(); i++)
            {
      		for(int j=0; j<Records[i].size(); j++)
      		{
			if(j == Records[i].size()-1)
				eachfield = eachfield + Records[i][j];
			else
				eachfield = eachfield + Records[i][j] + "                    ";
      		}

      		insertvalue = strdup(eachfield.c_str());
      		std::cerr << insertvalue << "\n";
		gds->listItemLabel = gtk_label_new(insertvalue);
		gtk_widget_set_halign (gds->listItemLabel, GTK_ALIGN_START);
      		gtk_list_box_insert(GTK_LIST_BOX (gds->resultShowList), gds->listItemLabel, i);
		
      		eachfield = "";
    	    }
	    gtk_box_pack_start( GTK_BOX (gds->SearchResultBox), gds->resultShowList, FALSE, FALSE, 10);
	    gtk_widget_show_all(gds->resultShowList);
	    g_signal_connect(gds->resultShowList, "row-activated", G_CALLBACK(SelectedRowActivated), gds);
	    CLearSearchUserContent(gds);
          }
	  
    }

    sql_con->Close();
	
}


static void BrowseFileCallback(GtkWidget *widget,
	GDS*   gds)
{
 
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
	gint res;

	dialog = gtk_file_chooser_dialog_new ("Open File", NULL, action, ("_Cancel"), GTK_RESPONSE_CANCEL, ("_Open"),
					       GTK_RESPONSE_ACCEPT, NULL);

	res = gtk_dialog_run (GTK_DIALOG (dialog));
	if (res == GTK_RESPONSE_ACCEPT)
 	 {
    		 char *filename;
   		 GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
   		 filename = gtk_file_chooser_get_filename (chooser);
		 gtk_entry_set_text(GTK_ENTRY(gds->BrowseFileText), filename);
   		 g_free (filename);
	  }

	gtk_widget_destroy (dialog);
	
	
}

static void SendFileCallback(GtkWidget *widget,
	GDS*   gds)
{
    TO = "";
    FROM = "";
    BODY = "";
    std::string strEmailID = "";
    std::string emailIDtxt(gtk_entry_get_text(GTK_ENTRY(gds->TypeEmailText)));
    //std::string selectedEmailtxt(gtk_frame_get_label(GTK_FRAME (gds->EmailToText)));	
    std::string File_Path(gtk_entry_get_text(GTK_ENTRY(gds->BrowseFileText)));
    std::string emailBody(gtk_entry_get_text(GTK_ENTRY(gds->EmailBodyText)));

    if (emailIDtxt.empty())
    {
            
	GtkDialogFlags flags6 = GTK_DIALOG_DESTROY_WITH_PARENT;
        GtkWidget *dialog7 = gtk_message_dialog_new (NULL,
                             flags6,
                             GTK_MESSAGE_ERROR,
                             GTK_BUTTONS_CLOSE,
                             "Please enter emailID/email folder !");
                                
        gtk_dialog_run (GTK_DIALOG (dialog7));
        gtk_widget_destroy (dialog7); 
	gtk_widget_grab_focus (gds->TypeEmailText);

     }
     else if (File_Path.empty())
     {
	GtkDialogFlags flags7 = GTK_DIALOG_DESTROY_WITH_PARENT;
        GtkWidget *dialog8 = gtk_message_dialog_new (NULL,
                             flags7,
                             GTK_MESSAGE_ERROR,
                             GTK_BUTTONS_CLOSE,
                             "Please browse file ! ");
                                
        gtk_dialog_run (GTK_DIALOG (dialog8));
        gtk_widget_destroy (dialog8); 
        gtk_widget_grab_focus (gds->BrowseFileText);

     }

     else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gds->SelectRadioButton2)))
     {
            if (emailIDtxt.find('@') != std::string::npos)
            {
		for(int i=0; i<emailIDtxt.find('@'); i++)
		{
                  strEmailID = strEmailID + emailIDtxt[i];
		}
                emailIDtxt = strEmailID;
            }

	    char* HomePath;
  	    HomePath = getenv ("HOME");
            std::string systemDrive = std::string(HomePath);
            
            
            std::string foldername = emailIDtxt;
            std:string folderLocation = systemDrive + "/" + foldername;
            bool exists = boost::filesystem::is_directory(strdup(folderLocation.c_str()));
	    
 	    std::string fileName = "";
            for(int j=File_Path.find_last_of('/') + 1; j<File_Path.length(); j++)
	    {
		fileName = fileName + File_Path[j];
	    }
            cout <<  fileName << "\n";
           
            std::string sourceFile = File_Path;
            std::string destFile = folderLocation + "/" + fileName;
            
            if (!exists)
            {
		GtkDialogFlags flags8 = GTK_DIALOG_DESTROY_WITH_PARENT;
        	GtkWidget *dialog9 = gtk_message_dialog_new (NULL,
                             	     flags8,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                    "Location does not exist !");
                                
        	gtk_dialog_run (GTK_DIALOG (dialog9));
        	gtk_widget_destroy (dialog9); 
            }
            else
            {
                if (!(boost::filesystem::exists(strdup(destFile.c_str()))))
                {
                    boost::filesystem::copy_file(strdup(sourceFile.c_str()), strdup(destFile.c_str()));

  		    GtkDialogFlags flags9 = GTK_DIALOG_DESTROY_WITH_PARENT;
        	    GtkWidget *dialog10 = gtk_message_dialog_new (NULL,
                             	          flags9,
                                          GTK_MESSAGE_INFO,
                                 	  GTK_BUTTONS_OK,
                                          "File Copied Successfully !");
                                
        	    gtk_dialog_run (GTK_DIALOG (dialog10));
        	    gtk_widget_destroy (dialog10); 

                    CLearShareFileContent(gds);
                }
                else
                {
		    GtkDialogFlags flags10 = GTK_DIALOG_DESTROY_WITH_PARENT;
        	    GtkWidget *dialog11 = gtk_message_dialog_new (NULL,
                             	          flags10,
                                          GTK_MESSAGE_ERROR,
                                          GTK_BUTTONS_CLOSE,
                                          "File already exists with same name !");
                                
        	    gtk_dialog_run (GTK_DIALOG (dialog11));
        	    gtk_widget_destroy (dialog11); 
                }
            }
    }
    else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gds->SelectRadioButton1)))
    {
	try
        {
            if (IsEmailvalid(emailIDtxt))
            {
                if (!(File_Path.empty()))
                {
		    FILENAME = strdup(File_Path.c_str());
                }
		FROM = "tamalika.bhowmick@wipro.com";
		TO = strdup(emailIDtxt.c_str());
		BODY = strdup(emailBody.c_str());

		int sendStatus = Send(FROM, TO, BODY);
                if(sendStatus == CURLE_OK)
		{
			GtkDialogFlags flags11 = GTK_DIALOG_DESTROY_WITH_PARENT;
			GtkWidget *dialog12 = gtk_message_dialog_new (NULL,
		                     	      flags11,
		                              GTK_MESSAGE_INFO,
		                              GTK_BUTTONS_OK,
		                              "Email Sent Successfully ! ");
		                        
			gtk_dialog_run (GTK_DIALOG (dialog12));
			gtk_widget_destroy (dialog12); 
		}
		else
			cout<<"\nEmail error\n";

                CLearShareFileContent(gds);
            }
            else
            {
		GtkDialogFlags flags12 = GTK_DIALOG_DESTROY_WITH_PARENT;
        	GtkWidget *dialog13 = gtk_message_dialog_new (NULL,
                             	      flags12,
                                      GTK_MESSAGE_ERROR,
                                      GTK_BUTTONS_CLOSE,
                                      "Please enter a valid address !");
                                
        	gtk_dialog_run (GTK_DIALOG (dialog13));
        	gtk_widget_destroy (dialog13); 
		
		gtk_widget_grab_focus (gds->TypeEmailText);
            }
        }
        catch (...)
        {
            GtkDialogFlags flags13 = GTK_DIALOG_DESTROY_WITH_PARENT;
            GtkWidget *dialog14 = gtk_message_dialog_new (NULL,
                             	  flags13,
                                  GTK_MESSAGE_ERROR,
                                  GTK_BUTTONS_CLOSE,
                                  "Exception Occurred !");
                                
            gtk_dialog_run (GTK_DIALOG (dialog14));
            gtk_widget_destroy (dialog14);
        }
    }

}


static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    gtk_main_quit ();
    return FALSE;
}

static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}


int main(int   argc, char *argv[])
{
	LoadData();	

	GtkWidget *UIWindow;
 	GtkWidget *MainBox;

	GtkWidget *CmdBox;
	GtkWidget *InOutBox;

	GtkWidget *CreateUserBtn;
	GtkWidget *SearchUserBtn;
	GtkWidget *ShareFileBtn;

	GtkWidget *CreateFirstNameBox;
	GtkWidget *CreateLastNameBox;
	GtkWidget *CreateEmailBox;
	GtkWidget *CellBox;
	GtkWidget *SubmitBtnCreateBox;
	
	GtkWidget* createfirstnamelabel;
	GtkWidget* createlastnamelabel;
	GtkWidget* createemaillabel;
	GtkWidget* celllabel;

	GtkWidget *SubmitBtnCreate;

	GtkWidget *SearchFirstNameBox;
	GtkWidget *SearchLastNameBox;
	GtkWidget *SearchEmailBox;
	GtkWidget *SubmitBtnSearchBox;
	GtkWidget *SearchResultTitleBox;
	
	GtkWidget* searchfirstnamelabel;
	GtkWidget* searchlastnamelabel;
	GtkWidget* searchemaillabel;

	GtkWidget *SubmitBtnSearch;

	GtkWidget *RadioButtonBox;
	GtkWidget *SendToFolderBox;
	GtkWidget *EmailToBox;
	GtkWidget *BrowseFileBox;
	GtkWidget *EmailBodyBox;
	GtkWidget *SendButtonBox;

	GtkWidget *sendtofolderlabel;
	GtkWidget *emailtolabel;
	//GtkWidget *selectemaillabel;
	GtkWidget *emailbodylabel;

	GtkWidget *BrowseFileBtn;
	GtkWidget *SendFileBtn;


	//adding background color
	GdkRGBA color;

	GtkCssProvider *provider; 
	GdkDisplay *display;
	GdkScreen *screen;
 

	color.red = 1;
	color.green = 0;
	color.blue = 0;
	color.alpha = 0;
       
	GDS* gds = g_slice_new(GDS);
	gtk_init (&argc, &argv);

 
        UIWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(UIWindow), "Digital Sign" );

    	gtk_widget_set_size_request( UIWindow, 1000, 700 );
    
	g_signal_connect (UIWindow, "delete-event",
		      G_CALLBACK (delete_event), NULL);
 
	g_signal_connect (UIWindow, "destroy",
		      G_CALLBACK (destroy), NULL);

	provider = gtk_css_provider_new ();
	display = gdk_display_get_default ();
	screen = gdk_display_get_default_screen (display);

	gtk_style_context_add_provider_for_screen (screen,
                                 GTK_STYLE_PROVIDER (provider),
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        gtk_css_provider_load_from_data (GTK_CSS_PROVIDER(provider),
                                   " #CmdBox {\n"
				   "   background-color: rgba (50,50,50,0.5);\n" 
                                   "}\n"
				   " #InOutBox {\n"
				   "   background-color: rgba (120,120,120,0.5);\n" 
                                   "}\n", -1, NULL);
  	g_object_unref (provider);

	/* Sets the border width of the window. */
	gtk_container_set_border_width (GTK_CONTAINER (UIWindow), 10);


	CmdBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	InOutBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
        gtk_widget_set_name(CmdBox, "CmdBox" );
	gtk_widget_set_name(InOutBox, "InOutBox" );

	gtk_widget_set_size_request( InOutBox, 800, 700 );

	gtk_container_set_border_width( GTK_CONTAINER(CmdBox), 2);
	gtk_container_set_border_width( GTK_CONTAINER(InOutBox), 2); 

	gds->CreateUserBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gds->SearchUserBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
	gds->ShareFileBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

	CreateUserBtn = gtk_button_new_with_label("Create User");
	gtk_widget_set_size_request(CreateUserBtn, 80, 30);
	g_signal_connect(CreateUserBtn, "clicked", G_CALLBACK(CreateUserCallback), gds);

	SearchUserBtn = gtk_button_new_with_label("Search User");
	gtk_widget_set_size_request(SearchUserBtn, 80, 30);
	g_signal_connect(SearchUserBtn, "clicked", G_CALLBACK(SearchUserCallback), gds);

	ShareFileBtn = gtk_button_new_with_label("Share File");
	gtk_widget_set_size_request(ShareFileBtn, 80, 30);
	g_signal_connect(ShareFileBtn, "clicked", G_CALLBACK(ShareFileCallback), gds);

	SubmitBtnCreate = gtk_button_new_with_label("CREATE");
	gtk_widget_set_size_request(SubmitBtnCreate, 80, 30);
	g_signal_connect(SubmitBtnCreate, "clicked", G_CALLBACK(CreateSubmitCallback), gds);

	SubmitBtnSearch = gtk_button_new_with_label("SEARCH");
	gtk_widget_set_size_request(SubmitBtnSearch, 80, 30);
	g_signal_connect(SubmitBtnSearch, "clicked", G_CALLBACK(SearchSubmitCallback), gds);

	BrowseFileBtn = gtk_button_new_with_label("Browse File");
	gtk_widget_set_size_request(BrowseFileBtn, 80, 30);
	g_signal_connect(BrowseFileBtn, "clicked", G_CALLBACK(BrowseFileCallback), gds);

	SendFileBtn = gtk_button_new_with_label("SEND");
	gtk_widget_set_size_request(SendFileBtn, 80, 30);
	g_signal_connect(SendFileBtn, "clicked", G_CALLBACK(SendFileCallback), gds);

	
	gds->SelectRadioButton1 = gtk_radio_button_new_with_label (NULL,
                                 "Send Email");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gds->SelectRadioButton1), FALSE);

	gds->SelectRadioButton2 = gtk_radio_button_new_with_label (NULL,
                                 "Send To Network Folder");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gds->SelectRadioButton2), FALSE);
	
	gtk_radio_button_join_group(GTK_RADIO_BUTTON (gds->SelectRadioButton2), GTK_RADIO_BUTTON (gds->SelectRadioButton1));
	//g_signal_connect(gds->SelectRadioButton2, "toggled", G_CALLBACK(RadioButtonToggled), gds);

	CreateFirstNameBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	CreateLastNameBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	CreateEmailBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	CellBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	SubmitBtnCreateBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	SearchFirstNameBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	SearchLastNameBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	SearchEmailBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	SubmitBtnSearchBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	SearchResultTitleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	gds->SearchResultBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	RadioButtonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	SendToFolderBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	EmailToBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	BrowseFileBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	//gds->SelectEmailBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	EmailBodyBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	SendButtonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);

	gtk_box_pack_start(GTK_BOX(CmdBox), CreateUserBtn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(CmdBox), SearchUserBtn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(CmdBox), ShareFileBtn, FALSE, FALSE, 0);


	createfirstnamelabel = gtk_label_new("First Name : ");
	createlastnamelabel = gtk_label_new("Last Name : ");
	createemaillabel = gtk_label_new("Email ID :   ");
	celllabel = gtk_label_new("Cell :      "  );

	gds->CreateFirstNameText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->CreateFirstNameText),0);

	gds->CreateLastNameText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->CreateLastNameText),0);

	gds->CreateEmailText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->CreateEmailText),0);

	gds->CellText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->CellText),0);	

	
	searchfirstnamelabel = gtk_label_new("First Name : ");
	searchlastnamelabel = gtk_label_new("Last Name : ");
	searchemaillabel = gtk_label_new("Email ID :   ");
	gds->searchresultlabel = gtk_label_new("Search Results");

	gds->SearchFirstNameText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->SearchFirstNameText),0);

	gds->SearchLastNameText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->SearchLastNameText),0);

	gds->SearchEmailText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->SearchEmailText),0);

	gds->resultShowList = gtk_list_box_new ();
 	gtk_list_box_set_selection_mode (GTK_LIST_BOX (gds->resultShowList), GTK_SELECTION_SINGLE);   //GTK_SELECTION_SINGLE	
	gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX (gds->resultShowList), FALSE);

	sendtofolderlabel = gtk_label_new("Send To Folder");
	emailtolabel = gtk_label_new("Send To");
	//selectemaillabel = gtk_label_new("Selected Email:");
	emailbodylabel = gtk_label_new("Email Body");

	gds->TypeEmailText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->TypeEmailText),0);

	//gds->EmailToText = gtk_entry_new ();
	//gtk_entry_set_max_length (GTK_ENTRY (gds->EmailToText),0);
	//gds->EmailToText = gtk_frame_new (NULL);
	//gtk_frame_set_label_align (GTK_FRAME (gds->EmailToText), 0.8, 0.5);
	//gtk_frame_set_shadow_type(GTK_FRAME (gds->EmailToText), GTK_SHADOW_ETCHED_IN);
	
	gds->BrowseFileText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->BrowseFileText),0);

	gds->EmailBodyText = gtk_entry_new ();
        gtk_entry_set_max_length (GTK_ENTRY (gds->EmailBodyText),0);

	gtk_box_pack_start( GTK_BOX (CreateFirstNameBox), createfirstnamelabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (CreateFirstNameBox), gds->CreateFirstNameText, FALSE, FALSE, 5);
	
	gtk_box_pack_start( GTK_BOX (CreateLastNameBox), createlastnamelabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (CreateLastNameBox), gds->CreateLastNameText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (CreateEmailBox), createemaillabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (CreateEmailBox), gds->CreateEmailText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (CellBox), celllabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (CellBox), gds->CellText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (SubmitBtnCreateBox), SubmitBtnCreate, FALSE, FALSE, 10);
	

	gtk_box_pack_start( GTK_BOX (gds->CreateUserBox), CreateFirstNameBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->CreateUserBox), CreateLastNameBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->CreateUserBox), CreateEmailBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->CreateUserBox), CellBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->CreateUserBox), SubmitBtnCreateBox, FALSE, FALSE, 10);
	


	gtk_box_pack_start( GTK_BOX (SearchFirstNameBox), searchfirstnamelabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (SearchFirstNameBox), gds->SearchFirstNameText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (SearchLastNameBox), searchlastnamelabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (SearchLastNameBox), gds->SearchLastNameText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (SearchEmailBox), searchemaillabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (SearchEmailBox), gds->SearchEmailText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (SubmitBtnSearchBox), SubmitBtnSearch, FALSE, FALSE, 10);
	
	gtk_box_pack_start( GTK_BOX (SearchResultTitleBox), gds->searchresultlabel, FALSE, FALSE, 5);
	gtk_box_pack_start( GTK_BOX (gds->SearchResultBox), gds->resultShowList, FALSE, FALSE, 10);
	


	gtk_box_pack_start( GTK_BOX (gds->SearchUserBox), SearchFirstNameBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->SearchUserBox), SearchLastNameBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->SearchUserBox), SearchEmailBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->SearchUserBox), SubmitBtnSearchBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->SearchUserBox), SearchResultTitleBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->SearchUserBox), gds->SearchResultBox, FALSE, FALSE, 10);

	

	gtk_box_pack_start( GTK_BOX (RadioButtonBox), gds->SelectRadioButton1, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (RadioButtonBox), gds->SelectRadioButton2, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (SendToFolderBox), sendtofolderlabel, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (EmailToBox), emailtolabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (EmailToBox), gds->TypeEmailText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (BrowseFileBox), BrowseFileBtn, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (BrowseFileBox), gds->BrowseFileText, FALSE, FALSE, 5);

	//gtk_box_pack_start( GTK_BOX (gds->SelectEmailBox), selectemaillabel, FALSE, FALSE, 10);
	//gtk_box_pack_start( GTK_BOX (gds->SelectEmailBox), gds->EmailToText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (EmailBodyBox), emailbodylabel, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (EmailBodyBox), gds->EmailBodyText, FALSE, FALSE, 5);

	gtk_box_pack_start( GTK_BOX (SendButtonBox), SendFileBtn, FALSE, FALSE, 5);
	

	gtk_box_pack_start( GTK_BOX (gds->ShareFileBox), RadioButtonBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->ShareFileBox), SendToFolderBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->ShareFileBox), EmailToBox, FALSE, FALSE, 10);
	//gtk_box_pack_start( GTK_BOX (gds->ShareFileBox), gds->SelectEmailBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->ShareFileBox), BrowseFileBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->ShareFileBox), EmailBodyBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (gds->ShareFileBox), SendButtonBox, FALSE, FALSE, 10);

	gtk_box_pack_start( GTK_BOX (InOutBox), gds->CreateUserBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (InOutBox), gds->SearchUserBox, FALSE, FALSE, 10);
	gtk_box_pack_start( GTK_BOX (InOutBox), gds->ShareFileBox, FALSE, FALSE, 10);


	MainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
	gtk_box_pack_start(GTK_BOX(MainBox), CmdBox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(MainBox), InOutBox, FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(UIWindow), MainBox);
        
	gtk_widget_show_all(UIWindow);
	
	gtk_widget_hide(gds->SearchUserBox);
	gtk_widget_hide(gds->ShareFileBox);
        

	gtk_main();

	return 0;
}
