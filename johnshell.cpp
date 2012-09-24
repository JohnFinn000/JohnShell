/*
 * =====================================================================================
 *
 *       Filename:  johnshell.cpp
 *
 *    Description:  A simple shell program
 *
 *        Version:  1.0
 *        Created:  02/23/2012 07:36:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  John Finn (JF), johnvincentfinn@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include "johnshell.h"

extern char **environ;
int main( int argc, char *argv[]) {
	printf("%s", WELCOME);

	Shell *shell = new Shell();
	shell->run();
}

// Class Shell
Shell::Shell() {
	environment = new Hashtable();
	history     = new History();
	alias       = new Hashtable();

	// Initialize the environment table
	for( int i = 0; environ[i]!=NULL;++i ) {
		int str_num = strcspn(environ[i], "=");
		char *hash_string = (char*) malloc(30);
		strncpy(hash_string, environ[i], str_num);
		hash_string[str_num] = '\0';

		char *data = strstr(environ[i], "=");
		++data;
		environment->put_data( hash_string, data );
	}

	// Store the most important variables
	char *pwd_temp  = (char*) environment->get_data( "PWD"  );
	char *user_temp = (char*) environment->get_data( "USER" );
	char *home_temp = (char*) environment->get_data( "HOME" );
	char name_temp[64]; 
	gethostname( name_temp, 64 );

	if( pwd_temp  ) {
		char *pwd_temp  = (char*) "";
	}
	if( user_temp ) {
		char *user_temp = (char*) "";
	}
	if( home_temp ) {
		char *home_temp = (char*) "";
	}

	current_path 	= new string( pwd_temp  );
	user 		= new string( user_temp );
	home 		= new string( home_temp );
	machine 	= new string( name_temp );

	// Read and execute commands from a configuration file
	FILE * config_file = fopen("johnshell_config", "r");
	if( config_file ) {
		while( true ) {
			char *ret = scan( config_file );
			if( ret == NULL ) {
				break;
			}
			execute_command( get_command( ret, 0, false, false ) );
		}
	} else {
		printf("no config file to read\n");
	}
}

int Shell::run() {
	while( true ) {
		printf("%s@%s:%s$ ", user->c_str(), machine->c_str(), current_path->c_str() );
		char *ret = scan( stdin );

		if( ret ) {
			int val = execute_command( get_command( ret, 0, false, false ) );
			switch( val ) {
			case 0:
				// Everything executed fine add the command to the history
				if( strcmp( ret, "!!" ) != 0 ) {
					history->add( ret );
				}
				break;
			case 1:
			{
				// It's time to logout
				rusage *u = new( rusage );
				getrusage( RUSAGE_SELF, u );
				if( u ) {
					printf("user CPU time: %ld, system CPU time: %ld \n", 
						u->ru_utime.tv_sec, u->ru_stime.tv_sec );
				} else {
					printf("rusage failure\n");
				}
				return 0;
			}
			case 2:
			default:
				// Something has gone wrong
				cout << "an error has occurred\n";
				break;
			}

		}
	}
	return 0;
}

command_list *Shell::get_command( char *str, int code, bool end_quote, bool alias_flag ) {
	if( code == DUP_STR) {
		// duplicate the string
		// this is done when get_command is run first 
		// so that the original string isn't edited
		char *new_str = new( char[strlen(str)+1] );
		strcpy( new_str, str );
		str  = new_str;
		code = NORMAL;
	}

	// spaces are replaced with end string
	// if a quote is seen it is replaced with an end string
	// spaces are ignored until the end quote is reached
	char *next_str;
	switch( end_quote ) {
	case true:
		next_str = strchr( str, '"' );
		if( next_str ) {
			end_quote = !end_quote;
			break;
		}
	case false:
		next_str = strchr( str, ' ' );
	}

	char *quote = strchr( str, '\"' );	
	if( quote ) {
		if( !next_str || quote < next_str ) { 
			next_str = quote;
			end_quote = !end_quote;
		}
	}

	if( next_str != NULL ) { 
		// replace each space with a string terminator.
		*next_str = '\0';
	} else {
		// the real end string has been found and the next code should be terminate
		code = TERMINATE;
	}

	// Chain together each command in a linked list
	if( strlen(str) > 0 ) {
		command_list * new_command = new( command_list );
		command_list * iter = new_command;
		new_command->next = NULL;
		new_command->command = string( str );	
		size_t tilde_loc = new_command->command.find('~');
		if( tilde_loc != string::npos ) {
			new_command->command.replace( tilde_loc, 1, *home );
		}
		new_command->type = T_COMMAND;

		// Check the alias table to see if the command is really an alias
		char *data = NULL;
		if( !alias_flag ) {
			data = (char*) alias->get_data( str );
		}
	
		if( data != NULL ) {
			new_command->type = T_ALIAS;
			new_command->next = get_command( data, DUP_STR, end_quote, true );
			while( iter->next != NULL ) {
				iter = iter->next;
			}
		}

		if( code != TERMINATE ) {
			iter->next = get_command( next_str+1, NORMAL, end_quote, alias_flag );	
		}
	
		return new_command;
	}

	if( code != TERMINATE ) {
		return get_command( next_str+1, NORMAL, end_quote, alias_flag );	
	}

	return NULL;
}

int Shell::execute_command( command_list *cl ) {
	bool backround_flag = false;
	command_list *command;
	command_list *pipe_command;
	command = cl;
	int pipe_count = 0;
	int command_count[2];
	command_count[0] = 0;
	command_count[1] = 0;
	char *redirect = NULL;
	bool redirect_io = false;

	while( command ) {
		char *str = (char*) command->command.c_str();
		if( !strcmp(str, "history") ) {
			command = command->next;
			if( !command ) {
				history->list();
				return 0;
			}
			int   val  = atoi(command->command.c_str());
			char *hist = (char*) history->get(val).c_str();
			if( hist ) {
				cout << hist << "\n";
				execute_command( get_command( hist, 0, false, false ) );
				return 0;
			} else {
				return 2;
			}
		} else if( !strcmp(str, "!!") ) {
			char *hist = (char*) history->get().c_str();
			if( hist ) {
				cout << hist << "\n";
				execute_command( get_command( hist, 0, false, false ) );
				return 0;
			} else {
				return 2;
			}
			return 0;
		} else if( *str == '!' ) {
			++str;
			int val = atoi( str );
			char *hist = (char*) history->get(val).c_str();
			if( hist ) {
				cout << hist << "\n";
				execute_command( get_command( hist, 0, false, false ) );
				return 0;
			} else {
				return 2;
			}
			return 0;
		} else if( !strcmp(str, "chdir") ) {
			command = command->next;
			if( !command ) {
				chdir( home->c_str() );
				current_path = new string( home->c_str() );
				return 0;
			}
			chdir( command->command.c_str() );
			current_path = new string( get_current_dir_name() );
			return 0;
		} else if( !strcmp(str, "which") ) {
		} else if( !strcmp(str, "kill") ) {
			command = command->next;
			if( !command ) {
				return 0;
			}
			int pid = atoi(command->command.c_str());
			printf("killing %d\n", pid);
			kill(pid, SIGINT);
		} else if( !strcmp(str, "logout") ) {
			return 1;
		} else if( !strcmp(str, "alias") ) {
			command = command->next;
			if( !command ) {
				alias->list_data();
				return 0;
			}
			char *hash_string = (char*) command->command.c_str();
			command = command->next;
			if( !command ) {
				printf("error\n");
				return 2;
			}
			char *data_string = (char*) command->command.c_str();
			alias->put_data(hash_string, data_string);
			return 0;
		} else if( !strcmp(str, "unalias") ) {
			command = command->next;
			if( !command ) {
				printf("error\n");
				return 0;
			}
			char *hash_string = (char*) command->command.c_str();
			alias->remove_data(hash_string);
			return 0;
		} else if( !strcmp(str, "&") ) {
			backround_flag = true;
			break;
		} else if( !strcmp(str, "|") ) {
			command->type = T_PIPE;
			if( pipe_count == 0 ) {
				pipe_command = command->next;
				command = pipe_command;
				++pipe_count;
				continue;
			} else {
				break;
			}
		} else if( !strcmp(str, "<") ) {
			command = command->next;
			if( !command ) {
				printf("error\n");
				return 0;
			}
			redirect_io = false;
			redirect = (char*) command->command.c_str();
			break;
		} else if( !strcmp(str, ">") ) {
			command = command->next;
			if( !command ) {
				printf("error\n");
				return 0;
			}
			redirect_io = true;
			redirect = (char*) command->command.c_str();
			break;
		}

		++command_count[pipe_count];
		command = command->next;
	}


	char *arg[command_count[0]+1];
	char *arg2[command_count[1]+1];

	// turn the linked list of commands into an array
	command = cl;
	for( int i = 0; i < command_count[0]; ++i ) {
		if( !command ) {
			arg[i] = (char*) NULL;
		} else if( command->type != T_ALIAS ) {
			arg[i] = (char*) command->command.c_str();
		} else {
			--i;
			--command_count[0];
		}
		command = command->next;
	}
	arg[command_count[0]] = (char*) NULL;

	if( pipe_count != 0 ) {

		// turn the linked list of commands into an array
		command = pipe_command;
		for( int i = 0; i < command_count[1]; ++i ) {
			if( !command ) {
				arg2[i] = (char*) NULL;
			} else if( command->type != T_ALIAS ) {
				arg2[i] = (char*) command->command.c_str();
			} else {
				--i;
				--command_count[1];
			}
			command = command->next;
		}
		arg2[command_count[1]] = (char*) NULL;

		int	pip[2];
		pid_t	pid1, pid2;
		if( pipe(pip) ) {
			printf("pipe fail\n");
			return 2;
		}
		switch( pid1 = fork() ) {
		case -1:
			printf("fork failed\n");
			return 2;
			break;
		case 0:
			if( dup2(pip[WRITE], STDOUT_FILENO)==-1 ) {
				printf("failure\n");
				return 2;
			}
			close(pip[WRITE]);
			close(pip[READ]);
			if( execvp( arg[0], arg ) == -1 ) {
				printf("failure\n");
				return 2;
			}
		}
		switch( pid2 = fork() ) {
		case -1:
			printf("fork failed\n");
			break;
		case 0:
			if( dup2(pip[READ], STDIN_FILENO)==-1 ) {
				printf("failure\n");
				return 2;
			}
			close(pip[WRITE]);
			close(pip[READ]);
 			if( execvp( arg2[0], arg2 ) == -1 ) {
				printf("failure\n");
				return 2;
			}
		}
		close(pip[WRITE]);
		close(pip[READ]);
		if( !backround_flag ) {
			if( waitpid(pid1, &pip[READ], 0) != pid1 || waitpid(pid2, &pip[WRITE], 0) != pid2 ) {
				printf("wait fail\n");
				return 2;
			}
		}
	} else {
		int pid;
		if((pid = fork()) == 0) {
			int fd;
			if( redirect ) {
				printf("redirect");
				if( redirect_io ) {
					fd = open( redirect, O_WRONLY | O_CREAT | O_TRUNC, 0644 );
					dup2( fd, STDOUT_FILENO );
				} else {
					fd = open( redirect, O_RDONLY );
					dup2( fd, STDIN_FILENO );
				}
				close( fd );
			}
			if( execvp( arg[0] , arg ) == -1 ) {
				printf("failed\n");
				return 2;
			}
		}
		if( !backround_flag ) {
			while( wait((int*) NULL) != pid );
		}
	}

	return 0;
}

char* Shell::scan(FILE * stream) {
	char current_char;
	char temp[161];
	int count = 0;
	char *result;
		
	while( count < 160 ) {
		current_char = getc(stream);
		if( current_char == EOF || current_char == '\n' ) {
			break;
		} 
		temp[count] = current_char;
		++count;
	}
	if( count == 0 ) {
		return NULL;
	}

	temp[count] = '\0';
	result = (char*) malloc( count );
	strcpy( result, temp );
	return result;
}

// Class History
History::History() {
	command_count = 0;
	history = NULL;
	begin = NULL;

}

string History::get( int num ) {
	if( num > command_count ) {
		printf("not that many commands are in the history\n");
		return "ERROR";
	}
	history_list *iter;
	if( num > command_count / 2 ) {
		iter = history;
		while( iter != NULL ) {
			if( iter->num == num ) {
				return iter->command;
			}
			iter = iter->next;
		}
	} else {
		iter = begin;
		while( iter != NULL ) {
			if( iter->num == num ) {
				return iter->command;
			}
			iter = iter->prev;
		}
	}
	return "ERROR";
}

string History::get( ) {
	if( command_count > 0 ) {
		return history->command;
	} else {
		printf("ERROR");
		return "ERROR";
	}
}

int History::add( string command ) {
	if( !history || history->command.compare(command) != 0 ) {
		history_list *new_element;
		new_element = new(history_list);
		new_element->num = command_count;
		new_element->command = command;
		new_element->next = history;
		if( history ) { 
			history->prev = new_element; 
		}
		new_element->prev = NULL;
		history = new_element;
		if( !begin ) {
			begin = new_element;
		}
		++command_count;
	}
	return 0;
}

void History::list( ) {
	history_list *iter;
	iter = begin;
	while( iter ) {
		printf("%d, %s\n", iter->num, iter->command.c_str());
		iter = iter->prev;
	}
	printf("--End of history--\n");
}



// Class Hashtable
Hashtable::Hashtable() {
	for( int i = 0; i < 53; ++i ) {
		table[i] = NULL;
	}

}

int Hashtable::hash( string str ) {
	int hash = 0;	
	string::iterator iter;

	for( iter=str.begin(); iter < str.end(); ++iter ) {
		hash += *iter;
	}
	hash %= 53;	
	return hash;
}

int Hashtable::put_data( string hash_string, void * data_ptr ) {
	bucket_list *new_node;
	new_node = new(bucket_list);
	new_node->hash_string = hash_string;
	new_node->data = data_ptr;
	new_node->next = NULL;
	int hashnum = hash( hash_string );	

	bucket_list * list = table[hashnum];
	if( list != NULL ) {
		if( !list->hash_string.compare(hash_string) ) {
			list->data = data_ptr;
		}
		while( list->next != NULL ) {
			list = list->next;
			if( !list->hash_string.compare(hash_string) ) {
				list->data = data_ptr;
			}
		} 
		list->next = new_node;
	} else {
		table[hashnum] = new_node;
		
	}
	return 0;
}

void *Hashtable::get_data( string hash_string ) {
	int hashnum = hash( hash_string );
	bucket_list *list = table[hashnum];
	while( list != NULL ) {
		if( !list->hash_string.compare(hash_string) ) {
			return list->data;	
		}
		list = list->next;
	}
	return NULL;
	
}

void Hashtable::remove_data( string hash_string ) {
	int hashnum = hash( hash_string );
	bucket_list *list = table[hashnum];
	bucket_list *prev = table[hashnum];
	while( list != NULL ) {
		if( !list->hash_string.compare(hash_string) ) {
			if( list == table[hashnum] ) {
				table[hashnum] = table[hashnum]->next;
			}
			prev->next = list->next;
			return;
		}
		prev = list;
		list = list->next;
	}
	return;
	
}


void Hashtable::list_data() {
	bucket_list *list;
	for( int i = 0; i < 52; ++i ) {
		list = table[i];
		while( list != NULL ) {
			cout << list->hash_string << "='"<< (char*) list->data << "'\n";	
			list = list->next;
		}
	}
	return;
	
}

