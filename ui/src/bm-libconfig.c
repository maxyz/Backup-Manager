/*
 * bm-libconf.c - librairy to handle configuration file 
 *
 * Copyright (C) 2005 The Backup Manager Authors
 * See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bm-libconfig.h"
#include "mem_manager.h"
#include "strLcpy.h"

bm_variable_data bm_config_data[] = {
	{ "BM_NAME_FORMAT", NULL },
	{ "BM_FILETYPE", NULL },
	{ "BM_MAX_TIME_TO_LIVE", NULL },
	{ "BM_DUMP_SYMLINKS", NULL },
	{ "BM_ARCHIVES_PREFIX", NULL },
	{ "BM_DIRECTORIES", NULL },
	{ "BM_DIRECTORIES_BLACKLIST", NULL },
	{ "BM_ARCHIVES_REPOSITORY", NULL },
	{ "BM_USER", NULL },
	{ "BM_GROUP", NULL },
	{ "BM_UPLOAD_MODE", NULL },
	{ "BM_UPLOAD_HOSTS", NULL },
	{ "BM_UPLOAD_USER", NULL },
	{ "BM_UPLOAD_PASSWD", NULL },
	{ "BM_FTP_PURGE", NULL },
	{ "BM_UPLOAD_KEY", NULL },
	{ "BM_UPLOAD_DIR", NULL },
	{ "BM_BURNING", NULL },
	{ "BM_BURNING_MEDIA", NULL },
	{ "BM_BURNING_DEVICE", NULL },
	{ "BM_BURNING_METHOD", NULL },
	{ "BM_BURNING_MAXSIZE", NULL },
	{ "BM_LOGGER", NULL },
	{ "BM_LOGGER_FACILITY", NULL },
	{ "BM_PRE_BACKUP_COMMAND", NULL },
	{ "BM_POST_BACKUP_COMMAND", NULL }
};

static char *
__read_variable_data (FILE *file); 

static char *
__read_variable_name (FILE *file);

static BM_Bool 
__is_variable_name (const char *variable, int *index );

static void 
__strip_space (FILE *file);

static void 
__go_to_next_line (FILE *file);

static BM_Bool 
__read_export (FILE *file);

static void
__go_to_end_data (FILE *file);

//static int 
int 
__add_slashes (const char *string, char **dest);

static int 
__strip_slashes (const char *string, char **dest);

/* external fonctions */

BM_Bool 
bm_load_config (const char* conf_file) 
{
	FILE		*bm_file;
	char		*bm_variable_name;
	char		*bm_variable_data;
	char		tmp[BM_BUFF_SIZE];
	int		bm_read_char;
	int		offset = 0;
	int		index = 0;
	BM_Bool		next = BM_TRUE;
	/* size_t		bm_variable_data_size; */
	
	if ( bm_file = fopen( conf_file, "r") ) {
		while ( !feof(bm_file) ) {
			if ( ( bm_read_char = fgetc(bm_file) ) != EOF ) {
				
				__strip_space(bm_file);
				
				/* comment line */
				if ( (char) bm_read_char == '#' ) {
					__go_to_next_line(bm_file);
					continue;
				} 
				
				/* end line */
				if ( (char) bm_read_char == '\n' )
					continue;
				

				fseek(bm_file, -1 * sizeof(char) , SEEK_CUR); 

				/* the word 'export' */
				if ( !__read_export(bm_file) )
					continue;
				
				__strip_space(bm_file);
				
				/* variable name */
				bm_variable_name = __read_variable_name(bm_file);

				if ( bm_variable_name == NULL ) /* FIXME */
					return BM_FALSE;
				
				if ( !__is_variable_name(bm_variable_name, &index) ) {
					mem_free(bm_variable_name);
					continue;
				}

				mem_free(bm_variable_name);
				__strip_space(bm_file);
			
				bm_variable_data = __read_variable_data(bm_file);

				if ( bm_variable_data == NULL )
					return BM_FALSE;
				
				if ( __strip_slashes(bm_variable_data, &bm_config_data[index].BM_VARIABLE_DATA) == -1 )
					return BM_FALSE; /* FIXME */
				
				mem_free(bm_variable_data);
			} 
		}

		fclose(bm_file);
	} else {
		perror("can't open configuration file");
		return BM_FALSE;
	}

	return BM_TRUE;
}

