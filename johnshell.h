/*
 * =====================================================================================
 *
 *       Filename:  johnshell.h
 *
 *    Description:  A simple shell program 
 *
 *        Version:  1.0
 *        Created:  02/23/2012 07:33:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  John Finn (JF), johnvincentfinn@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

/* #####   HEADER FILE INCLUDES   ################################################### */

#include <iostream>
#include <string.h>
#include <stdio.h>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

/* #####   MACROS   ################################################################# */

// Change this to change the message displayed on startup
#define WELCOME \
"Welcome to John shell written by John Finn\n\
configuration is read from file \'johnshell_config\'\n\
aliases are set like so: alias ls \"ls -l\"\n\
the last history command can be run using !!\n\
history commands can also be run by typing history followed by\n\
their command number ex) \"history 5\" will execute the command labeled 5\n\
in the history.\n\
The tilde as notation for \\home\\user was left out because I kept getting\n\
confused about if I was in my shell or the normal shell.\n"

#define READ 	0
#define WRITE 	1

#define DUP_STR    0
#define TERMINATE  1
#define NORMAL	   3

// Forward Declarations
class Shell;
class History;
class Hashtable;

/* #####   TYPE DEFINITIONS   ####################################################### */

enum command_type {
	T_COMMAND,
	T_ALIAS,
	T_PIPE
};

struct command_list {
	command_type type;
	string command;
	command_list *next;
};

/* #####   PROTOTYPES   ############################################################# */

/*
 * =====================================================================================
 *        Class:  Shell
 *  Description:  This manages all of the shell data
 * =====================================================================================
 */
class Shell {
public:

	Shell();

	/*
	 *--------------------------------------------------------------------------------------
	 *       Class:  Shell
	 *      Method:  Shell :: run
	 * Description:  This begins to run the terminal and should be called after
	 * 		instantiating a Shell object.
	 *--------------------------------------------------------------------------------------
	 */
	int run();

private:
	/*
	 *--------------------------------------------------------------------------------------
	 *       Class:  Shell
	 *      Method:  Shell :: get_command
	 * Description:  This will break down a string into single word commands and inject
	 * 		the corresponding alias data where needed. on success a linked list 
	 * 		of commands is returned.
	 *--------------------------------------------------------------------------------------
	 */
	command_list *get_command( char *str, int code, bool end_quote, bool alias_flag );

	/*
	 *--------------------------------------------------------------------------------------
	 *       Class:  Shell
	 *      Method:  Shell :: execute_command
	 * Description:  Executes the command list that is returned by get_command.
	 *--------------------------------------------------------------------------------------
	 */
	int execute_command( command_list *cl );

	/*
	 *--------------------------------------------------------------------------------------
	 *       Class:  Shell
	 *      Method:  Shell :: scan
	 * Description:  Scans in a single line of characters.
	 *--------------------------------------------------------------------------------------
	 */
	char *scan( FILE * stream );
	
	string *current_path;
	string *user;
	string *machine;
	string *home;	
		
	History   *history;
	Hashtable *environment;
	Hashtable *alias;

};


/*
 * =====================================================================================
 *        Class:  History
 *  Description:  Tracks and manages past commands that have been executed
 * =====================================================================================
 */
class History {
public:
	struct history_list {
		int    num;
		string command;
		history_list *next, *prev;
	};

	History();


	/*
	 *--------------------------------------------------------------------------------------
	 *       Class:  History
	 *      Method:  History :: get
	 * Description:  returns the command that was executed at num. If no num is given
	 * 		the most recent command is given.
	 *--------------------------------------------------------------------------------------
	 */
	string get( int num );
	string get( );


	/*
	 *--------------------------------------------------------------------------------------
	 *       Class:  History
	 *      Method:  History :: add
	 * Description:  inserts a new history_list element into the linked list.
	 *--------------------------------------------------------------------------------------
	 */
	int  add( string command );
	void list();

private:
	history_list *history;
	history_list *begin;
	int command_count;
};


/*
 * =====================================================================================
 *        Class:  Hashtable
 *  Description:  Allows fast access to data using a hash string
 * =====================================================================================
 */
class Hashtable {
public:
	struct bucket_list {
		string hash_string;
		void  *data;
		bucket_list *next;
	};


	Hashtable();

	/*
	 *--------------------------------------------------------------------------------------
	 *       Class:  Hashtable
	 *      Method:  Hashtable :: put_data
	 * Description:  adds a new element into the hashtable. If an element with the
	 * 		hash_string already exists it is overwritten with the new data.
	 *--------------------------------------------------------------------------------------
	 */
	int put_data( string hash_string, void * data_ptr );

	/*
	 *--------------------------------------------------------------------------------------
	 *       Class:  Hashtable
	 *      Method:  Hashtable :: get_data
	 * Description:  returns the data that is associated with hash_string.
	 *--------------------------------------------------------------------------------------
	 */
	void *get_data(   string hash_string );
	void remove_data( string hash_string );
	void list_data();

private:
	bucket_list *table[53];

	int hash( string str );
};

