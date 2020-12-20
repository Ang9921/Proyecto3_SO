#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

/* Comandos
 *  vs -f	Imprime el disco
 *  cd	 	Desplazar direcorios
 *  mkdir	Crear directorio
 *  rmdir	Eliminar directorio 
 *  mvdir	Renombrar directorio
 *  mkfil	Crear archivo
 *  rmfil	Eliminar archivo
 *  mvfil	Renombrar archivo
 *  szfil	Cambiar attributos
 *  exit    Salir
 */

int debug = 1;	

int do_root (char *name, char *size, char *content);
int do_print(char *name, char *flag, char *content);
int do_print_vs(char *name, char *size, char *content);
int do_cd(char *name, char *size, char *content);
int do_mkdir(char *name, char *size, char *content);
int do_rmdir(char *name, char *size, char *content);
int do_mvdir(char *name, char *size, char *content);
int do_mkfil(char *name, char *size, char *content);
int do_rmfil(char *name, char *size, char *content);
int do_mvfil(char *name, char *size, char *content);
int do_wrfil(char *name, char *size, char *content);
int do_rdfil(char *name, char *size, char *content);
int do_szfil(char *name, char *size, char *content);
int do_exit (char *name, char *size, char *content);

struct action {
  char *cmd;					
  int (*action)(char *name, char *size, char* ext);	
} table[] = {
    { "ls", do_print },
	{ "vs", do_print_vs },
    { "cd", do_cd },
    { "mkdir", do_mkdir },
    { "rmdir", do_rmdir },
    { "mvdir", do_mvdir },
    { "mkfil", do_mkfil },
    { "rmfil", do_rmfil },
    { "mvfil", do_mvfil },
	{ "wrfil", do_wrfil },
	{ "rdfil", do_rdfil },
    { "szfil", do_szfil },
    { "exit" , do_exit  },
    { NULL, NULL }
};


void printing(char *name);
void printing_ls(char *name, char *flag);
void print_descriptor ( );
void parse(char *buf, int *argc, char *argv[]);
int allocate_block (char *name, bool directory ) ;
void unallocate_block ( int offset );
int find_block ( char* name, bool directory );

int add_descriptor ( char * name );
int edit_descriptor ( int free_index, bool free, int name_index, char * name );
int edit_descriptor_name (int index, char* new_name,char *content);
int edit_descriptor_name2 (int index, char* new_name,char *content);
int add_directory( char * name );
int remove_directory( char * name );
int edit_directory ( char * name,  char*subitem_name, char *new_name, bool name_change, bool directory );
int add_file( char * name, int size, char * content );
int edit_file ( char * name, int size, char *new_name, char *content);
int edit_file2 ( char * name, char *size, int content, int new_name);
int remove_file (char* name);
int edit_directory_subitem (char* name, char* sub_name, char* new_sub_name,char *content);
int edit_directory_subitem2 (char* name, char* sub_name, char* new_sub_name,char *content);

void print_directory ( char *name);
char * get_directory_name ( char*name );
char * get_directory_top_level ( char*name);
char * get_directory_subitem ( char*name, int subitem_index, char * subitem );
int get_directory_subitem_count ( char*name);

char * get_file_name ( char*name );
char * get_file_top_level ( char*name);
int get_file_size( char*name);
void print_file ( char *name);


#define LINESIZE 128
#define DISK_PARTITION 4000000
#define BLOCK_SIZE 5000
#define BLOCKS 4000000/5000
#define MAX_STRING_LENGTH 20
#define MAX_DATE_LENGTH 100
#define MAX_CONTENT_LENGTH 1000
#define MAX_FILE_DATA_BLOCKS (BLOCK_SIZE-64*59) //Hard-coded as of now
#define MAX_SUBDIRECTORIES  (BLOCK_SIZE - 136)/MAX_STRING_LENGTH
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"

typedef struct {
	char directory[MAX_STRING_LENGTH];
	int directory_index;
	char parent[MAX_STRING_LENGTH];
	int parent_index;
} working_directory;


typedef struct dir_type {
	char name[MAX_STRING_LENGTH];		
	char top_level[MAX_STRING_LENGTH];
	char (*subitem)[MAX_STRING_LENGTH];
	bool subitem_type[MAX_SUBDIRECTORIES];
	int subitem_count;
	char date[MAX_DATE_LENGTH];	
	struct dir_type *next;
	char content[MAX_STRING_LENGTH];
} dir_type;


typedef struct file_type {
	char name[MAX_STRING_LENGTH];		
	char top_level[MAX_STRING_LENGTH];	
	int data_block_index[MAX_FILE_DATA_BLOCKS];
	char content[MAX_CONTENT_LENGTH];
	int data_block_count;
	int size;
	char date[MAX_DATE_LENGTH];	
	struct file_type *next;
} file_type;

typedef struct {
	bool free[BLOCKS];
	bool directory[BLOCKS];
	char (*name)[MAX_STRING_LENGTH];
	char content[MAX_STRING_LENGTH];
} descriptor_block;

char *disk;
char dir[100]= "";
working_directory current;
bool disk_allocated = false; 