BM_Bool 
bm_write_config (const char *dest_file_name) 
{

	FILE	*fh_in, *fh_out;
	char	*bm_variable_name;
	char	tmp[BM_BUFF_SIZE];
	int	read_char;
	int	index_word		= 0;
	int	index_config		= 0;
	BM_Bool	write_variable		= BM_FALSE;
	BM_Bool	read_variable		= BM_FALSE;
	BM_Bool	find_tpl_special_char	= BM_FALSE;
	BM_Bool	in_comment		= BM_FALSE;
	BM_Bool word_is_export		= BM_FALSE;
	BM_Bool ret			= BM_TRUE;
	
	
	if ( fh_in = fopen(BM_TPL_FILE, "r") ) {
		if ( fh_out = fopen(dest_file_name, "w") ) {
			while( (read_char = fgetc(fh_in)) != EOF ) {
				
				//printf("%c", (char)read_char);

				fputc(read_char, fh_out);
				if ( in_comment ) {
					if ( (char) read_char == '\n' ) 					
						in_comment = BM_FALSE;
					continue;
				}
				
				if ( read_char >= BM_BUFF_SIZE ) {
					ret = BM_FALSE;
					break;
				}
				
				if ( (char)read_char == ' ' || (char)read_char == '\n' ) { 
					tmp[index_word] = '\0';
				} else {
					tmp[index_word] = (char)read_char;
					index_word++;
					continue;
				}

				if ( strcmp(tmp, "export") == 0 )
					read_variable = BM_TRUE;
				

				tmp[0] = '\0';
				index_word = 0;

				if ( read_variable ) {
					
					bm_variable_name = __read_variable_name(fh_in);
					fwrite(bm_variable_name, sizeof(char), 
					       strlen(bm_variable_name) , fh_out);
					fputc('=', fh_out);
					
					if ( __is_variable_name( bm_variable_name, &index_config) ) 
						write_variable= BM_TRUE;
					
					mem_free(bm_variable_name);
				}

				if ( write_variable ) {

					fputc('\"', fh_out);
					if ( bm_config_data[index_config].BM_VARIABLE_DATA != NULL ) 
						fwrite (bm_config_data[index_config].BM_VARIABLE_DATA, 
						        sizeof(char), strlen(bm_config_data[index_config].BM_VARIABLE_DATA),
							fh_out);
					
					fputc('\"', fh_out);				
					__go_to_end_data(fh_in);
				}
				
				write_variable = BM_FALSE;
				read_variable = BM_FALSE;
				
				if ( (char)read_char == '#' ) 
					in_comment = BM_TRUE;
				
			}
			fclose(fh_out);
		} else {
			perror("can't open sav file");
			ret = BM_FALSE;
		}

		fclose(fh_in);
	} else {
		// printf("%s", BM_TPL_FILE);
		perror("can't open template file");
		ret = BM_FALSE;
	}

	return ret;
} 

void 
bm_free_config () 
{

	int i;
	for ( i = 0 ; i < BM_NB_VARIABLE ; i++ ) {
		if (bm_config_data[i].BM_VARIABLE_DATA != NULL) 
			mem_free(bm_config_data[i].BM_VARIABLE_DATA);
	}

}

void 
bm_display_config () 
{
	int i;

	for ( i = 0 ; i < BM_NB_VARIABLE ; i++ ) {
		printf ( "%s = %s\n", 
			 bm_config_data[i].BM_VARIABLE_NAME, 
			 bm_config_data[i].BM_VARIABLE_DATA);
	}

}

char* 
bm_get_variable_data (const char *bm_variable) 
{
	int index = 0;
	
	if ( __is_variable_name(bm_variable, &index) ) {
		return bm_config_data[index].BM_VARIABLE_DATA;
	} else {
		return NULL;
	}
	
}

BM_Bool
bm_set_variable_data (const char *bm_variable, const char *bm_dada) 
{
	int	index	= 0;
	size_t	size	= 0;
	BM_Bool	ret	= BM_FALSE;
	
	if ( __is_variable_name(bm_variable, &index) ) {
		
		if ( bm_config_data[index].BM_VARIABLE_DATA != NULL ) 
			mem_free(bm_config_data[index].BM_VARIABLE_DATA);
		
		size = strlen(bm_dada);
		
		bm_config_data[index].BM_VARIABLE_DATA = (char*) mem_alloc( (size + 1) * sizeof(char));
		
		if ( bm_config_data[index].BM_VARIABLE_DATA != NULL ) {
			string_copy ( bm_config_data[index].BM_VARIABLE_DATA, 
			              bm_dada, size + 1 );
			ret = BM_TRUE;
		}
	}

	return ret;
}

/* static fonctions */

static char*
__read_variable_data (FILE *file) 
{

	BM_Bool	next	   = BM_TRUE;
	BM_Bool	data_start = BM_FALSE;
	int 	offset	   = 0;
	int	read_char;
	size_t	name_size;
	char	tmp[BM_BUFF_SIZE];	
	char	*dest;
	char	last_read_char = '\0';

	
	while ( next && ( offset < BM_BUFF_SIZE ) ) {
		
		if ( ( read_char = fgetc(file) ) != EOF ) {
			
			if ( !data_start ) {
				if ( read_char == '"' ) 
					data_start = BM_TRUE;
				continue;
			}
			
			if ( (char)read_char == '\n' ) 
				break;
			
			if ( (char)read_char == '"' && last_read_char != '\\' ) {
				next = BM_FALSE;
			} else {
				tmp[offset] = (char) read_char;
			}
			last_read_char = (char)read_char;
		
		} 
		offset++;
	}

	if ( next ) // FIXME le ficheir de conf est moisi 
		return NULL;
	
	tmp[offset-1] = '\0';
	name_size = strlen(tmp) + 1;
	
	dest = (char*) mem_alloc_with_name (name_size * sizeof(char), "dest_variable_data");

	if ( dest == NULL )
		return NULL;
	
	string_copy(dest, tmp, name_size);
	
	return dest;
}

static char *
__read_variable_name (FILE *file) 
{

	char	tmp[BM_BUFF_SIZE];	
	BM_Bool	next	= BM_TRUE;
	int 	offset	= 0;
	int	read_char;
	size_t	name_size;
	char	*dest;
	
	while ( next && ( offset < BM_BUFF_SIZE - 1 ) ) {
		if ( ( read_char = fgetc(file)) != EOF )  {
			if ( (char)read_char == ' ' || (char)read_char == '=' || (char)read_char == '\n' ) {
				next = BM_FALSE;
			} else {
				tmp[offset] = (char) read_char;
			}
		} else {
			next = BM_FALSE;
		}
		offset++;
	}
	tmp[(offset - 1)] = '\0';
	
	name_size = strlen(tmp) + 1;
	dest = (char*) mem_alloc_with_name (name_size * sizeof(char), "dest_variable_name");

	if ( dest == NULL ) 
		return NULL;
	
	string_copy(dest, tmp, name_size);

	return dest;
}

static BM_Bool 
__is_variable_name (const char *variable, int *p_index ) 
{
	int 	i;
	BM_Bool	bm_variable_ok = BM_FALSE;
	*p_index = 0;
	
	for ( i = 0 ; i < BM_NB_VARIABLE ; i++ ) {
		if ( strcmp(variable , bm_config_data[i].BM_VARIABLE_NAME) == 0 ) {
			bm_variable_ok = BM_TRUE;
			*p_index = i;
		}
	}

	return bm_variable_ok;
}