int main(int argc, char *argv[])
{
	do_root("","","");
	(void)argc;
	(void)*argv;
    char in[LINESIZE];
    char *cmd, *fnm, *fsz, *fex;
    char dummy[] = "";

    int n;
    char *a[LINESIZE];
   
   while (fgets(in, LINESIZE, stdin) != NULL)
    {
	  
      parse(in, &n, a);

      cmd = (n > 0) ? a[0] : dummy;
      fnm = (n > 1) ? a[1] : dummy;
      fsz = (n > 2) ? a[2] : dummy;

      if (n == 0) continue;	

      int found = 0;
     
      for (struct action *ptr = table; ptr->cmd != NULL; ptr++){  
            if (strcmp(ptr->cmd, cmd) == 0)
                {
                    found = 1;
                    
                    int ret = (ptr->action)(fnm, fsz, fex);
                    if (ret == -1)
                        { printf("  %s %s %s %s: failed\n", cmd, fnm, fsz, fex); }
                    break;
                }
	    }
      if (!found) { printf("command not found: %s\n", cmd );}
	  printf(ANSI_COLOR_BLUE"%s/%s$ "ANSI_COLOR_RESET, current.parent,current.directory);
    }

  return 0;
}


void parse(char *buf, int *argc, char *argv[])
{
  char *delim;          
  int count = 0;        

  char whsp[] = " \t\n\v\f\r";          

  while (1)                             
    {
      buf += strspn(buf, whsp);         
      delim = strpbrk(buf, whsp);       
      if (delim == NULL)               
        { break; }
      argv[count++] = buf;              
      *delim = '\0';                    
      buf = delim + 1;                  
    }
  argv[count] = NULL;

  *argc = count;

  return;
}

/*------------------------------------INICIAR DISCO--------------------------------------------*/

int do_root(char *name, char *size, char *content)
{
	(void)*name;
	(void)*size;
	if ( disk_allocated == true )
		return 0;

	disk = (char*)malloc ( DISK_PARTITION );
		if ( debug ) printf("Asignando [%d] Bytes de memoria al disco\n", DISK_PARTITION );

	add_descriptor("descriptor");
	add_directory("root");
	
	strcpy(current.directory, "root");
	current.directory_index = 3;
	strcpy(current.parent, "" );
	current.parent_index = -1;
	
	if ( debug ) printf("Disco listo!\n");
	disk_allocated = true;
	
 	return 0;
}

/*-----------------------------------------VISUALIZACION---------------------------------------*/

int do_print(char *name, char *flag, char *content)
{
	(void)*name;
	(void)*flag;
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	printing_ls(current.directory, name);
	
	return 0;
}


int do_print_vs(char *name, char *size, char *content)
{
	(void)*name;
	(void)*size;
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	printing(current.directory);
	
	return 0;
}

/*-------------------------------------------DIRECTORIOS-------------------------------------*/

int do_cd(char *name, char *size, char *content)
{
	(void)*size;
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	
	if ( strcmp(name, ".." ) == 0 ) {

		if ( strcmp(current.directory, "root") == 0 )
			return 0;
		strcpy ( current.directory, current.parent );	
		strcpy (current.parent, get_directory_top_level( current.parent) );
			//if ( debug ) printf ("\t[%s] Current Directory is now [%s], Parent Directory is [%s]\n", __func__, current.directory, current.parent);
		return 0;
	}
	else
	{	
		char tmp[20];
		if ( (strcmp(get_directory_subitem(current.directory, -1, name), "-1") == 0) && strcmp( current.parent, name ) != 0 ) {
			//if ( debug ) printf( "\t\t\t[%s] Cannot Change to Directory [%s]\n", __func__, name );
			if (!debug ) printf( "%s: %s: No such file or directory\n", "cd", name );
			return 0;
		}
	
		strcpy( tmp, get_directory_name(name));
		if ( strcmp(tmp, "") == 0 )
			return 0;		
		if ( strcmp( tmp, name ) == 0 ) {
			//Adjust the working_directory struct
			strcpy ( current.directory, tmp);
			strcpy(current.parent, get_directory_top_level(name) );
				//if ( debug ) printf ("\t[%s] Current Directory is now [%s], Parent Directory is [%s]\n", __func__, current.directory, current.parent);
				char *temp = current.directory;			
			return 1;
		}
		return -1;
	}
  return 0;
}

int do_mkdir(char *name, char *size, char *content)
{
	(void)*size;
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}	
	if ( get_directory_subitem(current.directory, -1, name) == 0  ) {
			//if ( debug ) printf( "\t\t\t[%s] Cannot Make Directory [%s]\n", __func__, name );
			if (!debug ) printf( "%s: cannot create directory '%s': Folder exists\n", "mkdir", name );
			return 0;
		}

	//if ( debug ) printf("\t[%s] Creating Directory: [%s]\n", __func__, name );
	if ( add_directory( name ) != 0 ) {
		if (!debug ) printf("%s: missing operand\n", "mkdir");
		return 0;
	}
	edit_directory( current.directory, name, NULL, false, true );
		//if ( debug ) printf("\t[%s] Updating Parents Subitem content\n", __func__ );
		
	if ( debug ) printf(" Created Successfully\n");
	if( debug ) print_directory(name);
	
  	return 0;
}

int do_rmdir(char *name, char *size, char *content)
{
	(void)*size;
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
		
	if ( strcmp(name,"") == 0 ) {
		if ( debug ) printf("\t[%s] Invalid Command\n", __func__ );
		if (!debug ) printf("%s: missing operand\n", "rmdir");
		return 0;
	}
	
	if( (strcmp(name, ".") == 0) || (strcmp(name, "..") == 0) ) {
		if ( debug ) printf("\t[%s] Invalid command [%s] Will not remove directory\n", __func__, name);
		if (!debug ) printf( "%s: %s: No such file or directory\n", "rmdir", name );
		return 0;
	}
	
	if ( strcmp(get_directory_subitem(current.directory, -1, name), "-1") == 0 ) {
		if ( debug ) printf( "\t[%s] Cannot Remove Directory [%s]\n", __func__, name );
		if (!debug ) printf( "%s: %s: No such file or directory\n", "rmdir", name );
		return 0;
	}
	
	dir_type *folder = malloc ( BLOCK_SIZE );
	int block_index = find_block(name, true);
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE );

	dir_type *top_folder = malloc ( BLOCK_SIZE );
	int top_block_index = find_block(folder->top_level, true);
	memcpy( disk + block_index*BLOCK_SIZE, folder, BLOCK_SIZE );
	memcpy( top_folder, disk + top_block_index*BLOCK_SIZE, BLOCK_SIZE );

	char subitem_name[MAX_STRING_LENGTH]; 
	const int subcnt = top_folder->subitem_count; 
	int j;
	int k=0;
	for(j = 0; j<subcnt; j++) {
		strcpy(subitem_name, top_folder->subitem[j]);
		if (strcmp(subitem_name, name) != 0)
		{
			strcpy(top_folder->subitem[k],subitem_name);
			k++;
		}
	}
	strcpy(top_folder->subitem[k], "");

	top_folder->subitem_count--;
	memcpy( disk + top_block_index*BLOCK_SIZE, top_folder, BLOCK_SIZE );
	free(top_folder);
	//if ( debug ) printf("\t[%s] Removing Directory: [%s]\n", __func__, name );
	if( remove_directory( name ) == -1 ) {
		return 0;
	}
	
	if (debug) printf("\t[%s] Directory Removed Successfully\n", __func__);
	return 0;
}

int do_mvdir(char *name, char *new_name, char *content) 
{
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	//if ( debug ) printf("\t[%s] Renaming Directory: [%s]\n", __func__, name );
	if( edit_directory( name, "", new_name, true, true ) == -1 ) {
		if (!debug ) printf( "%s: cannot rename file or directory '%s'\n", "mvdir", name );
		return 0;
	}

	if (debug) printf( "Directory Renamed Successfully: [%s]\n", new_name );
	if (debug) print_directory(new_name); 
	return 0;
}

/*------------------------------------------ARCHIVOS--------------------------------------*/

int do_mkfil(char *name, char *size, char *content)
{
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	
	//if ( debug ) printf("\t[%s] Creating File: [%s], with Size: [%s]\n", __func__, name, size );
	if ( get_directory_subitem(current.directory, -1, name) == 0  ) {
			if ( debug ) printf( "\t\t\t[%s] Cannot make file [%s], a file or directory [%s] already exists content [%s]\n", __func__, name, name, content);
			if (!debug ) printf( "%s: cannot create file '%s': File exists\n", "mkfil", name );
			return 0;
		}
	
	if ( add_file ( name, atoi(size), content) != 0 )
		return 0;
  	edit_directory( current.directory, name, NULL, false, false);
  		//if ( debug ) printf("\t[%s] Updating Parents Subitem content\n", __func__ );
  	
  	if ( debug ) print_file(name);
  	return 0; 
	  }
int do_rmfil(char *name, char *size, char *content)
{
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	
	(void)*size;
	//if ( debug ) printf("\t[%s] Removing File: [%s]\n", __func__, name);

	if ( get_directory_subitem(current.directory, -1, name) == 0  ) {
			remove_file(name);
			return 0;
		}
	else{ 
		if ( debug ) printf( "\t\t\t[%s] Cannot remove file [%s], it does not exist in this directory\n", __func__, name );
		if (!debug ) printf( "%s: %s: No such file or directory\n", "rmfil", name );
		return 0;
	}
}

int do_mvfil(char *name, char *size, char *content)
{
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	
	if ( debug ) printf("\t[%s] Renaming File: [%s], to: [%s]\n", __func__, name, size );

	if ( get_directory_subitem(current.directory, -1, size) == 0  ) {
			if ( debug ) printf( "\t\t\t[%s] Cannot rename file [%s], a file or directory [%s] already exists [%s]\n", __func__, name, size,content );
			if (!debug ) printf( "%s: cannot rename file or directory '%s'\n", "mvfil", name ); 
			return 0;
		}

	int er = edit_file( name, 0, size,content);
	
	if (er == -1) return -1;
	if (debug) print_file(size);
}

int do_wrfil(char *name, char *size, char *content)
{
		if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	
	//if ( debug ) printf("\t[%s] Adding content [%s] to File: [%s]\n", __func__, size, name );
	if (remove_file(name) != -1)  do_mkfil(name, size, size);

	else {
		if ( debug ) printf("\t[%s] File: [%s] does not exist. Cannot add content\n", __func__, name);
		if (!debug ) printf( "%s: cannot resize '%s': No such file or directory\n", "szfil", name );
	}

	return 0;
}

int do_rdfil(char *name, char *size, char *content)
{
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}	
	//if ( debug ) printf("\t[%s] Renaming File: [%s], to: [%s]\n", __func__, name, size );
	//If it returns 0, there is a subitem with that name already
	if ( get_directory_subitem(current.directory, -1, size) == 0  ) {
			if ( debug ) printf( "\t\t\t[%s] Cannot rename file [%s], a file or directory [%s] already exists [%s]\n", __func__, name, size,content );
			if (!debug ) printf( "%s: cannot rename file or directory '%s'\n", "mvfil", name ); 
			return 0;
		}
	file_type *file = malloc ( BLOCK_SIZE);
	
	//Find the block in memory where this file is written	
	int block_index = find_block(name, false);
	if ( block_index == -1 )  {
		if ( debug ) printf("\t\t\t[%s] File [%s] not found\n", __func__, name);
		return -1;
	}
	//if (debug) printf("\t\t[%s] File [%s] Found At Memory Block [%d]\n", __func__, name, block_index);

	memcpy( file, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	printf("[%s]\n",file->content);
	free(file);
	return 0;
}
int do_szfil(char *name, char *size, char *content)
{
	if ( disk_allocated == false ) {
		printf("Error: Disk not allocated\n");
		return 0;
	}
	
	//if ( debug ) printf("\t[%s] Resizing File: [%s], to: [%s]\n", __func__, name, size );
	if (remove_file(name) != -1)  do_mkfil(name, size,content);

	else {
		if ( debug ) printf("\t[%s] File: [%s] does not exist. Cannot resize.\n", __func__, name);
		if (!debug ) printf( "%s: cannot resize '%s': No such file or directory\n", "szfil", name );
	}

	return 0;
}