static void 
__strip_space (FILE *file) 
{

	BM_Bool next = BM_TRUE;
	int read_char;
	
	while (next) {
		if ( ( read_char = fgetc(file) != EOF ) ) {
			if ( (char) read_char != ' ' ) {
				next = BM_FALSE;
				fseek(file, -1 * sizeof(char) , SEEK_CUR); 
			}
		} else {
			next = BM_FALSE;
		}
	};

}

static void 
__go_to_next_line (FILE *file) 
{

	int	read_char;
	BM_Bool	next;	
	
	next = BM_TRUE;
	
	while ( next ) {
		if ( (read_char = fgetc(file) ) != EOF ) {
			if ( (char) read_char == '\n')
				next = BM_FALSE;
		} else {
			next = BM_FALSE;
		}
	}

}

static BM_Bool 
__read_export (FILE *file) 
{
	
	char	tmp[BM_BUFF_SIZE];
	int	offset = 0;	
	int	read_char;
	BM_Bool	next = BM_TRUE;

	while ( next && ( offset < BM_BUFF_SIZE - 1 ) ) {
		if ( ( read_char = fgetc(file) ) != EOF ) {
			if ( (char)read_char == ' ' || (char)read_char == '\n' || (char)read_char == '\t') {
				next = BM_FALSE;
			} else {
				tmp[offset] = (char) read_char;	
			}
		} else {
			next = BM_FALSE;
		}
		offset++;
	}
	tmp[offset - 1 ] = '\0';
	
	if ( strcmp(tmp, "export") == 0 ) {
		return BM_TRUE;
	} else {
		return BM_FALSE;
	}
}

static void 
__go_to_end_data (FILE *file) 
{

	int	read_char;
	char	last_read_char = '\0';
	BM_Bool	next, start_word;	
	
	next		= BM_TRUE;
	start_word	= BM_FALSE;
	
	while ( next ) {
		if ( (read_char = fgetc(file) ) != EOF ) {

			if ( (char) read_char == '\n' ) {
				next = BM_FALSE;
				continue;
			}
				
			if ( (char) read_char == '"') {
				if ( !start_word ) {
					start_word = BM_TRUE;	
				} else {
					if ( last_read_char != '\\' )
						next = BM_FALSE;
				}
			}
			last_read_char = (char)read_char;
		} else {
			next = BM_FALSE;
		}
	}
}

//static int 
int 
__add_slashes (const char *string, char **dest) 
{

	int	nb, i, index;
	char	tmp[BM_BUFF_SIZE];
	size_t	size;
	
	nb	= 0;
	index	= 0;
	size	= strlen(string);

	for ( i = 0 ; i < size ; i++ ) {
		if ( index == BM_BUFF_SIZE ) 
			break;
		
		if ( string[i] == '"' || string[i] == '\\' ) {
			tmp[index] = '\\';
			index++;
			nb++;
		} 

		tmp[index] = string[i];
		index++;
	}
	tmp[index] = '\0';

	*dest = (char*) mem_alloc((index + 1) * sizeof(char));
	
	if ( *dest == NULL )
		return -1;

	string_copy(*dest, tmp, index + 1);
	
	return nb;
}

static int 
__strip_slashes (const char *string, char **dest)
{
	int     nb, i, index;
	char    tmp[BM_BUFF_SIZE];
	char	read_char;
	size_t	size;
	
	nb      = 0;
	index   = 0;
	size    = strlen(string);
	read_char = '\0';
	
	for ( i = 0 ; i < size ; i++ ) {
		if ( index == BM_BUFF_SIZE )
			break;

		if ( string[i] == '\\' && read_char != '\\') {
			nb++;
			continue;
		}
		
		tmp[index] = string[i];
		read_char = string[i];
		index++;
	
	}
	
	tmp[index] = '\0';

	*dest = (char*) mem_alloc((index + 1) * sizeof(char));
	
	if ( *dest == NULL )
		return -1;

	string_copy(*dest, tmp, index + 1);
	
	return nb;
}