/*---------------------------------------------SALIR-----------------------------------*/

int do_exit(char *name, char *size, char *content)
{
	(void)*name;
	(void)*size;
	if (debug) printf("\t[%s] Exiting\n", __func__);
	exit(0);
	return 0;
}

/******************************* 		FUNCIONES AUXILIARES *****************************/

void printing(char *name) {
	dir_type *folder = malloc (BLOCK_SIZE);
	int block_index = find_block(name, true);

	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
		
	printf("%s:\n", folder->name);
	for( int i = 0; i < folder->subitem_count; i++ ) {
		 printf("\t%s\n", folder->subitem[i]);
	}
	for( int i = 0; i < folder->subitem_count; i++ ) {
		if( folder->subitem_type[i] == true ) {
			printing(folder->subitem[i]);
		}
	}
}

void printing_ls(char *name, char *flag) {
	dir_type *folder = malloc (BLOCK_SIZE);
	file_type *file = malloc( BLOCK_SIZE);
	int block_index = find_block(name, true);
	char** vector, **vector2;
	vector = malloc(folder->subitem_count*sizeof(char*));
	vector2 = malloc(folder->subitem_count*sizeof(char*));
	char *temp;

	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
		
	printf("%s:\n", folder->name);

	
	if(strcmp(flag, "-a") == 0 || strcmp(flag, "") == 0){
		for( int i = 0; i < folder->subitem_count; i++ ) {
			vector[i] = folder->subitem[i];		
		}
		for(int i=0; i< folder->subitem_count-1; i++){
			for(int j=i+1; j< folder->subitem_count; j++){
				if(strcmp(vector[i], vector[j]) > 0){
					temp = vector[i];
					vector[i] = vector[j];
					vector[j] = temp;
			}
		}
	}

	for(int i=0; i<folder->subitem_count; i++){
		if(find_block(vector[i], true) != -1){
			printf(ANSI_COLOR_YELLOW"\t%s\n"ANSI_COLOR_RESET, vector[i]);
		}else{
			printf("\t%s\n", vector[i]);
		}
	}
	}
	else if(strcmp(flag, "-m") == 0){
		for( int i = 0; i < folder->subitem_count; i++ ) {
		if(find_block(folder->subitem[i], true) != -1){
			printf(ANSI_COLOR_YELLOW"\t%s\n"ANSI_COLOR_RESET, folder->subitem[i]);
		}else{
			printf("\t%s\n", folder->subitem[i]);
		}
	}
		
	}else{
		printf("Command flag is invalid\n");
		return; 
	}

}

void print_descriptor ( ) {
	descriptor_block *descriptor = malloc( BLOCK_SIZE*2 );

	memcpy ( descriptor, disk, BLOCK_SIZE*2 );

	printf("Disk Descriptor Free Table:\n");
	
	for ( int i = 0; i < BLOCKS ; i++ ) {
		printf("\tIndex %d : %d\n", i, descriptor->free[i]);
	}
	
	free(descriptor);
}

int allocate_block ( char *name, bool directory ) { 

	descriptor_block *descriptor = malloc( BLOCK_SIZE*2);
	memcpy ( descriptor, disk, BLOCK_SIZE*2 );

	//if ( debug ) printf("\t\t\t[%s] Finding Free Memory Block in the Descriptor\n", __func__ );
	for ( int i = 0; i < BLOCKS; i++ ) {
		 if ( descriptor->free[i] ) {
			descriptor->free[i] = false;
			descriptor->directory[i] = directory;
			strcpy(descriptor->name[i], name);
			memcpy(disk, descriptor, BLOCK_SIZE*2);
				//if ( debug ) printf("\t\t\t[%s] Allocated [%s] at Memory Block [%d]\n", __func__, name, i );

			free(descriptor);
			return i; 
		}
	}
	free(descriptor);
	if ( debug ) printf("\t\t\t[%s] No Free Space Found: Returning -1\n", __func__);
	return -1;
}

void unallocate_block ( int offset ) { 
	descriptor_block *descriptor = malloc( BLOCK_SIZE*2 );
	
	memcpy ( descriptor, disk, BLOCK_SIZE*2 );
	if ( debug ) printf("\t\t\t[%s] Unallocating Memory Block [%d]\n", __func__, offset );
	descriptor->free[offset] = true;
	strcpy( descriptor->name[offset], "" );

	memcpy ( disk, descriptor, BLOCK_SIZE*2 );	
	
	free(descriptor);
}

int find_block ( char *name, bool directory ) {
	descriptor_block *descriptor = malloc( BLOCK_SIZE*2 );
	memcpy ( descriptor, disk, BLOCK_SIZE*2 );
	for ( int i = 0; i < BLOCKS; i++ ) {
		if ( strcmp(descriptor->name[i], name) ==0 ){
			if ( descriptor->directory[i] == directory ) {
				if ( debug )
				free(descriptor);
				return i;
			}
		}
	}
	
	free(descriptor);
	return -1;
}

int add_descriptor ( char * name ) {
	descriptor_block *descriptor = malloc( BLOCK_SIZE*2);
		//if ( debug ) printf("\t\t[%s] Allocating Space for Descriptor Block\n", __func__);
	descriptor->name = malloc ( sizeof*name*BLOCKS );
		//if ( debug ) printf("\t\t[%s] Allocating Space for Descriptor's Name Member\n", __func__);
	//if ( debug ) printf("\t\t[%s] Initializing Descriptor to Have All of Memory Available\n", __func__);
	for (int i = 0; i < BLOCKS; i++ ) {
		descriptor->free[i] = true;
		descriptor->directory[i] = false;
	}
	int limit = (int)(sizeof(descriptor_block)/BLOCK_SIZE) + 1;
	
	//if ( debug ) printf("\t\t[%s] Updating Descriptor to Show that first [%d] Memory Blocks Are Taken\n", __func__, limit+1);
	for ( int i = 0; i < limit; i ++ ) {
		descriptor->free[i]= false;
	}	
	strcpy(descriptor->name[0], "descriptor"); 	
	memcpy ( disk, descriptor, (BLOCK_SIZE*(limit+1)));

	return 0;	
}
int edit_descriptor ( int free_index, bool free, int name_index, char * name ) {

	descriptor_block *descriptor = malloc( BLOCK_SIZE*2 );
	memcpy ( descriptor, disk, BLOCK_SIZE*2 );
	if ( free_index > 0 ) {
		descriptor->free[free_index] = free;
			//if ( debug ) printf("\t\t[%s] Descriptor Free Member now shows Memory Block [%d] is [%s]\n", __func__, free_index, free == true ? "Free": "Used");
	}
	if ( name_index > 0 ) {
		strcpy(descriptor->name[name_index], name );
			//if ( debug ) printf("\t\t[%s] Descriptor Name Member now shows Memory Block [%d] has Name [%s]\n", __func__, name_index, name);	
	}
	memcpy(disk, descriptor, BLOCK_SIZE*2);
	return 0;
}

int edit_descriptor_name (int index, char* new_name,char *content)
{
	descriptor_block *descriptor = malloc( BLOCK_SIZE*2 );
	memcpy ( descriptor, disk, BLOCK_SIZE*2 );
	strcpy(descriptor->name[index], new_name);
	memcpy(disk, descriptor, BLOCK_SIZE*2);
	free(descriptor);
	return 0;
}
int edit_descriptor_name2 (int index, char* content,char *new_name)
{
	descriptor_block *descriptor = malloc( BLOCK_SIZE*2 );
	memcpy ( descriptor, disk, BLOCK_SIZE*2 );
	strcpy(descriptor->name[index], content);
	memcpy(disk, descriptor, BLOCK_SIZE*2);
	free(descriptor);
	return 0;
}

/*******************************************AUXILIARES DIRECTORIOS*****************************/
int add_directory( char * name ) {
	
	if ( strcmp(name,"") == 0 ) {
		if ( debug ) printf("\t\t[%s] Invalid Command\n", __func__ );
		return -1;
	}
	dir_type *folder = malloc ( BLOCK_SIZE);
		//if ( debug ) printf("\t\t[%s] Allocating Space for New Folder\n", __func__);
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char s[64];
	assert(strftime(s, sizeof(s), "%c", tm));
	strcpy(folder->name, name);					
	strcpy(folder->top_level, current.directory);
	strcpy( folder->date, s);
	folder->subitem = malloc ( sizeof*(folder->subitem)*MAX_SUBDIRECTORIES);
	folder->subitem_count = 0;	
	int index = allocate_block(name, true);
		//if ( debug ) printf("\t\t[%s] Assigning New Folder to Memory Block [%d]\n", __func__, index);
	memcpy( disk + index*BLOCK_SIZE, folder, BLOCK_SIZE);
	
	//if ( debug ) printf("\t\t[%s] Folder [%s] Successfully Added\n", __func__, name);
	free(folder);
	return 0;
}
int remove_directory( char * name ) {
	
	dir_type *folder = malloc (BLOCK_SIZE);
	int block_index = find_block(name, true);
	if( block_index == -1 ) {
		if ( debug ) printf("\t\t[%s] Directory [%s] does not exist in the current folder [%s]\n", __func__, name, current.directory);
		return -1;
	}

	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE );
	for( int i = 0; i < folder->subitem_count; i++ ) {
		if( folder->subitem_type[i] == true ) {
			remove_directory(folder->subitem[i]);
		}
		else {
			remove_file(folder->subitem[i]);
		}
	}
	unallocate_block(block_index);
	free(folder);
	
	return 0;
}

int edit_directory (char * name,  char*subitem_name, char *new_name, bool name_change, bool directory ) {
	
	if( strcmp(name,"") == 0 ) {
		if( debug ) printf("\t\t[%s] Invalid Command\n", __func__ );
		return -1;
	}
	dir_type *folder = malloc ( BLOCK_SIZE);
	int block_index = find_block(name, true);
	if( block_index == -1 ) {
		if ( debug ) printf("\t\t[%s] Directory [%s] does not exist\n", __func__, name);
		return -1;
	}
		//if ( debug ) printf("\t\t[%s] Folder [%s] Found At Memory Block [%d]\n", __func__, name, block_index);
	
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);

	if ( strcmp(subitem_name, "") != 0 ) {
		
		if ( !name_change ) { 
			//if ( debug ) printf("\t\t[%s] Added Subitem [%s] at Subitem index [%d] to directory [%s]\n", __func__, subitem_name, folder->subitem_count, folder->name );
			strcpy (folder->subitem[folder->subitem_count], subitem_name );
			folder->subitem_type[folder->subitem_count] = directory;
			folder->subitem_count++;
				//if ( debug ) printf("\t\t[%s] Folder [%s] Now Has [%d] Subitems\n", __func__, name, folder->subitem_count);
			memcpy( disk + block_index*BLOCK_SIZE, folder, BLOCK_SIZE);
			
			free(folder);
			return 0;
		}
		else {
			for ( int i =0; i < folder->subitem_count; i++ ) {
				if ( strcmp(folder->subitem[i], subitem_name) == 0 ) {
					strcpy( folder->subitem[i], new_name);
						//if ( debug ) printf("\t\t[%s] Edited Subitem [%s] to [%s] at Subitem index [%d] for directory [%s]\n", __func__, subitem_name, new_name, i, folder->name );	

					memcpy( disk + block_index*BLOCK_SIZE, folder, BLOCK_SIZE);
					free(folder);
					return 0;
				}
			}

			//if ( debug ) printf("\t\t[%s] Subitem Does Not Exist in Directory [%s]\n", __func__, folder->name );
			free(folder);
			return -1;
		}
	}
	else {
		int block_index2 = find_block(new_name, true);
		if( block_index2 != -1 ) {
			if ( debug ) printf("\t\t[%s] Directory [%s] already exists. Choose a different name\n", __func__, new_name);
			return -1;
		}	
		strcpy(folder->name, new_name );
			if ( debug ) printf("\t\t[%s] Folder [%s] Now Has Name [%s]\n", __func__, name, folder->name);
		
		memcpy( disk + block_index*BLOCK_SIZE, folder, BLOCK_SIZE);

		edit_descriptor(-1, false, block_index, new_name );
			if ( debug ) printf("\t\t[%s] Updated Descriptor's Name Member\n", __func__);
			if ( debug ) print_directory(folder->name);
		edit_directory(folder->top_level, name, new_name, true, true );
			if ( debug ) printf("\t\t[%s] Updated Parents Subitem Name\n", __func__);
		
		int child_index;
		for ( int i = 0; i < folder->subitem_count; i++) {
			file_type *child_file = malloc ( BLOCK_SIZE);
			dir_type *child_folder = malloc ( BLOCK_SIZE);
			
			child_index = find_block ( folder->subitem[i], folder->subitem_type);
			if ( folder->subitem_type[i] ) {
				//if type == folder
				memcpy( child_folder, disk + child_index*BLOCK_SIZE, BLOCK_SIZE);
				strcpy( child_folder->top_level, new_name );
				
				memcpy( disk + child_index*BLOCK_SIZE, child_folder, BLOCK_SIZE);
				free ( child_folder );
				free ( child_file );
			}
			else {
				memcpy( child_file, disk + child_index*BLOCK_SIZE, BLOCK_SIZE);
				strcpy( child_file->top_level, new_name );
			
				memcpy( disk + child_index*BLOCK_SIZE, child_file, BLOCK_SIZE);	
				free ( child_folder );
				free ( child_file );
			} 
		}
			
		free(folder);
		return 0;
	}
		
	free ( folder );
}

/*-------------------------------------------------AUXILIARES ARCHIVOS-------------------------------*/
int add_file( char * name, int size, char * content) {
	char subname[20];
	
	if ( size < 0 || strcmp(name,"") == 0 ) {
		if ( debug ) printf("\t\t[%s] Invalid command\n", __func__);
		if (!debug ) printf("%s: missing operand\n", "mkfil");
		return 1;
	}
	file_type *file = malloc ( BLOCK_SIZE );
		//if ( debug ) printf("\t\t[%s] Allocating Space for New File\n", __func__);
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char s[64];
	assert(strftime(s, sizeof(s), "%c", tm));
	strcpy( file->name, name);	
	strcpy( file->content, content);	
	strcpy ( file->top_level, current.directory );
	file->size = size;
	strcpy( file->date, s);		
	file->data_block_count = 0;
		//if ( debug ) printf("\t\t[%s] Initializing File Members\n", __func__);
	int index = allocate_block(name, false);
	//if ( debug ) printf("\t\t[%s] Allocating [%d] Data Blocks in Memory for File Data\n", __func__, (int)size/BLOCK_SIZE);
	for ( int i = 0; i < size/BLOCK_SIZE + 1; i++ ) {
		sprintf(subname, "%s->%d", name, i);
		file->data_block_index[i] = allocate_block(subname, false);
		file->data_block_count++;
	}  
	memcpy( disk + index*BLOCK_SIZE, file, BLOCK_SIZE);
	
	//if ( debug ) printf("\t\t[%s] File [%s] Successfully Added\n", __func__, name);
	
	free(file);
	return 0;
}

int remove_file (char* name)
{
	if (strcmp(name,"") == 0 ) {
		if ( debug ) printf("\t\t[%s] Invalid command\n", __func__);
		if (!debug ) printf("%s: missing operand\n", "rmfil");
		return 1;
	}

	file_type *file = malloc ( BLOCK_SIZE);
	dir_type *folder = malloc ( BLOCK_SIZE);
	
	int file_index = find_block(name, false);
	if ( file_index == -1 )  {
		if ( debug ) printf("\t\t\t[%s] File [%s] not found\n", __func__, name);
		return -1;
	}
	
	//if ( debug ) printf("\t\t[%s] File [%s] Found At Memory Block [%d]\n", __func__, name, file_index);
	
	memcpy( file, disk + file_index*BLOCK_SIZE, BLOCK_SIZE);

	int folder_index = find_block(file->top_level, true);
	
	//if ( debug ) printf("\t\t[%s] Folder [%s] Found At Memory Block [%d]\n", __func__, name, folder_index);
	memcpy( folder, disk + folder_index*BLOCK_SIZE, BLOCK_SIZE);
	char subitem_name[MAX_STRING_LENGTH]; 
	const int subcnt = folder->subitem_count;
	int j;
	int k=0;
	for(j = 0; j<subcnt; j++)
		{
			strcpy(subitem_name, folder->subitem[j]);
			if (strcmp(subitem_name, name) != 0) 
			{
				strcpy(folder->subitem[k],subitem_name);
				k++;
			}
		}
	strcpy(folder->subitem[k], "");
	folder->subitem_count--;

	memcpy(disk + folder_index*BLOCK_SIZE, folder, BLOCK_SIZE);
	int i = 0;
	while(file->data_block_count != 0)
	{
		unallocate_block(file->data_block_index[i]);
		file->data_block_count--;
		i++;
	}
	
	unallocate_block(file_index);
	
	free(folder);
	free(file);
	return 0;
}
int edit_file ( char * name, int size, char *new_name, char *content) {
	file_type *file = malloc ( BLOCK_SIZE);
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char s[64];
	assert(strftime(s, sizeof(s), "%c", tm));	
	int block_index = find_block(name, false);
	if ( block_index == -1 )  {
		if ( debug ) printf("\t\t\t[%s] File [%s] not found\n", __func__, name);
		return -1;
	}
	//if ( debug ) printf("\t\t[%s] File [%s] Found At Memory Block [%d]\n", __func__, name, block_index);

	memcpy( file, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	
	if ( size > 0 ) { 
		strcpy( file->date, s);
		//if ( debug ) printf("\t\t[%s] File [%s] Now Has Size [%d]\n", __func__, name, size);
		free(file);
		return 0;
	}
	else {		  
		char top_level[MAX_STRING_LENGTH];
		strcpy(top_level, get_file_top_level(name));
		file->size = size;
		edit_directory_subitem(top_level, name, new_name, content); 
		edit_descriptor_name(block_index, new_name, content); 

		strcpy(file->name, new_name );
		memcpy( disk + block_index*BLOCK_SIZE, file, BLOCK_SIZE);	

		//if ( debug ) printf("\t\t\t[%s] File [%s] Now Has Name [%s] and Date [%s]\n", __func__, name, file->name, file->date);

		free(file);
		return 0;
	}
}

int edit_file2 ( char * name, char* content, int size, int new_name) {
	file_type *file = malloc ( BLOCK_SIZE);
	
	int block_index = find_block(name, false);
	if ( block_index == -1 )  {
		if ( debug ) printf("\t\t\t[%s] File [%s] not found\n", __func__, name);
		return -1;
	}
	//if ( debug ) printf("\t\t[%s] File [%s] Found At Memory Block [%d]\n", __func__, name, block_index);

	memcpy( file, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	strcpy(file->content, content);
	 //printf("\t\t\t[%s] File [%s] Now Has Name [%s] and content [%s] \n", __func__, name, file->name, file->content);
	 memcpy( disk + block_index*BLOCK_SIZE, file, BLOCK_SIZE);	
	free(file);
	return 1;
}
char * get_directory_name ( char*name ) {
	dir_type *folder = malloc ( BLOCK_SIZE);
	char *tmp = malloc(sizeof(char)*MAX_STRING_LENGTH); 
		
	int block_index = find_block(name, true);
	if ( block_index == -1 )  {
		if ( debug ) printf("\t\t\t[%s] Folder [%s] not found\n", __func__, name);
		strcpy ( tmp, "");
		return tmp;
	}
	
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	
	strcpy( tmp, folder->name);
		//if ( debug ) printf("\t\t\t[%s] Name [%s] found for [%s] folder\n", __func__, tmp, name );
		
	free ( folder );
	return tmp;
}

char * get_directory_top_level ( char*name) {
	dir_type *folder = malloc ( BLOCK_SIZE);
	char *tmp = malloc(sizeof(char)*MAX_STRING_LENGTH); 
	int block_index = find_block(name, true);
	if ( block_index == -1 )  {
		if ( debug ) printf("\t\t\t[%s] Folder [%s] not found\n", __func__,name);
		strcpy ( tmp, "");
		return tmp;
	}
	
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	
	strcpy( tmp, folder->top_level);
		//if ( debug ) printf("\t\t\t[%s] top_level [%s] found for [%s] folder\n", __func__, tmp, name );
	
	free ( folder );
	return tmp;
}

char * get_directory_subitem ( char*name, int subitem_index, char*subitem_name ) {
	dir_type *folder = malloc ( BLOCK_SIZE);
	char *tmp = malloc(sizeof(char)*MAX_STRING_LENGTH); 

	int block_index = find_block(name, true);
	if ( block_index == -1 ) {
		if ( debug ) printf("\t\t\t[%s] Folder [%s] not found\n", __func__, name);
		strcpy ( tmp, "");
		return tmp;
	}
	
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	
	if ( subitem_index >= 0 ) { 
		strcpy( tmp, folder->subitem[subitem_index]);
		if ( debug ) printf("\t\t\t[%s] subitem[%d] = [%s] for [%s] folder\n", __func__, subitem_index, tmp, name );
		free(folder);
		return tmp;
	}
	else { 			 
		for ( int i =0; i < folder->subitem_count; i ++ ) {
			if ( strcmp( folder->subitem[i], subitem_name ) == 0 ) {
				//if ( debug ) printf( "\t\t\t[%s] Found [%s] as a Subitem of Directory [%s]\n", __func__, subitem_name, name );
				return "0";
			}
		}
		//if ( debug ) printf( "\t\t\t[%s] Did Not Find [%s] as a Subitem of Directory [%s]\n", __func__, subitem_name, name );
		free ( folder );
		return "-1";
	}
	free ( folder );
	return tmp;
}

int edit_directory_subitem (char* name, char* sub_name, char* new_sub_name,char *content)
{
	dir_type *folder = malloc ( BLOCK_SIZE);
	int block_index = find_block(name, true);
	if ( block_index == -1 ) {
		if ( debug ) printf("\t\t\t[%s] Folder [%s] not found\n", __func__, name);
	}
	
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	const int cnt = folder->subitem_count;	
	int i;
	for (i=0; i < cnt; i++)
	{
		if (strcmp(folder->subitem[i], sub_name) == 0)
		{
			strcpy(folder->subitem[i], new_sub_name);
			//if (debug) printf("\t\t\t[%s] Edited subitem in %s from %s to %s\n", __func__, folder->name, sub_name, folder->subitem[i]);

			memcpy(disk + block_index*BLOCK_SIZE ,folder, BLOCK_SIZE);
			free(folder);
			return i;
		}
	}

	free(folder);
	return -1;
}

int edit_directory_subitem2 (char* name, char* sub_name, char* content,char *new_sub_name)
{
	dir_type *folder = malloc ( BLOCK_SIZE);
	int block_index = find_block(name, true);
	if ( block_index == -1 ) {
		if ( debug ) printf("\t\t\t[%s] Folder [%s] not found\n", __func__, name);
	}
	
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);

	const int cnt = folder->subitem_count;	
	int i;
	for (i=0; i < cnt; i++)
	{
		if (strcmp(folder->subitem[i], sub_name) == 0)
		{
			strcpy(folder->subitem[i], new_sub_name);
			//if (debug) printf("\t\t\t[%s] Edited subitem in %s from %s to %s\n", __func__, folder->name, sub_name, folder->subitem[i]);

			memcpy(disk + block_index*BLOCK_SIZE ,folder, BLOCK_SIZE);
			free(folder);
			return i;
		}
	}

	free(folder);
	return -1;
}

int get_directory_subitem_count( char*name) {
	
	dir_type *folder = malloc ( BLOCK_SIZE);
	int tmp;

	int block_index = find_block(name, true);
	if ( block_index == -1 ) {
		if ( debug ) printf("\t\t\t[%s] Folder [%s] not found\n", __func__, name);
		return -1;
	}
	
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
 	
 	tmp = folder->subitem_count;
		//if ( debug ) printf("\t\t\t[%s] subitem_count [%d] found for [%s] folder\n", __func__, folder->subitem_count, name );
	
	free ( folder );
	return tmp;
}
char * get_file_name ( char*name ) {
	file_type *file = malloc ( BLOCK_SIZE);
	char *tmp = malloc(sizeof(char)*MAX_STRING_LENGTH); 
	int block_index = find_block(name, false);
	if ( block_index == -1 ) {
		if ( debug ) printf("\t\t\t[%s] File [%s] not found\n", __func__, name);
		strcpy ( tmp, "");
		return tmp;
	}
				
	memcpy( file, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	
	strcpy( tmp, file->name);
		//if ( debug ) printf("\t\t\t[%s] Name [%s] found for [%s] file\n", __func__, tmp, name );
		
	free ( file );
	return tmp;
}

char * get_file_top_level ( char*name) {
	file_type *file = malloc ( BLOCK_SIZE);
	char *tmp = malloc(sizeof(char)*MAX_STRING_LENGTH); 

	int block_index = find_block(name, false);
	if ( block_index == -1 ) {
		if ( debug ) printf("\t\t\t[%s] File [%s] not found\n", __func__, name);
		strcpy ( tmp, "");
		return tmp;
	}
		
	memcpy( file, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
	
	strcpy( tmp, file->top_level);
		if ( debug ) printf("\t\t\t[%s] top_level [%s] found for [%s] file\n", __func__, tmp, name );
	
	free ( file );
	return tmp;
}

int get_file_size( char*name) {
	
	file_type *file = malloc ( BLOCK_SIZE);
	int tmp;

	int block_index = find_block(name, false);
	if ( block_index == -1 ) {
		if ( debug ) printf("\t\t\t[%s] File [%s] not found\n", __func__, name);
		return -1;
	}
		
	memcpy( file, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);
 	
 	tmp = file->size;
		if ( debug ) printf("\t\t\t[%s] size of [%d] found for [%s] file\n", __func__, tmp, name );
	
	free ( file );
	return tmp;
}

/********************************* VISUALIZACION ********************************/
void print_directory ( char *name) {
	dir_type *folder = malloc( BLOCK_SIZE);
	int block_index = find_block(name, true);
	memcpy( folder, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char s[64];
	assert(strftime(s, sizeof(s), "%c", tm));
	
	printf("	-----------------------------\n");
	printf("	New Folder Attributes:\n\n\tname = %s\n\tDate and hour= %s\n\t", folder->name, s);
	for (int i = 0; i < folder->subitem_count; i++) {
		printf( "%s ", folder->subitem[i]);
	}
	printf("	-----------------------------\n");
	
	free(folder);
}

void print_file ( char *name) {
	file_type *file = malloc( BLOCK_SIZE);
	int block_index = find_block(name, false);
	memcpy( file, disk + block_index*BLOCK_SIZE, BLOCK_SIZE);

	time_t t = time(NULL);
    	struct tm *tm = localtime(&t);
    	char s[64];
    	assert(strftime(s, sizeof(s), "%c", tm));
	
	printf("	-----------------------------\n");
	printf("	New File Attributes:\n\n\tname = %s\n\tDate and hour = %s\n\tFile Size = %d\n\tLast Modified= %d\nFile Content = %s\n", file->name, s, file->size, file->data_block_count,file->content);
	printf("	-----------------------------\n");
	
	free(file);
}