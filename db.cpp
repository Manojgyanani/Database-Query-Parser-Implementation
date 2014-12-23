/************************************************************
Project#1:	CLP & DDL
************************************************************/
#include "db.h"

int main(int argc, char** argv)
{
	if ((argc != 2) || (strlen(argv[1]) == 0))
	{
		printf("Usage: db \"command statement\"");
		return 1;
	}

	return process_input(argv[1]);
}

int process_input(char *str)
{
	int rc = 0;
	//char backup[MAX_IDENT_LEN];
	token_list *tok_list=NULL, *tok_ptr=NULL, *tmp_tok_ptr=NULL;
	if ((rc = create_log_file()) != 0)
	{
		printf("Error creating db.log file\nrc = %d\n", rc);
	}
	else if ((rc = initialize_tpd_list()) != 0)
	{
		printf("\nError in initialize_tpd_list().\nrc = %d\n", rc);
	}
	else
	{
		rc = get_token(str, &tok_list);

		/* Test code */
		tok_ptr = tok_list;
		/*while (tok_ptr != NULL)
		{
			printf("%16s \t%d \t %d\n",tok_ptr->tok_string, tok_ptr->tok_class,
				tok_ptr->tok_value);
			tok_ptr = tok_ptr->next;
		}*/

		if (!rc)
		{
			rc = do_semantic(tok_ptr, str);
		}

		if (rc)
		{
			tok_ptr = tok_list;
			while (tok_ptr != NULL)
			{
				if ((tok_ptr->tok_class == error) ||
					(tok_ptr->tok_value == INVALID))
				{
					printf("\nError in the string: %s\n", tok_ptr->tok_string);
					printf("rc=%d\n", rc);
					break;
				}
				tok_ptr = tok_ptr->next;
			}
		}

		/* Whether the token list is valid or not, we need to free the memory */
		tok_ptr = tok_list;
		if (!stristr(str, "restore"))
		{
			while (tok_ptr != NULL)
			{
				tmp_tok_ptr = tok_ptr->next;
				free(tok_ptr);
				tok_ptr=tmp_tok_ptr;
			}
		}
	}

	return rc;
}
/*************************************************************************************/
/*								 GIVEN FUNCTIONS									 */
/*************************************************************************************/
int get_token(char* command, token_list** tok_list)
{
	int rc=0,i,j;
	char *start, *cur, temp_string[MAX_TOK_LEN];
	bool done = false;

	start = cur = command;
	while (!done)
	{
		bool found_keyword = false;

		/* This is the TOP Level for each token */
		memset ((void*)temp_string, '\0', MAX_TOK_LEN);
		i = 0;

		/* Get rid of all the leading blanks */
		while (*cur == ' ')
			cur++;

		if (cur && isalpha(*cur))
		{
			// find valid identifier
			int t_class;
			do 
			{
				temp_string[i++] = *cur++;
			}
			while ((isalnum(*cur)) || (*cur == '_'));

			if (!(strchr(STRING_BREAK, *cur)))
			{
				/* If the next char following the keyword or identifier
				is not a blank, (, ), or a comma, then append this
				character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else
			{

				// We have an identifier with at least 1 character
				// Now check if this ident is a keyword
				for (j = 0, found_keyword = false; j < TOTAL_KEYWORDS_PLUS_TYPE_NAMES; j++)
				{
					if ((stricmp(keyword_table[j], temp_string) == 0))
					{
						found_keyword = true;
						break;
					}
				}

				if (found_keyword)
				{
					if (KEYWORD_OFFSET+j < K_CREATE)
						t_class = type_name;
					else if (KEYWORD_OFFSET+j >= F_SUM)
						t_class = function_name;
					else
						t_class = keyword;

					add_to_list(tok_list, temp_string, t_class, KEYWORD_OFFSET+j);
				}
				else
				{
					if (strlen(temp_string) <= MAX_IDENT_LEN)
						add_to_list(tok_list, temp_string, identifier, IDENT);
					else
					{
						add_to_list(tok_list, temp_string, error, INVALID);
						rc = INVALID;
						done = true;
					}
				}

				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else if (isdigit(*cur))
		{
			// find valid number
			do 
			{
				temp_string[i++] = *cur++;
			}
			while (isdigit(*cur));

			if (!(strchr(NUMBER_BREAK, *cur)))
			{
				/* If the next char following the keyword or identifier
				is not a blank or a ), then append this
				character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else
			{
				add_to_list(tok_list, temp_string, constant, INT_LITERAL);

				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else if ((*cur == '(') || (*cur == ')') || (*cur == ',') || (*cur == '*')
			|| (*cur == '=') || (*cur == '<') || (*cur == '>'))
		{
			/* Catch all the symbols here. Note: no look ahead here. */
			int t_value;
			switch (*cur)
			{
			case '(' : t_value = S_LEFT_PAREN; break;
			case ')' : t_value = S_RIGHT_PAREN; break;
			case ',' : t_value = S_COMMA; break;
			case '*' : t_value = S_STAR; break;
			case '=' : t_value = S_EQUAL; break;
			case '<' : t_value = S_LESS; break;
			case '>' : t_value = S_GREATER; break;
			}

			temp_string[i++] = *cur++;

			add_to_list(tok_list, temp_string, symbol, t_value);

			if (!*cur)
			{
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
		}
		else if (*cur == '\'')
		{
			/* Find STRING_LITERRAL */
			int t_class;
			cur++;
			do 
			{
				temp_string[i++] = *cur++;
			}
			while ((*cur) && (*cur != '\''));

			temp_string[i] = '\0';

			if (!*cur)
			{
				/* If we reach the end of line */
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else /* must be a ' */
			{
				add_to_list(tok_list, temp_string, constant, STRING_LITERAL);
				cur++;
				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else
		{
			if (!*cur)
			{
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
			else
			{
				/* not a ident, number, or valid symbol */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
		}
	}

	return rc;
}

void add_to_list(token_list **tok_list, char *tmp, int t_class, int t_value)
{
	token_list *cur = *tok_list;
	token_list *ptr = NULL;

	// printf("%16s \t%d \t %d\n",tmp, t_class, t_value);

	ptr = (token_list*)calloc(1, sizeof(token_list));
	strcpy(ptr->tok_string, tmp);
	ptr->tok_class = t_class;
	ptr->tok_value = t_value;
	ptr->next = NULL;

	if (cur == NULL)
		*tok_list = ptr;
	else
	{
		while (cur->next != NULL)
			cur = cur->next;

		cur->next = ptr;
	}
	return;
}

int initialize_tpd_list()
{
	int rc = 0;
	FILE *fhandle = NULL;
	struct _stat file_stat;

	/* Open for read */
	if((fhandle = fopen("dbfile.bin", "rbc")) == NULL)
	{
		if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
		{
			rc = FILE_OPEN_ERROR;
		}
		else
		{
			g_tpd_list = NULL;
			g_tpd_list = (tpd_list*)calloc(1, sizeof(tpd_list));
			//printf("DEBUG: sizeof(tpd_list) = %d\n", sizeof(tpd_list)); // temp

			if (!g_tpd_list)
			{
				rc = MEMORY_ERROR;
			}
			else
			{
				g_tpd_list->list_size = sizeof(tpd_list);
				fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
				//fflush(fhandle);
				fclose(fhandle);
			}
		}
	}
	else
	{
		/* There is a valid dbfile.bin file - get file size */
		_fstat(_fileno(fhandle), &file_stat);
		//printf("dbfile.bin size = %d\n", file_stat.st_size);

		g_tpd_list = (tpd_list*)calloc(1, file_stat.st_size);

		if (!g_tpd_list)
		{
			rc = MEMORY_ERROR;
		}
		else
		{
			fread(g_tpd_list, file_stat.st_size, 1, fhandle);
			//fflush(fhandle);
			fclose(fhandle);

			if (g_tpd_list->list_size != file_stat.st_size)
			{
				rc = DBFILE_CORRUPTION;
			}

		}
	}

	return rc;
}

int add_tpd_to_list(tpd_entry *tpd)
{
	int rc = 0;
	int old_size = 0;
	FILE *fhandle = NULL;

	if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		old_size = g_tpd_list->list_size;

		if (g_tpd_list->num_tables == 0)
		{
			/* If this is an empty list, overlap the dummy header */
			g_tpd_list->num_tables++;
			g_tpd_list->list_size += (tpd->tpd_size - sizeof(tpd_entry)); //120
			fwrite(g_tpd_list, old_size - sizeof(tpd_entry), 1, fhandle);
		}
		else
		{
			/* There is at least 1, just append at the end */
			g_tpd_list->num_tables++;
			g_tpd_list->list_size += tpd->tpd_size;
			fwrite(g_tpd_list, old_size, 1, fhandle);
		}

		fwrite(tpd, tpd->tpd_size, 1, fhandle);
		//fflush(fhandle);
		fclose(fhandle);
	}

	return rc;
}

int drop_tpd_from_list(char *tabname)
{
	int rc = 0;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int count = 0;

	if (num_tables > 0)
	{
		while ((!found) && (num_tables-- > 0))
		{
			if (stricmp(cur->table_name, tabname) == 0)
			{
				/* found it */
				found = true;
				int old_size = 0;
				FILE *fhandle = NULL;

				if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
				{
					rc = FILE_OPEN_ERROR;
				}
				else
				{
					old_size = g_tpd_list->list_size;

					if (count == 0)
					{
						/* If this is the first entry */
						g_tpd_list->num_tables--;

						if (g_tpd_list->num_tables == 0)
						{
							/* This is the last table, null out dummy header */
							memset((void*)g_tpd_list, '\0', sizeof(tpd_list));
							g_tpd_list->list_size = sizeof(tpd_list);
							fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
						}
						else
						{
							/* First in list, but not the last one */
							g_tpd_list->list_size -= cur->tpd_size;

							/* First, write the 8 byte header */
							fwrite(g_tpd_list, sizeof(tpd_list) - sizeof(tpd_entry),
								1, fhandle);

							/* Now write everything starting after the cur entry */
							fwrite((char*)cur + cur->tpd_size,
								old_size - cur->tpd_size -
								(sizeof(tpd_list) - sizeof(tpd_entry)),
								1, fhandle);
						}
					}
					else
					{
						/* This is NOT the first entry - count > 0 */
						g_tpd_list->num_tables--;
						g_tpd_list->list_size -= cur->tpd_size;

						/* First, write everything from beginning to cur */
						fwrite(g_tpd_list, ((char*)cur - (char*)g_tpd_list),
							1, fhandle);

						/* Check if cur is the last entry. Note that g_tdp_list->list_size
						has already subtracted the cur->tpd_size, therefore it will
						point to the start of cur if cur was the last entry */
						if ((char*)g_tpd_list + g_tpd_list->list_size == (char*)cur)
						{
							/* If true, nothing else to write */
						}
						else
						{
							/* NOT the last entry, copy everything from the beginning of the
							next entry which is (cur + cur->tpd_size) and the remaining size */
							fwrite((char*)cur + cur->tpd_size,
								old_size - cur->tpd_size -
								((char*)cur - (char*)g_tpd_list),							     
								1, fhandle);
						}
					}

					//fflush(fhandle);
					fclose(fhandle);
				}


			}
			else
			{
				if (num_tables > 0)
				{
					cur = (tpd_entry*)((char*)cur + cur->tpd_size);
					count++;
				}
			}
		}
	}

	if (!found)
	{
		rc = INVALID_TABLE_NAME;
	}

	return rc;
}

tpd_entry* get_tpd_from_list(char *tabname)
{
	tpd_entry *tpd = NULL;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;

	if (num_tables > 0)
	{
		while ((!found) && (num_tables-- > 0))
		{
			if (stricmp(cur->table_name, tabname) == 0)
			{
				/* found it */
				found = true;
				tpd = cur;
			}
			else
			{
				if (num_tables > 0)
				{
					cur = (tpd_entry*)((char*)cur + cur->tpd_size);
				}
			}
		}
	}

	return tpd;
}

int do_semantic(token_list *tok_list, char *statement)
{
	clock_t start = NULL, end = NULL;
	int rc = 0, cur_cmd = INVALID_STATEMENT;
	bool unique = false;
	token_list *cur = tok_list;

	if (g_tpd_list->db_flags)
	{
		if (cur->tok_value != K_ROLLFORWARD)
		{
			printf("Error: ROLLFORWARD is pending!\n");
			rc = ROLLFORWARD_PENDING;
		}
		else
		{
			cur_cmd = ROLLFORWARD;
			cur = cur->next;
		}
	}
	else if ((cur->tok_value == K_CREATE) &&
		((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		//printf("CREATE TABLE statement\n");
		cur_cmd = CREATE_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_DROP) &&
		((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		//printf("DROP TABLE statement\n");
		cur_cmd = DROP_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_LIST) &&
		((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		//printf("LIST TABLE statement\n");
		cur_cmd = LIST_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_LIST) &&
		((cur->next != NULL) && (cur->next->tok_value == K_SCHEMA)))
	{
		//printf("LIST SCHEMA statement\n");
		cur_cmd = LIST_SCHEMA;
		cur = cur->next->next;
	}
	else if (cur->tok_value == K_INSERT && 
		cur->next != NULL && cur->next->tok_value == K_INTO)
	{
		//printf("INSERT statement\n");
		cur_cmd = INSERT;
		cur = cur->next->next;
	}
	else if (cur->tok_value == K_DELETE &&
		cur->next != NULL && cur->next->tok_value == K_FROM)
	{
		//printf("DELETE statement\n");
		cur_cmd = DELETE;
		cur = cur->next->next;
	}
	else if (cur->tok_value == K_UPDATE &&
		cur->next != NULL)
	{
		//printf("UPDATE statement\n");
		cur_cmd = UPDATE;
		cur = cur->next;
	}
	else if (cur->tok_value == K_SELECT &&
		cur->next != NULL)
	{
		//printf("SELECT statement\n");
		cur_cmd = SELECT;
		cur = cur->next;
	}
	else if (cur->tok_value == K_BACKUP &&
		cur->next->tok_value == K_TO)
	{
		cur_cmd = BACKUP;
		cur = cur->next->next;
	}
	else if (cur->tok_value == K_RESTORE &&
		cur->next->tok_value != EOC)
	{
		cur_cmd = RESTORE;
		cur = cur->next;
	}
	else
	{
		printf("Invalid statement\n");
		rc = cur_cmd;
	}

	if (cur_cmd != INVALID_STATEMENT)
	{
		start = clock();
		switch(cur_cmd)
		{
		case CREATE_TABLE:
			rc = sem_create_table(cur);
			break;
		case DROP_TABLE:
			rc = sem_drop_table(cur);
			break;
		case LIST_TABLE:
			rc = sem_list_tables();
			break;
		case LIST_SCHEMA:
			rc = sem_list_schema(cur);
			break;
		case INSERT:
			rc = sem_insert(cur);
			break;
		case DELETE:
			rc = sem_delete(cur);
			break;
		case UPDATE:
			rc = sem_update(cur);
			break;
		case SELECT:
			rc = sem_select(cur);
			break;
		case BACKUP:
			rc = sem_backup(cur);
			break;
		case RESTORE:
			rc = sem_restore(cur);
			break;
		case ROLLFORWARD:
			rc = sem_rollforward(cur);
			break;
		default:
			; /* no action */
		}
	}

	end = clock();
	if (!rc || rc == NO_ROWS_DELETED || rc == NO_ROWS_UPDATED)
		printf ("(%.2lf sec)\n", (double)(end - start) / CLOCKS_PER_SEC);

	// valid query, add to log file
	if (!rc && cur_cmd != BACKUP && cur_cmd != RESTORE && cur_cmd != ROLLFORWARD)
		add_log_entry(statement);

	return rc;
}

int sem_create_table(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry tab_entry;
	tpd_entry *new_entry = NULL;
	bool column_done = false;
	int cur_id = 0;
	int row_size = 0;
	cd_entry col_entry[MAX_NUM_COL];

	memset(&tab_entry, '\0', sizeof(tpd_entry));
	//printf("DEBUG: sizeof(tpd_entry) = %d\n", sizeof(tpd_entry));
	cur = t_list;
	if ((cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		if ((new_entry = get_tpd_from_list(cur->tok_string)) != NULL)
		{
			rc = DUPLICATE_TABLE_NAME;
			cur->tok_value = INVALID;
		}
		else
		{
			strcpy(tab_entry.table_name, cur->tok_string);
			cur = cur->next;
			if (cur->tok_value != S_LEFT_PAREN)
			{
				//Error
				rc = INVALID_TABLE_DEFINITION;
				cur->tok_value = INVALID;
			}
			else
			{
				memset(&col_entry, '\0', (MAX_NUM_COL * sizeof(cd_entry)));

				/* Now build a set of column entries */
				cur = cur->next;
				do
				{
					if ((cur->tok_class != keyword) &&
						(cur->tok_class != identifier) &&
						(cur->tok_class != type_name))
					{
						// Error
						rc = INVALID_COLUMN_NAME;
						cur->tok_value = INVALID;
					}
					else
					{
						int i;
						for(i = 0; i < cur_id; i++)
						{
							/* make column name case sensitive */
							if (strcmp(col_entry[i].col_name, cur->tok_string)==0)
							{
								rc = DUPLICATE_COLUMN_NAME;
								cur->tok_value = INVALID;
								break;
							}
						}

						if (!rc)
						{
							strcpy(col_entry[cur_id].col_name, cur->tok_string);
							col_entry[cur_id].col_id = cur_id;
							col_entry[cur_id].not_null = false;    /* set default */

							cur = cur->next;
							if (cur->tok_class != type_name)
							{
								// Error
								rc = INVALID_TYPE_NAME;
								cur->tok_value = INVALID;
							}
							else
							{
								/* Set the column type here, int or char */
								col_entry[cur_id].col_type = cur->tok_value;
								cur = cur->next;

								if (col_entry[cur_id].col_type == T_INT)
								{
									if ((cur->tok_value != S_COMMA) &&
										(cur->tok_value != K_NOT) &&
										(cur->tok_value != S_RIGHT_PAREN))
									{
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
									else
									{
										col_entry[cur_id].col_len = sizeof(int);
										row_size += 1 + sizeof(int); 

										if ((cur->tok_value == K_NOT) &&
											(cur->next->tok_value != K_NULL))
										{
											rc = INVALID_COLUMN_DEFINITION;
											cur->tok_value = INVALID;
										}	
										else if ((cur->tok_value == K_NOT) &&
											(cur->next->tok_value == K_NULL))
										{					
											col_entry[cur_id].not_null = true;
											cur = cur->next->next;
										}

										if (!rc)
										{
											/* I must have either a comma or right paren */
											if ((cur->tok_value != S_RIGHT_PAREN) &&
												(cur->tok_value != S_COMMA))
											{
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											}
											else
											{
												if (cur->tok_value == S_RIGHT_PAREN)
												{
													column_done = true;
												}
												cur = cur->next;
											}
										}
									}
								}   // end of S_INT processing
								else
								{
									// It must be char()
									if (cur->tok_value != S_LEFT_PAREN)
									{
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
									else
									{
										/* Enter char(n) processing */
										cur = cur->next;

										if (cur->tok_value != INT_LITERAL)
										{
											rc = INVALID_COLUMN_LENGTH;
											cur->tok_value = INVALID;
										}
										else
										{
											/* Got a valid integer - convert */
											col_entry[cur_id].col_len = atoi(cur->tok_string);
											cur = cur->next;
											row_size += 1 + sizeof(char) * col_entry[cur_id].col_len;

											if (cur->tok_value != S_RIGHT_PAREN)
											{
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											}
											else
											{
												cur = cur->next;

												if ((cur->tok_value != S_COMMA) &&
													(cur->tok_value != K_NOT) &&
													(cur->tok_value != S_RIGHT_PAREN))
												{
													rc = INVALID_COLUMN_DEFINITION;
													cur->tok_value = INVALID;
												}
												else
												{
													if ((cur->tok_value == K_NOT) &&
														(cur->next->tok_value != K_NULL))
													{
														rc = INVALID_COLUMN_DEFINITION;
														cur->tok_value = INVALID;
													}
													else if ((cur->tok_value == K_NOT) &&
														(cur->next->tok_value == K_NULL))
													{					
														col_entry[cur_id].not_null = true;
														cur = cur->next->next;
													}

													if (!rc)
													{
														/* I must have either a comma or right paren */
														if ((cur->tok_value != S_RIGHT_PAREN) &&															  (cur->tok_value != S_COMMA))
														{
															rc = INVALID_COLUMN_DEFINITION;
															cur->tok_value = INVALID;
														}
														else
														{
															if (cur->tok_value == S_RIGHT_PAREN)
															{
																column_done = true;
															}
															cur = cur->next;
														}
													}
												}
											}
										}	/* end char(n) processing */
									}
								} /* end char processing */
							}
						}  // duplicate column name
					} // invalid column name

					/* If rc=0, then get ready for the next column */
					if (!rc)
					{
						cur_id++;
					}

				} while ((rc == 0) && (!column_done));

				if ((column_done) && (cur->tok_value != EOC))
				{
					rc = INVALID_TABLE_DEFINITION;
					cur->tok_value = INVALID;
				}

				if (!rc)
				{
					/* Now finished building tpd and add it to the tpd list */
					tab_entry.num_columns = cur_id;
					tab_entry.tpd_size = sizeof(tpd_entry) + sizeof(cd_entry) *	tab_entry.num_columns;
					tab_entry.cd_offset = sizeof(tpd_entry);
					new_entry = (tpd_entry*)calloc(1, tab_entry.tpd_size);

					if (new_entry == NULL)
					{
						rc = MEMORY_ERROR;
					}
					else
					{
						memcpy((void*)new_entry,
							(void*)&tab_entry,
							sizeof(tpd_entry));

						memcpy((void*)((char*)new_entry + sizeof(tpd_entry)),
							(void*)col_entry,
							sizeof(cd_entry) * tab_entry.num_columns);

						rc = add_tpd_to_list(new_entry);

						if (!rc)
						{
							//printf("DEBUG: row_size = %d\n", row_size); // temp
							rc = create_tab_file(new_entry->table_name, row_size);

							if (!rc)
								printf("Table created ");
						}

						free(new_entry);
					}
				}
			}
		}
	}
	return rc;
}

int sem_drop_table(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;

	cur = t_list;
	if ((cur->tok_class != keyword) &&
		(cur->tok_class != identifier) &&
		(cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		if (cur->next->tok_value != EOC)
		{
			rc = INVALID_STATEMENT;
			cur->next->tok_value = INVALID;
		}
		else
		{
			if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
			{
				rc = TABLE_NOT_EXIST;
				cur->tok_value = INVALID;
			}
			else
			{
				/* Found a valid tpd, drop it from tpd list */
				rc = drop_tpd_from_list(cur->tok_string);

				if (!rc)
				{
					rc = delete_tab_file(cur->tok_string);
					if (!rc)
						printf("Table dropped ");
				}
			}
		}
	}

	return rc;
}

int sem_list_tables()
{
	int rc = 0;
	int num_tables = g_tpd_list->num_tables;
	tpd_entry *cur = &(g_tpd_list->tpd_start);

	if (num_tables == 0)
	{
		printf("\nThere are currently no tables defined\n");
	}
	else
	{
		printf("\nTable List\n");
		printf("*****************\n");
		while (num_tables-- > 0)
		{
			printf("%s\n", cur->table_name);
			if (num_tables > 0)
			{
				cur = (tpd_entry*)((char*)cur + cur->tpd_size);
			}
		}
		printf("****** End ******\n");
	}

	return rc;
}

int sem_list_schema(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;
	cd_entry  *col_entry = NULL;
	char tab_name[MAX_IDENT_LEN+1];
	char filename[MAX_IDENT_LEN+1];
	bool report = false;
	FILE *fhandle = NULL;
	int i = 0;

	cur = t_list;

	if (cur->tok_value != K_FOR)
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;

		if ((cur->tok_class != keyword) &&
			(cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
		{
			// Error
			rc = INVALID_TABLE_NAME;
			cur->tok_value = INVALID;
		}
		else
		{
			memset(filename, '\0', MAX_IDENT_LEN+1);
			strcpy(tab_name, cur->tok_string);
			cur = cur->next;

			if (cur->tok_value != EOC)
			{
				if (cur->tok_value == K_TO)
				{
					cur = cur->next;

					if ((cur->tok_class != keyword) &&
						(cur->tok_class != identifier) &&
						(cur->tok_class != type_name))
					{
						// Error
						rc = INVALID_REPORT_FILE_NAME;
						cur->tok_value = INVALID;
					}
					else
					{
						if (cur->next->tok_value != EOC)
						{
							rc = INVALID_STATEMENT;
							cur->next->tok_value = INVALID;
						}
						else
						{
							/* We have a valid file name */
							strcpy(filename, cur->tok_string);
							report = true;
						}
					}
				}
				else
				{ 
					/* Missing the TO keyword */
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
			}

			if (!rc)
			{
				if ((tab_entry = get_tpd_from_list(tab_name)) == NULL)
				{
					rc = TABLE_NOT_EXIST;
					cur->tok_value = INVALID;
				}
				else
				{
					if (report)
					{
						if((fhandle = fopen(filename, "a+tc")) == NULL)
						{
							rc = FILE_OPEN_ERROR;
						}
					}

					if (!rc)
					{
						/* Find correct tpd, need to parse column and index information */

						/* First, write the tpd_entry information */
						printf("Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
						printf("Table Name               (table_name)  = %s\n", tab_entry->table_name);
						printf("Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
						printf("Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
						printf("Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags); 

						if (report)
						{
							fprintf(fhandle, "Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
							fprintf(fhandle, "Table Name               (table_name)  = %s\n", tab_entry->table_name);
							fprintf(fhandle, "Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
							fprintf(fhandle, "Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
							fprintf(fhandle, "Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags); 
						}

						/* Next, write the cd_entry information */
						for(i = 0, col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
							i < tab_entry->num_columns; i++, col_entry++)
						{
							printf("Column Name   (col_name) = %s\n", col_entry->col_name);
							printf("Column Id     (col_id)   = %d\n", col_entry->col_id);
							printf("Column Type   (col_type) = %d\n", col_entry->col_type);
							printf("Column Length (col_len)  = %d\n", col_entry->col_len);
							printf("Not Null flag (not_null) = %d\n\n", col_entry->not_null);

							if (report)
							{
								fprintf(fhandle, "Column Name   (col_name) = %s\n", col_entry->col_name);
								fprintf(fhandle, "Column Id     (col_id)   = %d\n", col_entry->col_id);
								fprintf(fhandle, "Column Type   (col_type) = %d\n", col_entry->col_type);
								fprintf(fhandle, "Column Length (col_len)  = %d\n", col_entry->col_len);
								fprintf(fhandle, "Not Null Flag (not_null) = %d\n\n", col_entry->not_null);
							}
						}

						if (report)
						{
							//fflush(fhandle);
							fclose(fhandle);
						}
					} // File open error							
				} // Table not exist
			} // no semantic errors
		} // Invalid table name
	} // Invalid statement

	return rc;
}

/*************************************************************************************/
/*								INSERT FUNCTIONS									 */
/*************************************************************************************/
/* Processes t_list and, if there are no errors, inserts 
   values into specified table. */
int sem_insert(token_list *t_list)
{
	int rc = 0, i, size, temp,
		offset = 0,
		sizeofint = sizeof(int);
	bool comma_next = false;
	token_list *cur;
	tpd_entry *tab_entry;
	cd_entry *col;
	row *r; //*new_r;
	table *tab = NULL, *new_tab = NULL;


	cur = t_list;
	// check table name
	if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
	{
		rc = TABLE_NOT_EXIST;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;
		if (cur->tok_value == EOC)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else
		{
			if (cur->tok_value != K_VALUES)
			{
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			}
			else
			{
				cur = cur->next;
				if (cur->tok_value != S_LEFT_PAREN)
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
				else
				{
					// move to the first value to be inserted
					cur = cur->next;
				}
			}
		}
	}

	// go through values and insert them into a new row
	if (!rc)
	{
		r = (row *)calloc(1, MAX_ROW_SIZE);
		memset(r, '\0', MAX_ROW_SIZE);

		for(i = 0, col = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset); 
			i < tab_entry->num_columns; i++, col++)
		{
			//check for NULL
			if (cur->tok_value == K_NULL)
			{
				if (col->not_null)
				{
					rc = NULL_ASSIGNMENT;
					cur->tok_value = INVALID;
					break;
				}
				else
				{
					// MAX_TOK_LEN + 1 is a sentinel value, it means that the record is NULL
					// insert MAX_TOK_LEN + 1 in one byte that holds record length and increment offset
					size = MAX_TOK_LEN + 1;
					memcpy(r + offset, &size, 1);
					offset += 1 + col->col_len;		
					cur = cur->next;
				}
			}
			// check new value length
			else if (strlen(cur->tok_string) > (col->col_len))
			{
				rc = INVALID_VALUE_LENGTH;
				cur->tok_value = INVALID;
				break;
			}
			// insert integer
			else if (col->col_type == T_INT)
			{
				if (cur->tok_value != INT_LITERAL)
				{
					rc = TYPE_MISMATCH;
					cur->tok_value = INVALID;
					break;
				}
				else
				{
					memcpy(r + offset, &sizeofint, 1);
					offset++;

					temp = atoi(cur->tok_string);
					memcpy(r + offset, &temp, sizeof(int));
					offset += sizeof(int);
					cur = cur->next;
				}
			}
			// insert string
			else
			{
				if (cur->tok_value != STRING_LITERAL)
				{
					rc = TYPE_MISMATCH;
					cur->tok_value = INVALID;
					break;
				}
				else
				{
					size = strlen(cur->tok_string);
					memcpy(r + offset, &size, 1);
					offset++;

					memcpy(r + offset, cur->tok_string, size);
					offset += col->col_len;
					cur = cur->next;
				}
			}

			// skip over the comma
			if (!rc)
			{
				if (cur->tok_value != S_COMMA && cur->tok_value != S_RIGHT_PAREN)
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
				else
				{
					if (cur->tok_value == S_COMMA)
					{
						if (i + 1 == tab_entry->num_columns)
						{
							rc = INVALID_NUMBER_PARAMS;
							cur->tok_value = INVALID;
							break;
						}
						else
						{
							cur = cur->next;
						}
					}
				}
			}
		}

		// check that insert statement ends with legal syntax
		if (!rc)
		{
			if (cur->tok_value != S_RIGHT_PAREN)
			{
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			}
			else if (cur->next->tok_value != EOC)
			{
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			}
		}

		// copy new row into a table
		if (!rc)
		{
			if ((tab = read_tab_file(tab_entry->table_name)) == NULL)
			{
				rc = DBFILE_CORRUPTION;
			}
			else
			{
				if (tab->num_of_rows == MAX_NUM_ROWS)
				{
					printf("Error: '%s' have reached maximum number of records!\n", tab_entry->table_name); 
					rc = MAX_NUM_ROWS_REACHED;
				}
				else
				{
					int old_size = tab->tab_size,
						new_size = tab->tab_size + tab->row_size;

					new_tab = (table*) calloc(1, new_size);
					memset(new_tab, '\0', new_size);

					memcpy(new_tab, tab, tab->tab_size);
					memcpy((void*)((char*)new_tab + old_size), (void*)r, tab->row_size);
					new_tab->tab_size = new_size;
					new_tab->num_of_rows++;

					rc = update_tab_file(new_tab, tab_entry->table_name);
					if (!rc)
						printf("Query OK, 1 row affected ");
				}
			}
		}
	}

	// free memory
	if (tab != NULL)
		free(tab);
	if (new_tab != NULL)
		free(new_tab);

	return rc;
}

/*************************************************************************************/
/*								DELETE FUNCTIONS									 */
/*************************************************************************************/
/* Processes t_list and, if there are no errors, deletes one
   or more records in specified table. */
int sem_delete(token_list *t_list)
{
	int rc = 0;
	token_list *cur = NULL;
	tpd_entry *tab_entry = NULL;
	conditions cond;

	cur = t_list;
	// check if table exists first
	if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
	{
		rc = TABLE_NOT_EXIST;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;
		if (cur->tok_value != K_WHERE && cur->tok_value != EOC)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
	}

	// get search conditions
	if (!rc)
	{
		memset(&cond, '\0', sizeof(conditions));
		if (cur->tok_value != EOC)
		{
			if(!(rc = get_conditions(tab_entry, &cur, &cond)))
			{
				if (cur->tok_value != EOC)
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}

			}
		}
		if (!rc)
			rc = delete_records(tab_entry, &cond);	
	}

	return rc;
}

/* Traverses specified table and deletes rows that satisfy conditions */
int delete_records(tpd_entry *tab_entry, conditions *cond)
{
	int rc = 0, count = 0;
	bool done = false;
	table *tab = NULL;
	row *cur_row = NULL,
		*last_row = NULL;
	cd_entry *col_entry = NULL;

	// get table from file
	if ((tab = read_tab_file(tab_entry->table_name)) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}

	// check if table is empty
	if (!rc)
	{
		if (!tab->num_of_rows)
		{
			printf("Query OK, 0 rows deleted ");
			rc = EMPTY_TABLE;
		}
	}

	// if no conditions, delete all rows from a table
	if (!rc)
	{
		if (!(*cond->col_names))
		{
			// delete all rows from table;
			count = tab->num_of_rows;
			tab->tab_size = sizeof(table);
			tab->num_of_rows = 0;
			done = true;
		}
	}

	// delete rows that satisfy conditions
	if (!rc && !done)
	{	
		cur_row = (row*)((char*)tab + sizeof(table));	

		if (tab->num_of_rows == 1)
		{	
			if (satisfies_cond(cur_row, cond, tab_entry))
			{
				process_delete(&cur_row, &cur_row, &tab, &count);
			}
		}
		else
		{
			last_row = (row*)((char*)tab + tab->tab_size - tab->row_size);
			while(cur_row <= last_row)
			{
				update_last_row_pt(&tab, &last_row, cond, tab_entry, &count);
				if (satisfies_cond(cur_row, cond, tab_entry))
				{
					if (cur_row <= last_row)
						process_delete(&cur_row, &last_row, &tab, &count);
				}
				cur_row += tab->row_size;
			}
		}
	}

	// update tab file
	if (!rc && count != 0)
	{
		rc = update_tab_file(tab, tab_entry->table_name);
	}

	// print count
	if (!rc)
	{
		if (count == 0)
		{
			printf("Query OK, 0 rows deleted ");
			rc = NO_ROWS_DELETED;
		}
		else if (count == 1)
		{
			puts("Query OK, 1 row deleted ");
		}
		else
		{
			printf("Query OK, %d rows deleted ", count);
		}
	}

	// free memory
	if (tab != NULL)
		free(tab);

	return rc;
}

/* Checks if the last row satisfies delete conditions, if yes, decriments
   last_row pointer by a size of row. */
void update_last_row_pt(table **tab, row **last_row, conditions *cond, tpd_entry *tab_entry,
	int *count)
{
	while (satisfies_cond(*last_row, cond, tab_entry) && (*tab)->num_of_rows > 0)
	{
		process_delete(last_row, last_row, tab, count);
	}
}

/* Performs deletion of a row by coping last_row to cur_row. */
void process_delete(row **cur_row, row **last_row, table **tab, int *count)
{
	if (*cur_row == *last_row)
	{
		(*tab)->num_of_rows--;
		(*tab)->tab_size -= (*tab)->row_size;
	}
	else
	{
		memset(*cur_row, '\0', (*tab)->row_size);
		memcpy(*cur_row, *last_row, (*tab)->row_size);
		(*tab)->num_of_rows--;
		(*tab)->tab_size -= (*tab)->row_size;
	}
	*last_row = (*last_row) - (*tab)->row_size;
	(*count)++;
}

/*************************************************************************************/
/*								UPDATE FUNCTIONS									 */
/*************************************************************************************/
/* Processes t_list and, if there are no errors, updates one
   or more records in specified table. */
int sem_update(token_list *t_list)
{
	int rc = 0;
	token_list *cur = NULL;
	tpd_entry *tab_entry = NULL;
	conditions cond, new_val;

	cur = t_list;
	memset(&new_val, '\0', sizeof(conditions));
	memset(&cond, '\0', sizeof(conditions));

	// check if table exists first
	if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
	{
		rc = TABLE_NOT_EXIST;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;
		if (cur->tok_value != K_SET)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
	}

	// get new values to be set
	if (!rc)
		rc = get_conditions(tab_entry, &cur, &new_val);

	// continue parsing update statement
	if (!rc)
	{
		if (cur->tok_value != K_WHERE && cur->tok_value != EOC)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
	}

	// get search conditions
	if (!rc)
	{
		if (cur->tok_value != EOC)
		{
			if ((rc = get_conditions(tab_entry, &cur, &cond)) == 0)
			{
				// cur->tok_value has to be EOC!
				if (cur->tok_value != EOC)
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
			}
		}

		// done parsing statement, update records
		if (!rc)
			rc = update_records(tab_entry, &new_val, &cond);
	}

	return rc;
}

/* Searches specified table for rows that satisfy conditions and updates them. */
int update_records(tpd_entry *tab_entry, conditions *new_val, conditions *cond)
{
	int rc = 0, count = 0, offset = 0, i;
	bool done = false;
	table *tab = NULL;
	row *cur_row = NULL;
	cd_entry *col = NULL;

	// get table from file
	if ((tab = read_tab_file(tab_entry->table_name)) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}

	// check if table is empty
	if (!rc)
	{
		if (!tab->num_of_rows)
		{
			printf("Query OK, 0 rows updated ");
			rc = EMPTY_TABLE;
		}
	}

	// if no conditions, update all rows
	if (!rc)
	{
		offset = calc_offset(tab_entry, *new_val->col_names);
		col = get_cd_entry(tab_entry, *new_val->col_names);	

		if (!(*cond->col_names))
		{
			cur_row = (row*)((char*)tab + sizeof(table));
			for (i = 0; i < tab->num_of_rows; i++)
			{
				if ((rc = process_update(&cur_row, new_val, col, &count, offset)) != 0)
					break;
				cur_row += tab->row_size;
			}
			done = true;
		}
	}

	// update rows that satisfy conditions
	if (!rc && !done)
	{	
		cur_row = (row*)((char*)tab + sizeof(table));	
		for (i = 0; i < tab->num_of_rows; i++)
		{
			if (satisfies_cond(cur_row, cond, tab_entry))
				if((rc = process_update(&cur_row, new_val, col, &count, offset)) != 0)
					break;

			cur_row += tab->row_size;
		}
	}

	// update tab file
	if (!rc && count != 0)
		rc = update_tab_file(tab, tab_entry->table_name);

	// print count
	if (!rc)
	{
		if (count == 0)
		{
			printf("Query OK, 0 rows updated ");
			rc = NO_ROWS_DELETED;
		}
		else if (count == 1)
		{
			printf("Query OK, 1 row updated ");
		}
		else
		{
			printf("Query OK, %d rows updated ", count);
		}
	}

	// free memory
	if (tab != NULL)
		free(tab);

	return rc;
}

/* Sets new value for specified column in a row */
int process_update(row **cur_row, conditions *new_val, cd_entry *col, int *count, int offset)
{
	int new_int, temp_int, rc = 0;
	char *temp_str;

	// check for null assignment
	if (stricmp("NULL", *new_val->values) == 0)
	{
		if (col->not_null)
		{
			printf("Error updating record! Column \'%s\' cannot be NULL\n", col->col_name);
			rc = NULL_ASSIGNMENT;
		}
		else
		{
			// check if column already not NULL already
			memcpy(&temp_int, (*cur_row) + offset - 1, 1);
			if (temp_int != MAX_TOK_LEN + 1)
			{
				memset((*cur_row) + offset - 1, MAX_TOK_LEN + 1, 1);
				memset((*cur_row) + offset, '\0', col->col_len);
				(*count)++;
			}
		}
	}
	// check new value length
	else if (strlen(*new_val->values) > col->col_len)
	{
		printf("Error: new value is longer than maximum allowed for this column\n");
		rc = INVALID_VALUE_LENGTH;
	}
	// update int
	else if (col->col_type == T_INT)
	{		
		new_int = atoi(*new_val->values);
		memcpy(&temp_int, (*cur_row) + offset, sizeof(int));

		// check if column already has correct value
		// if not, update it
		if (new_int != temp_int)
		{
			memset((*cur_row) + offset - 1, sizeof(int), 1);
			memset((*cur_row) + offset, '\0', sizeof(int));
			memcpy((*cur_row) + offset, &new_int, sizeof(int));
			(*count)++;
		}
	}
	// update string
	else
	{
		temp_str = (char *)malloc(col->col_len * sizeof(char));
		memcpy(temp_str, (*cur_row) + offset, col->col_len);

		// check if column already has correct value
		// if not, update it
		if (strcmp(temp_str, *new_val->values) != 0)
		{
			memset((*cur_row) + offset - 1, strlen(*new_val->values), 1);
			memset((*cur_row) + offset, '\0', col->col_len);
			memcpy((*cur_row) + offset, *new_val->values, strlen(*new_val->values));
			(*count)++;
		}
	}

	return rc;
}

/*************************************************************************************/
/*								SELECT FUNCTIONS									 */
/*************************************************************************************/
/* Processes t_list and, if there are no errors, prints out
   date from specified table. */
int sem_select(token_list *t_list)
{
	int rc = 0;
	char **pt, **select_col = NULL;
	bool stop = false;
	token_list *cur = NULL;
	tpd_entry *tab_entry = NULL;
	table *tab = NULL;
	cd_entry *col = NULL;
	conditions cond;
	function func;

	cur = t_list;
	memset(&cond, '\0', sizeof(conditions));
	memset(&func, '\0', sizeof(function));

	// get column names to be selected
	if (cur->tok_value == S_STAR)
	{
		// '*'
		cur = cur->next;
	}
	else if (cur->tok_class == function_name)
	{
		// function
		rc = get_function(&cur, &func);
	}
	else if (cur->tok_class == identifier)
	{
		// has to be columns
		select_col = (char **)malloc(MAX_NUM_COL);
		memset(select_col, '\0', MAX_NUM_COL); 
		pt = select_col;
		while(!rc && cur->tok_value != K_FROM)
		{	
			// check for duplicate columns
			if (unique_col(select_col, cur->tok_string))
			{
				*pt = (char *)malloc(strlen(cur->tok_string));
				memset(*pt, '\0', strlen(cur->tok_string));
				strcpy(*pt, cur->tok_string);
				pt++;
			}
			else
			{
				rc = DUPLICATE_COLUMN_NAME;
				cur->tok_value = INVALID;
				break;
			}

			if (cur->next->tok_value == EOC)
			{
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			}
			else
			{
				// skip comma
				cur = cur->next;
				if (cur->tok_value != K_FROM)
				{
					cur = cur->next;
					if (cur->tok_value == K_FROM)
					{
						rc = INVALID_STATEMENT;
						cur->tok_value = INVALID;
						break;
					}
				}
			}
		}
		*pt = (char *)malloc(1);
		memset(*pt, '\0', 1);
	}
	else
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}

	// check table name, if valid get tab_antry and read tab from file
	if (!rc)
	{
		cur = cur->next;
		if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
		{
			// table does not exist
			rc = TABLE_NOT_EXIST;
			cur->tok_value = INVALID;
		}
		else
		{
			if ((tab = read_tab_file(tab_entry->table_name)) == NULL)
			{
				// error reading tab file
				rc = FILE_OPEN_ERROR;
				cur->tok_value = INVALID;
			}
			else
			{
				cur = cur->next;
			}
		}
	}

	// check select_col they are all valid
	if (!rc && select_col)
	{
		pt = select_col;
		while (**pt)
		{
			if ((col = get_cd_entry(tab_entry, *pt)) == NULL)
			{
				rc = UNKNOWN_COLUMN;
				break;
			}
			pt++;
		}
	}

	// check if column passed to function is valid
	if (!rc)
	{
		if (*func.col_name)
		{
			if (*func.col_name == '*')
			{
				if (func.func_id == F_SUM || func.func_id == F_AVG)
				{
					// cannot sum() or ave() all columns
					rc = INVALID_PARAM_TO_FUNC;

					if (func.func_id == F_SUM)
						printf("\nError: Invalid parameter \'%s\' to function \'SUM()\'\n", func.col_name);
					else
						printf("\nError: Invalid parameter \'%s\' to function \'AVG()\'\n", func.col_name);
				}
			}
			else
			{	if ((col = get_cd_entry(tab_entry, func.col_name)) == NULL)
				{
					rc = UNKNOWN_COLUMN;
				}
				else
				{
					if ((func.func_id == F_SUM || func.func_id == F_AVG)
						&& col->col_type != T_INT)
					{
						// cannot sum() or ave() chars
						rc = INVALID_PARAM_TO_FUNC;

						if (func.func_id == F_SUM)
							printf("\nError: Invalid parameter \'%s\' to function \'SUM()\'\n", func.col_name);
						else
							printf("\nError: Invalid parameter \'%s\' to function \'AVG()\'\n", func.col_name);
					}
				}
			}
		}
	}

	// continue parsing select statement, get search conditions and order by
	if (!rc)
	{
		while (!rc && cur->tok_value != EOC)
		{
			if (cur->tok_value == K_ORDER)
			{
				// cannot have order by when using function in select statement
				if (*func.col_name)
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
				else
				{
					// proceed with order by
					rc = order_by(&tab, tab_entry, &cur);
					stop = true;
				}
			}
			else if (cur->tok_value == K_WHERE)
			{
				// get search conditions
				rc = get_conditions(tab_entry, &cur, &cond);
			}
			else 
			{
				// invalid statement
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			}
		}
	}

	if (!rc)
		print_tab(tab, tab_entry, select_col, &cond, &func);

	// free memory
	if (tab != NULL)
		free(tab);

	return rc;
}

/* Prints specified table accordint to selected columns and conditions */
void print_tab(table *tab, tpd_entry *tab_entry, char **select_col, conditions *con, function *func)
{
	int i, j, k, index, offset = 0, col_len, *pt_operators, *pt_and_or, cur_and_or, temp_int;
	char *line_separator, **pt, **pt_col_names, **pt_values, temp_str[MAX_TOK_LEN], row_str[MAX_ROW_SIZE * 2],
		 header[MAX_TOK_LEN];
	cd_entry *col;
	table *tab_to_print;
	row *r, *r2;
	bool copy;

	if (tab->num_of_rows == 0)
	{
		printf("Empty set ");
	}

	// check for function
	else if (*func->col_name)
	{
		line_separator = get_line_separator(tab_entry, select_col, func);

		memset(header, '\0', MAX_TOK_LEN);
		memset(temp_str, '\0', MAX_TOK_LEN);
		memset(row_str, '\0', MAX_ROW_SIZE);

		strcat(row_str, "| ");
		if (func->func_id == F_COUNT)
			strcat(header, "COUNT(");
		else if (func->func_id == F_SUM)
			strcat(header, "SUM(");
		else
			strcat(header, "AVG(");

		strcat(header, func->col_name);
		strcat(header, ")");
		strcat(row_str, header);
		strcat(row_str, " |\n");

		printf("%s", line_separator);
		printf("%s", row_str);
		printf("%s", line_separator);

		memset(row_str, '\0', MAX_ROW_SIZE);
		if (func->func_id == F_COUNT)
			sprintf(temp_str, "%d", get_count(tab, tab_entry, func->col_name, con));
		else if (func->func_id == F_SUM)
			sprintf(temp_str, "%d", get_sum(tab, tab_entry, func->col_name, con));
		else if (func->func_id == F_AVG)
			sprintf(temp_str, "%.2f", get_avg(tab, tab_entry, func->col_name, con));

		strcat(row_str, "| ");				
		for ( i = 0; i < strlen(header) - strlen(temp_str); i++)
			strcat(row_str, " ");
		strcat(row_str, temp_str);
		strcat(row_str, " |\n");

		printf("%s", row_str);
		printf("%s", line_separator);
		printf("1 row in set ");
	}
	else
	{
		line_separator = get_line_separator(tab_entry, select_col, func);
		if (!(*con->col_names))
		{
			// no conditions
			// print column headers
			printf("%s", line_separator);
			if (!select_col)
			{
				for (i = 0, col = (cd_entry*) ((char*)tab_entry + tab_entry->cd_offset); 
					i < tab_entry->num_columns; i++, col++)
				{
					col_len = strlen(col->col_name);
					if (i == 0)
						printf("| %s", col->col_name);
					else
						printf(" | %s", col->col_name);

					if (col_len < col->col_len)
					{
						for (j = 0; j < col->col_len - col_len; j++)
							printf(" ");
					}
				}
				printf(" |\n%s", line_separator);

				// print rows
				for (i = 0, r = (row *) tab + sizeof(table); i < tab->num_of_rows; i++)
				{	
					offset = 0;
					for (j = 0, col = (cd_entry*) ((char*)tab_entry + tab_entry->cd_offset);
						j < tab_entry->num_columns; j++, col++)
					{
						memset(temp_str, '\0', MAX_TOK_LEN);
						offset++;

						if (col->col_type == T_INT) 
						{
							printf("| ");
							memset(&temp_int, '\0', sizeof(int));
							memcpy(&temp_int, r + offset - 1, 1);
							if (temp_int == MAX_TOK_LEN + 1)
							{
								// This value is NULL
								int length = strlen(col->col_name);
								if (sizeof(int) > length)
									length = sizeof(int);

								for (k = 0; k < length + 1; k++)
									printf(" ");

							}
							else
							{
								memcpy(&temp_int, r + offset, sizeof(int));
								itoa(temp_int, temp_str, 10);

								if (strlen(col->col_name) <= sizeof(int))
								{
									col_len = sizeof(int);
								}
								else
								{
									col_len = strlen(col->col_name);
								}

								for ( k = 0; k < col_len - strlen(temp_str); k++)
									printf(" ");
								printf("%s ", temp_str);
							}

							/*
							*/

							offset += sizeof(int);
						}
						else
						{
							memcpy(temp_str, r + offset, col->col_len);

							if (strlen(col->col_name) > col->col_len)
							{
								col_len = strlen(col->col_name);
							}
							else
							{
								col_len = col->col_len;
							}

							printf("| %s ", temp_str);
							for ( k = 0; k < col_len - strlen(temp_str); k++)
								printf(" ");

							offset += col->col_len;
						}
					}
					r += tab->row_size;
					printf("|\n");
				}
			}
			else
			{
				// print column headers
				bool indent = false;

				for (i = 0, col = (cd_entry*) ((char*)tab_entry + tab_entry->cd_offset); i < tab_entry->num_columns; i++, col++)
				{
					pt = select_col;
					while (**pt)
					{
						if (!stricmp(col->col_name, *pt++))
						{
							col_len = strlen(col->col_name);
							if (!indent)
								printf("| %s ", col->col_name);
							else
								printf(" | %s", col->col_name);

							indent = false;

							if (col_len < col->col_len)
							{
								for (j = 0; j < col->col_len - col_len; j++)
									printf(" ");
							}
						}
					}
				}
				printf("|\n%s", line_separator);
				// print rows
				for (i = 0, r = (row *) tab + sizeof(table); i < tab->num_of_rows; i++)
				{	
					offset = 1;
					for (j = 0, col = (cd_entry*) ((char*)tab_entry + tab_entry->cd_offset);
						j < tab_entry->num_columns; j++, col++)
					{
						pt = select_col;
						while(**pt)
						{

							if (!stricmp(col->col_name, *pt++))
							{
								memset(temp_str, '\0', MAX_TOK_LEN);

								if (col->col_type == T_INT) 
								{
									memcpy(&temp_int, r + offset - 1, 1);
									if (temp_int == MAX_TOK_LEN + 1) 
									{
										col_len = sizeof(int);
									}
									else
									{
										memcpy(&temp_int, r + offset, sizeof(int));
										itoa(temp_int, temp_str, 10);
									}

									if (strlen(col->col_name) <= sizeof(int))
									{
										col_len = sizeof(int);
									}
									else
									{
										col_len = strlen(col->col_name);
									}

									printf("| ");
									for ( k = 0; k < col_len - strlen(temp_str); k++)
										printf(" ");
									printf("%s ", temp_str);
								}
								else
								{
									memcpy(temp_str, r + offset, col->col_len);

									if (strlen(col->col_name) > col->col_len)
									{
										col_len = strlen(col->col_name);
									}
									else
									{
										col_len = col->col_len;
									}

									printf("| %s ", temp_str);
									for ( k = 0; k < col_len - strlen(temp_str); k++)
										printf(" ");
								}
								break;
							}
						}
						if (col->col_type == T_INT)
							offset += sizeof(int) + 1;
						else
							offset += col->col_len + 1;
					}
					r += tab->row_size;
					printf("|\n");
				}
			}
			printf("%s", line_separator);

			if (tab->num_of_rows == 1)
				printf("1 row in set ");
			else 
				printf("%d rows in set ", tab->num_of_rows);
		}
		else
		{
			// conditons not null
			tab_to_print = (table *)malloc(tab->tab_size);
			memset(tab_to_print, '\0', tab->tab_size);
			tab_to_print->row_size = tab->row_size;

			r = (row *)((char *)tab + sizeof(table));
			r2 = (row *)((char *)tab_to_print + sizeof(table));

			for (i = 0; i < tab->num_of_rows; i++, r += tab->row_size)
			{
				if (satisfies_cond(r, con, tab_entry))
				{
					memcpy(r2, r, tab->row_size);
					r2 += tab->row_size;
					tab_to_print->num_of_rows++;
				}
			}
			*con->col_names = NULL; // wth is this??
			print_tab(tab_to_print, tab_entry, select_col, con, func);
		}
	}
}

/* Creates line separator to be used in print_tab functions */
/* Ex: +-------------+----------+-------------------------+ */
char *get_line_separator(tpd_entry *tab_entry, char **select_col, function *func)
{
	char line_separator[MAX_ROW_SIZE] = {0}, *result, **pt;
	cd_entry *col;
	int i, j, k = 0, size = 0, col_name_size;

	if (!select_col && !(*func->col_name))
	{
		// no columns
		for (i = 0, col = (cd_entry*) ((char*)tab_entry + tab_entry->cd_offset); i < tab_entry->num_columns; i++, col++)
		{
			size += 1;
			col_name_size = strlen(col->col_name);
			strcat(line_separator, "+-");

			if (col->col_type == T_INT)
			{
				if (col_name_size < sizeof(int))
				{
					size += sizeof(int) + sizeof(char) * 2;
					strcat(line_separator, "----");
				}
				else
				{
					size += col_name_size + sizeof(char) * 2;
					for (j = 0; j < col_name_size; j++)
						strcat(line_separator, "-");
				}
			}
			else
			{
				if (col_name_size < col->col_len)
				{
					size += col->col_len + sizeof(char) * 2;
					for (j = 0; j < col->col_len; j++)
						strcat(line_separator, "-");
				}
				else
				{
					size += col_name_size + sizeof(char) * 2;
					for (j = 0; j < col_name_size; j++)
						strcat(line_separator, "-");
				}
			}
			strcat(line_separator, "-");
		}
		size += 1;
		strcat(line_separator, "+\n");
	}
	else if (select_col)
	{
		for (i = 0, col = (cd_entry*) ((char*)tab_entry + tab_entry->cd_offset); i < tab_entry->num_columns; i++, col++)
		{
			pt = select_col;
			while(**pt)
			{
				if (!stricmp(col->col_name, *pt++))
				{
					k++;
					size += 1;
					col_name_size = strlen(col->col_name);
					strcat(line_separator, "+-");

					if (col->col_type == T_INT)
					{
						if (col_name_size < sizeof(int))
						{
							size += sizeof(int) + sizeof(char) * 2;
							strcat(line_separator, "----");
						}
						else
						{
							size += col_name_size + sizeof(char) * 2;
							for (j = 0; j < col_name_size; j++)
								strcat(line_separator, "-");
						}
					}
					else
					{
						if (col_name_size < col->col_len)
						{
							size += col->col_len + sizeof(char) * 2;
							for (j = 0; j < col->col_len; j++)
								strcat(line_separator, "-");
						}
						else
						{
							size += col_name_size + sizeof(char) * 2;
							for (j = 0; j < col_name_size; j++)
								strcat(line_separator, "-");
						}
					}
					strcat(line_separator, "-");
					break;
				}
			}
			size += 1;
		}
		strcat(line_separator, "+\n");
	}
	else if (*func->col_name)
	{
		if (func->func_id == F_COUNT)
		{ 
			strcat(line_separator, "+-------");
			for (i = 0; i < strlen(func->col_name); i++)
				strcat(line_separator, "-");
		}
		else if (func->func_id == F_SUM)
		{
			strcat(line_separator, "+-----");
			for ( i = 0; i < strlen(func->col_name); i++)
				strcat(line_separator, "-");
		}
		else if (func->func_id == F_AVG)
		{
			strcat(line_separator, "+-----");
			for ( i = 0; i < strlen(func->col_name); i++)
				strcat(line_separator, "-");
		}

		strcat(line_separator, "--+\n");
	}

	result = (char*) malloc(strlen(line_separator));
	strcpy(result, line_separator);

	return result;
}

/*************************************************************************************/
/*								FILE I/O FUNCTIONS									 */
/*************************************************************************************/
/* Creates new .dat file with specified name and default size of 8 bytes. */
int create_tab_file(char *tabname, int row_size)
{
	int rc = 0;
	FILE *fhandle = NULL;
	char fname[MAX_IDENT_LEN + 4];
	table tab;

	memset(fname, '\0', MAX_IDENT_LEN + 4);
	strcat(fname, tabname);
	strcat(fname, ".dat");

	if ((fhandle = fopen(fname, "wbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		memset(&tab, '\0', sizeof(table));

		tab.tab_size = sizeof(table);
		tab.num_of_rows = 0;
		tab.row_size = row_size;

		fwrite(&tab, sizeof(table), 1, fhandle);

		//fflush(fhandle);
		fclose(fhandle);
	}

	return rc;
}

/* Deletes .dat file for specified table. */
int delete_tab_file(char *tabname)
{
	int rc = 0;
	char fname[MAX_IDENT_LEN + 4];

	memset(fname, '\0', MAX_IDENT_LEN + 4);
	strcat(fname, tabname);
	strcat(fname, ".dat");

	if (remove(fname))
	{
		rc = FILE_DELETE_ERROR;
	}

	return rc;
}

/* Reads file for specified table and returns pointer to table. */
table *read_tab_file(char *tabname)
{
	int rc = 0;
	FILE *fhandle = NULL;
	struct _stat file_stat;
	table *tab = NULL;
	char fname[MAX_IDENT_LEN + 4]; // +4 for ".dat"

	memset(fname, '\0', MAX_IDENT_LEN + 4);
	strcat(fname, tabname);
	strcat(fname, ".dat");

	if ((fhandle = fopen(fname, "rbc")) == NULL)
	{
		printf("Error: unable to open %s!\n", fname);
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		_fstat(_fileno(fhandle), &file_stat);

		tab = (table*) calloc(1, file_stat.st_size);
		memset(tab, '\0', file_stat.st_size);
		fread(tab, file_stat.st_size, 1, fhandle);

		//fflush(fhandle);
		fclose(fhandle);
	}

	return tab;
}

/* Update specified table file with new table data. */
int update_tab_file(table *tab, char *tabname)
{

	int rc = 0;
	FILE *fhandle = NULL;
	char fname[MAX_IDENT_LEN + 4];

	memset(fname, '\0', MAX_IDENT_LEN + 4);
	strcat(fname, tabname);
	strcat(fname, ".dat");

	if ((fhandle = fopen(fname, "wbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		fwrite(tab, tab->tab_size, 1, fhandle);
	}


	//fflush(fhandle);
	fclose(fhandle);

	return rc;
}

/*************************************************************************************/
/*								COUNT(), SUM(), AVG() FUNCTIONS						 */
/*************************************************************************************/
/* Calculates number of entries that satisfy conditions 
   Accepts * or any valid column name 
   Null values ignored */
int get_count(table *tab, tpd_entry *tab_entry, char *col_name, conditions *cond)
{
	int i, count = 0, offset = 0, *temp_int;
	row *r = NULL;

	if (*col_name != '*')
			offset = calc_offset(tab_entry, col_name);

	if (!offset && !(*cond->col_names))
	{
		// '*' and no conditions, count equlas to number of rows in table
		count = tab->num_of_rows;
	}
	else
	{
		r = (row *)((char *)tab + sizeof(table));
		temp_int = (int *)malloc(sizeof(int));



		for (i = 0; i < tab->num_of_rows; i++)
		{
			if (!offset)
			{
				// '*' and conditions
				if (satisfies_cond(r, cond, tab_entry))
					count++;
			}
			else if (!(*cond->col_names))
			{
				// column name and no conditions
				memset(temp_int, '\0', sizeof(int));
				memcpy(temp_int, r + offset - 1, 1);

				if (*temp_int != MAX_TOK_LEN +1)
					count++;
			}
			else
			{
				// column name and conditions
				memset(temp_int, '\0', sizeof(int));
				memcpy(temp_int, r + offset - 1, 1);

				if (*temp_int != MAX_TOK_LEN +1)
					if (satisfies_cond(r, cond, tab_entry))
						count++;
			}

			r += tab->row_size;
		}
	}

	return count;
}

/*
	Calculates sum of entries that satisfy conditions
	Accepts only integer columns
	Null values ignored */
int get_sum(table *tab, tpd_entry *tab_entry, char *col_name, conditions *cond)
{
	int i, sum = 0, offset = 0, *temp_int;
	cd_entry *col = NULL;
	row *r = NULL;

	offset = calc_offset(tab_entry, col_name);

	r = (row *)((char *)tab + sizeof(table));
	temp_int = (int *)malloc(sizeof(int));

	for (i = 0; i < tab->num_of_rows; i++)
	{

		if (!(*cond->col_names))
		{
			// no conditions, add up all values that are not null
			memset(temp_int, '\0', sizeof(int));
			memcpy(temp_int, r + offset - 1, 1);

			if (*temp_int != MAX_TOK_LEN +1)
			{
				memset(temp_int, '\0', sizeof(int));
				memcpy(temp_int, r + offset, sizeof(int));
				sum += *temp_int;
			}
		}
		else
		{
			// add up values that satisfy conditions
			memset(temp_int, '\0', sizeof(int));
			memcpy(temp_int, r + offset - 1, 1);

			if (*temp_int != MAX_TOK_LEN +1)
			{
				if (satisfies_cond(r, cond, tab_entry))
				{
					memset(temp_int, '\0', sizeof(int));
					memcpy(temp_int, r + offset, sizeof(int));
					sum += *temp_int;
				}
			}
		}

		r += tab->row_size;
	}

	return sum;
}

/* Calculates average of entries that satisfy conditions 
   Accepts only integer columns
   Null values ignored */
double get_avg(table *tab, tpd_entry *tab_entry, char *col_name, conditions *cond)
{
	int i, sum = 0, offset = 0, count = 0, *temp_int;
	cd_entry *col = NULL;
	row *r = NULL;

	offset = calc_offset(tab_entry, col_name);

	r = (row *)((char *)tab + sizeof(table));
	temp_int = (int *)malloc(sizeof(int));

	for (i = 0; i < tab->num_of_rows; i++)
	{
		if (!(*cond->col_names))
		{
			// no conditions, add up all values that are not null
			memset(temp_int, '\0', sizeof(int));
			memcpy(temp_int, r + offset - 1, 1);

			if (*temp_int != MAX_TOK_LEN +1)
			{
				memset(temp_int, '\0', sizeof(int));
				memcpy(temp_int, r + offset, sizeof(int));
				sum += *temp_int;
				count++;
			}
		}
		else
		{
			// add up values that satisfy conditions
			memset(temp_int, '\0', sizeof(int));
			memcpy(temp_int, r + offset - 1, 1);

			if (*temp_int != MAX_TOK_LEN +1)
			{
				if (satisfies_cond(r, cond, tab_entry))
				{
					memset(temp_int, '\0', sizeof(int));
					memcpy(temp_int, r + offset, sizeof(int));
					sum += *temp_int;
					count++;
				}
			}
		}

		r += tab->row_size;
	}

	return count == 0 ? 0 : ((double)sum/count);
}

/*************************************************************************************/
/*								OTHER HELPER FUNCTIONS								 */
/*************************************************************************************/
/* Searches tpd_entry for specified column name. If a match found, returns 
   a pointer to that cd_entry, NULL otherwise. */
cd_entry *get_cd_entry(tpd_entry *tab_entry, char *col_name)
{
	int i;
	cd_entry *col_entry = NULL;

	for (i = 0, col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
		i < tab_entry->num_columns; i++, col_entry++)
	{
		if (!stricmp(col_entry->col_name, col_name))
			return col_entry;
	}
	printf("\nError: Unknown column \'%s\'\n", col_name);

	return NULL;
}

/* Calculates offset - integer value that represents number of bytes
   to skip in a row */
int calc_offset(tpd_entry *tab_entry, char *col_name)
{
	int offset = 1, i;
	bool found = false;
	cd_entry *cur = NULL;

	cur = (cd_entry *)((char *)tab_entry + tab_entry->cd_offset);
	while(!found)
	{
		if (!stricmp(cur->col_name, col_name))
		{
			found = true;
		}
		else
		{
			if (cur->col_type == T_INT)
				offset += sizeof(int) + 1;
			else
				offset += sizeof(char) * cur->col_len + 1;
		}
		cur++;
	}

	return offset;
}

/* Checks if all column names are valid columns in a table */
int valid_col_names(tpd_entry *tab_entry, char **col_names)
{
	cd_entry *col;
	char **pt;
	int i, num_of_col = 0, col_found = 0;

	pt = col_names;
	while(*pt++)
		num_of_col++;

	pt = col_names;
	while(*pt)
	{
		for (i = 0, col = (cd_entry*) ((char*)tab_entry + tab_entry->cd_offset); i < tab_entry->num_columns; i++, col++)
		{
			if (!stricmp(col->col_name, *pt))
			{
				col_found++;
				pt++;
				break;
			}

			if (i == tab_entry->num_columns - 1)
				pt++;
		}
	}

	return num_of_col == col_found ? 0 : INVALID_COLUMN_NAME;
}

/* Creates a struct with conditions */
int get_conditions(tpd_entry *tab_entry, token_list **t_list, conditions *cond)
{
	char **pt_col_names, **pt_values;
	int rc = 0, *pt_and_or, *pt_operators;
	token_list *cur;
	cd_entry *col;

	cur = *t_list;
	if (cur->next->tok_value == EOC)
	{
		// invalid statemen
		rc = INVALID_STATEMENT;
		cur->next->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;
		pt_col_names = cond->col_names;
		pt_values = cond->values;
		pt_and_or = cond->and_or;
		pt_operators = cond->operators;

		// loop through conditions until EOC, K_ORDER, K_WHERE, or !rc
		while(!rc)
		{
			// get column name
			// check if valid column name
			col = NULL;
			if ((col = get_cd_entry(tab_entry, cur->tok_string)) == NULL)
			{
				// invalid column name
				rc = UNKNOWN_COLUMN;
				cur->tok_value = INVALID;
			}
			else
			{
				*pt_col_names = (char *)malloc(strlen(cur->tok_string));
				memset(*pt_col_names, '\0', strlen(cur->tok_string));
				strcpy(*pt_col_names, cur->tok_string);				
				pt_col_names++;
				cur = cur->next;
			}

			// get operator
			if (!rc)
			{
				// check if valid operator
				if (!strchr(OPERATORS, *(cur->tok_string)))
				{
					// invalid operator
					rc = INVALID_OPERATOR;
					cur->tok_value = INVALID;
				}
				else
				{
					*pt_operators = cur->tok_value;
					pt_operators++;
					cur = cur->next;
				}
			}

			// get value
			if (!rc)
			{
				if (cur->tok_value == EOC)
				{
					// invalid statement
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
				else
				{
					if (cur->tok_value != INT_LITERAL && cur->tok_value != STRING_LITERAL &&
						cur->tok_value != K_NULL)
					{
						// invalid statement
						rc = INVALID_STATEMENT;
						cur->tok_value = INVALID;
					}
					else if (cur->tok_value == INT_LITERAL && col->col_type != T_INT)
					{
						// type mismatch
						rc = TYPE_MISMATCH;
						cur->tok_value = INVALID;	
					}
					else if (cur->tok_value == STRING_LITERAL && col->col_type != T_CHAR)
					{
						// type mismatch
						rc = TYPE_MISMATCH;
						cur->tok_value = INVALID;							
					}
					else
					{
						// valid value!
						*pt_values = (char *)malloc(strlen(cur->tok_string));
						memset(*pt_values, '\0', strlen(cur->tok_string));
						strcpy(*pt_values, cur->tok_string);
						pt_values++;
						cur = cur->next;
					}
				}
			}

			// get AND, OR
			if (!rc)
			{
				if (cur->tok_value == EOC || cur->tok_value == K_ORDER || cur->tok_value == K_WHERE)
				{
					// no more conditions, end loop here
					break;
				}
				else if (cur->tok_value == K_AND || cur->tok_value == K_OR)
				{
					*pt_and_or = cur->tok_value;
					pt_and_or++;
					cur = cur->next;
				}
				else
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
			}
		}
		*t_list = cur;
	}

	return rc;
}

/* Checks syntax of a query that has order by clows */
int order_by(table **tab, tpd_entry *tab_entry, token_list **t_list)
{
	int rc = 0;
	token_list *cur;
	cd_entry *col;

	cur = *t_list;
	if (cur->next->tok_value == EOC)
	{
		// invalid statement
		rc = INVALID_STATEMENT;
		cur->next->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;
		if (cur->tok_value != K_BY)
		{
			// invalid statement
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else
		{
			cur = cur->next;
			if (cur->tok_value == EOC)
			{
				// invalid statemen
				rc = INVALID_STATEMENT;
				cur->tok_value = INVALID;
			}
			else
			{
				if ((col = get_cd_entry(tab_entry, cur->tok_string)) == NULL)
				{
					// invalid column name
					rc = UNKNOWN_COLUMN;
					cur->tok_value = INVALID;
				}
				else
				{
					cur = cur->next;
					if (cur->tok_value == EOC)
					{
						// sort table ascending order
						sort(tab, tab_entry, col, K_ASCE);
					}
					else if (cur->tok_value == K_DESC)
					{
						if (cur->next->tok_value != EOC)
						{
							// invalid statement
							rc = INVALID_STATEMENT;
							cur->next->tok_value = INVALID;
						}
						else
						{
							// sort table descending order
							cur = cur->next;
							sort(tab, tab_entry, col, K_DESC);
						}
					}
				}
			}
		}
		*t_list = cur;
	}

	return rc;
}

/* Sorts table by specified column in asceding or descending order 
    */
void sort(table **tab, tpd_entry *tab_entry, cd_entry *col, int order)
{
	int rc = 0, i, j, temp_int1, temp_int2, offset;
	char *temp_str1, *temp_str2;
	table *pt_tab;
	row *r1, *r2;

	pt_tab = *tab;

	// check if table is empty or only has one row
	if (pt_tab->num_of_rows == 0 || pt_tab->num_of_rows == 1)
	{
		// nothing to sort
		return;
	}
	else
	{
		offset = calc_offset(tab_entry, col->col_name);
		for (i = 0;	i < pt_tab->num_of_rows; i++)
		{
			for (j = 0;j < pt_tab->num_of_rows - 1 - i; j++)
			{
				r1 = (row *)((char *)pt_tab + sizeof(table) + pt_tab->row_size * j);
				r2 = (row *)((char *)pt_tab + sizeof(table) + pt_tab->row_size * (j +1));

				if (col->col_type == T_INT)
				{
					memcpy(&temp_int1, r1 + offset, sizeof(int));
					memcpy(&temp_int2, r2 + offset, sizeof(int));

					if (order == K_DESC)
					{
						if (temp_int1 < temp_int2)
							swap(&r1, &r2, pt_tab->row_size);
					}
					else
					{
						if (temp_int1 > temp_int2)
							swap(&r1, &r2, pt_tab->row_size);
					}
				}
				else
				{
					temp_str1 = (char *)malloc(col->col_len);
					memset(temp_str1, '\0', col->col_len);
					memcpy(temp_str1, r1 + offset, col->col_len);

					temp_str2 = (char *)malloc(col->col_len);
					memset(temp_str2, '\0', col->col_len);
					memcpy(temp_str2, r2 + offset, col->col_len);

					if (order == K_DESC)
					{
						if (strcmp(temp_str1, temp_str2) < 0)
							swap(&r1, &r2, pt_tab->row_size);
					}
					else
					{
						if (strcmp(temp_str1, temp_str2) > 0)
							swap(&r1, &r2, pt_tab->row_size);
					}
				}
			}
		}
	}
}

/* Performs a swap of 2 rows */
void swap(row **r1, row **r2, int size)
{
	row *temp;
	temp = (row *)malloc(size);
	memset(temp, '\0', size);
	memcpy(temp, *r1, size);

	memset(*r1, '\0', size);
	memcpy(*r1, *r2, size);

	memset(*r2, '\0', size);
	memcpy(*r2, temp, size);
}

/* Checks if specified row satisfies conditions */
bool satisfies_cond(row *r, conditions *cond, tpd_entry *tab_entry)
{
	int i, offset, temp_int, cur_and_or = 0;
	int *pt_operators = NULL, 
		*pt_and_or = NULL;
	char *temp_str,
		 **pt_col_names = NULL,
		 **pt_values = NULL;
	cd_entry *col = NULL;
	bool satisfies;

	temp_str = (char *)malloc(MAX_TOK_LEN);
	pt_col_names = cond->col_names;
	pt_operators = cond->operators;
	pt_values = cond->values;
	pt_and_or = cond->and_or;
	satisfies = false;
	offset = 0;

	while (*pt_col_names)
	{
		col = (cd_entry *)((char *)tab_entry + tab_entry->cd_offset);
		for (i = 0; i < tab_entry->num_columns; i++, col++)
		{
			if (stricmp(col->col_name, *pt_col_names) == 0)
			{
				offset = calc_offset(tab_entry, col->col_name);
				// check for NULL
				if (!strcmp(*pt_values, "NULL"))
				{
					temp_int = 0;
					memcpy(&temp_int, r + offset - 1, 1);
					if (temp_int == MAX_TOK_LEN + 1)
						satisfies = true;
					else
						satisfies = false;
					break;
				}
				else if (col->col_type == T_INT)
				{
					temp_int = 0;
					memcpy(&temp_int, r + offset, sizeof(int));

					if ((*pt_operators == S_EQUAL) && 
						(temp_int == atoi(*pt_values)))
					{
						satisfies = true;
					}
					else if ((*pt_operators == S_LESS) && 
						(temp_int < atoi(*pt_values)))
					{
						satisfies = true;
					}
					else if ((*pt_operators == S_GREATER) && 
						(temp_int > atoi(*pt_values)))
					{
						satisfies = true;
					}
					else
					{
						satisfies = false;
					}
					break;
				}
				else
				{
					memset(temp_str, '\0', MAX_TOK_LEN);
					memcpy(temp_str, r + offset, col->col_len);

					if ((*pt_operators == S_EQUAL) && 
						(strcmp(temp_str, *pt_values) == 0))
					{
						satisfies = true;
					}
					else if ((*pt_operators == S_LESS) && 
						(strcmp(temp_str, *pt_values) < 0))
					{
						satisfies = true;
					}
					else if ((*pt_operators == S_GREATER) && 
						(strcmp(temp_str, *pt_values) > 0))
					{
						satisfies = true;
					}
					else
					{
						satisfies = false;
					}
					break;
				}	
			}

		}
		if (*pt_and_or)
		{
			cur_and_or = *pt_and_or;
			pt_and_or++;	
		}

		if (cur_and_or == K_AND && !satisfies)
		{
			// row does not satisfies conditions
			break;
		}
		if (cur_and_or == K_OR && satisfies)
		{
			// row satisfies conditions
			break;
		}

		pt_col_names++;
		pt_operators++;
		pt_values++;
	}

	return satisfies;
}

/* Creates a func struct, and checks select select function query
   syntax */
int get_function(token_list **t_list, function *func)
{
	int rc = 0, *pt_func_id = NULL;
	token_list *cur = NULL;
	char *pt_col_name = NULL;

	cur = *t_list;
	pt_func_id = &func->func_id;
	pt_col_name = func->col_name;
	// get function id
	*pt_func_id = cur->tok_value;

	// check if next token is S_LEFT_PAREN
	cur = cur->next;
	if (cur->tok_value == EOC)
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else if (cur->tok_value != S_LEFT_PAREN)
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;
	}

	// get column name
	if (!rc)
	{
		if (cur->tok_value == EOC)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else
		{
			strcpy(pt_col_name, cur->tok_string);
			cur = cur->next;
		}
	}

	// check if next token is S_RIGHT_PAREN
	if (!rc)
	{
		if (cur->tok_value == EOC)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else if (cur->tok_value != S_RIGHT_PAREN)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else
		{
			cur = cur->next;
		}
	}

	*t_list = cur;
	return rc;
}

/* Checks if specified column is unique in the list of selected column */
bool unique_col(char **select_col, char *col_name)
{
	char **pt;

	if (*select_col)
	{
		pt = select_col;
		while(*pt)
		{
			if (!stricmp(*pt++, col_name))
				return false;
		}
	}
	return true;
}

/*************************************************************************************/
/*								LOGGING FUNCTIONS									 */
/*************************************************************************************/
/* Returns current timestamp in form of a string yyyymmddhhmmss */
char *get_time_stamp()
{
	time_t t;
	struct tm *timeinfo;
	char buffer[TIME_STAMP_LENGTH + 1];

	time(&t);
	timeinfo = localtime(&t);
	strftime(buffer, TIME_STAMP_LENGTH + 1, "%Y%m%d%H%M%S", timeinfo);

	return buffer;
}

/* If db.log does not exist, it will be created */
int create_log_file()
{
	int rc = 0;
	FILE *fhandle = NULL;

	if ((fhandle = fopen(LOG_FILE, "a")) == NULL)
		rc = FILE_OPEN_ERROR;

	if (!rc)
		fclose(fhandle);

	return rc;
}

/* Adds one entry to log file */
int add_log_entry(char *query)
{
	FILE *fhandle = NULL;
	char *new_entry = NULL, *copy,*pt;
	int rc = 0;

	if ((fhandle = fopen(LOG_FILE, "a")) == NULL)
		rc = FILE_OPEN_ERROR;

	if (!rc)
	{
		// make a copy of query
		copy = (char *)malloc((strlen(query) + 1) *
			sizeof(char));
		strcpy(copy, query);

		pt = strtok(query, " ");
		// insert log entry for backup
		if (stricmp(pt, "backup") == 0)
		{
			new_entry = (char *)malloc((strlen("backup") + strlen(pt) + 2) *
				sizeof(char)); // 2 = 1 space + 1 new line character
			strcpy(new_entry, copy);
 		}
		// insert log entry for restore
		else if (stricmp(copy, "RF_START") == 0)
		{
			new_entry = (char *)malloc((strlen("RF_START") + 1) * sizeof(char));
			strcpy(new_entry, "RF_START\n");
		}
		// insert log entry for the rest of operations
		else
		{
			new_entry = (char *)malloc((strlen(copy) + TIME_STAMP_LENGTH + 4) * 
				sizeof(char)); //4 = 2 quotes + 1 space + new line character

			strcpy(new_entry, get_time_stamp());
			strcat(new_entry, " \"");
			strcat(new_entry, copy);
			strcat(new_entry, "\"\n");
		}
		fputs(new_entry, fhandle);
		fclose(fhandle);
	}

	return rc;
}

/*************************************************************************************/
/*								BACKUP FUNCTIONS									 */
/*************************************************************************************/
/* Parses backup statement */
int sem_backup(token_list *t_list)
{
	int rc = 0;
	char statement[BIG_LINE];
	token_list *cur = NULL;

	cur = t_list;
	if (cur->tok_value == EOC)
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else
	{
		if (cur->next->tok_value != EOC)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else
		{
			rc = backup(cur->tok_string);
		}
	}

	if (!rc)
	{
		// add log entry
		strcpy(statement, "BACKUP ");
		strcat(statement, cur->tok_string);
		strcat(statement, "\n");
		rc = add_log_entry(statement);
	}

	return rc;
}
/* Executes backup statement by appending dbfile.bin and all table files
   to one image file */
int backup(char *image_name)
{
	FILE *image_fp = NULL;
	table *tab = NULL;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables, i, rc = 0, size;

	// check if image file exists already
	// if not, create new image file and open it for append
	if ((image_fp = fopen(image_name, "r")) != NULL)
	{
		printf("Error: mage file \'%s\' already exists!\n", image_name);
		rc = IMAGE_FILE_ALREADY_EXISTS;
	}
	else if ((image_fp = fopen(image_name, "wbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}

	if (!rc)
	{
		// append tpd_list to backup image
		fwrite(&(g_tpd_list->list_size), sizeof(int), 1, image_fp);
		fwrite(g_tpd_list, g_tpd_list->list_size, 1, image_fp);
		// loop through table definitions in the tpd list,
		// read each table from file and append it to backup image
		i = 0;
		while (i < num_tables)
		{
			if ((tab = read_tab_file(cur->table_name)) == NULL)
			{
				printf("Error reading %s.dat\n", cur->table_name);
				rc = FILE_OPEN_ERROR;
				fclose(image_fp);
				remove(image_name);
				break;
			}
			else
			{

				// append 4 byte size to image file
				fwrite(&(tab->tab_size), sizeof(int), 1, image_fp);
				// append the entire table to image file
				fwrite(tab, tab->tab_size, 1, image_fp);
				cur = (tpd_entry*)((char*)cur + cur->tpd_size);
				i++;
			}
		}
	}

	// close image file
	if (image_fp != NULL)
		fclose(image_fp);
	// free memory
	if (tab != NULL)
		free(tab);

	if (!rc)
	{
		if (num_tables == 0)
			printf("Query OK, no tables to backup ");
		else if (num_tables == 1)
			printf("Query OK, 1 table imaged ");
		else
			printf("Query OK, %d tables imaged ", num_tables);
	}

	return rc;
}

/*************************************************************************************/
/*								RESTORE FUNCTIONS									 */
/*************************************************************************************/

/* Parses restore statement */
int sem_restore(token_list *t_list)
{
	FILE *fp;
	int rc = 0;
	char image_name[MAX_IDENT_LEN] = {""};
	char statement[BIG_LINE], old_log[MAX_IDENT_LEN + 5];
	token_list *cur = NULL;

	cur = t_list;
	if (cur->tok_value != K_FROM)
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;
		if (cur->tok_value == EOC)
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
		else
		{
			// has to be image name
			strcpy(image_name, cur->tok_string);

			if (cur->next->tok_value == EOC)
			{
				if ((rc = restore(image_name, K_RF)) == 0)
					if ((rc = toggle_db_flag()) == 0)
						add_log_entry("RF_START");
			}
			else
			{
				cur = cur->next;
				if (cur->tok_value != K_WITHOUT)
				{
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
				else
				{
					cur = cur->next;
					if (cur->tok_value != K_RF ||
						cur->next->tok_value != EOC)
					{
						rc = INVALID_STATEMENT;
						cur->tok_value = INVALID;
					}
					else
					{
						rc = restore(image_name, 0);
						// create new log file and copy log entries to it
						if (!rc)
						{
							strcpy(old_log, rename_log());
							rc = copy_log(old_log);
						}
					}
				}
			}
		}
	}

	return rc;
}

/* Executes restore by recovering dbfile.bin and all tables 
   and writing them to disk */
int restore(char *image_name, int rf)
{
	FILE *image_fp = NULL;
	FILE *fp = NULL;
	tpd_entry *cur = NULL;
	table *tab = NULL;
	int i, rc = 0, size, num_tables, offset;
	char fname[MAX_IDENT_LEN + 5];

	if ((image_fp = fopen(image_name, "rbc")) == NULL)
	{
		printf("Error: unable to open %s image file!\n", image_name);
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		// get dbfile.bin size
		fread(&size, sizeof(int), 1, image_fp);
		// restore dbfile.bin
		fread(g_tpd_list, size, 1, image_fp);
		if ((fp = fopen(DB_FILE, "wbc")) == NULL)
		{
			printf("Error: unable to open %s!\n", DB_FILE);
			rc = FILE_OPEN_ERROR;
		}
		else 
		{
			strcpy(g_tpd_list->image_name, image_name);
			strcat(g_tpd_list->image_name, "\0");
			fwrite(g_tpd_list, size, 1, fp);
		}

		// loop through g_tpd_list and restore each table
		if (!rc)
		{
			num_tables = g_tpd_list->num_tables;
			for (i = 0, cur = &(g_tpd_list->tpd_start); 
				i < num_tables; i++, cur = (tpd_entry*)((char*)cur + cur->tpd_size))
			{
				size = 0;
				fread(&size, sizeof(int), 1, image_fp);

				// get table name
				strcpy(fname, cur->table_name);
				strcat(fname, ".dat\0");

				// read table data from backup image and write it to disk
				tab = (table *)malloc(size);
				memset(tab, '\0', size);
				fread(tab, size, 1, image_fp);

				if ((rc = update_tab_file(tab, cur->table_name)) != 0)
				{
					// break out of the loop if file update was unsuccessfull
					break;
				}
				// free memory
				free(tab);
			}
		}
	}

	if (image_fp)
		fclose(image_fp);
	if (fp)
		fclose(fp);

	if (!rc)
	{
		if (num_tables == 0)
			printf("Query OK, no tables to restore ");
		else if (num_tables == 1)
			printf("Query OK, 1 table restored ");
		else
			printf("Query OK, %d tables restored ", num_tables);

		strcpy(g_tpd_list->image_name, image_name);
	}

	return rc;
}

/* Attamps to rename db.log file to db<n>.log, where n is some integer */
char *rename_log()
{
	FILE *fhandle = NULL;
	int i;
	char fname[MAX_IDENT_LEN], temp_str[MAX_IDENT_LEN];
	bool done = false;

	i = 1;
	while (!done)
	{
		// get new file name
		memset(fname, '\0', MAX_IDENT_LEN);
		strcpy(fname, LOG_FILE);
		itoa(i, temp_str, 10);
		strcat(fname, temp_str);

		// try to open file, if fopen returns NULL 
		// rename log file to the current file name!
		if ((fhandle = fopen(fname, "r")) == NULL)
		{
			rename(LOG_FILE, fname);
			done = true;
		}
		i++;
	}

	return fname;
}

/* Find an offset from the begging of db.log file to the specified 
   backup entry */
int get_backup_offset(long *offset, char *log)
{
	FILE *fp = NULL;
	char line[BIG_LINE] = {""}, temp_str[MAX_TOK_LEN] = {""}, *token = NULL;
	long temp_offset = 0;
	int rc = 0;
	bool found = false;

	if ((fp = fopen(log, "r")) == NULL)
	{
		printf("Error reading %s file!\n", log);
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		while(fgets(line, 2048, fp))
		{	
			strncpy(temp_str, line, 31);
			token = strtok(temp_str, " ");

			if (stricmp(token, "BACKUP") == 0)
			{
				token = strtok(NULL, " ");
				char *new_line_pt = strchr(token, '\n');
				*new_line_pt = '\0';

				if (!stricmp(token, g_tpd_list->image_name))
				{
					*offset = temp_offset;
					found = true;
					break;
				}
			}
			temp_offset = ftell(fp);
		}

		if (!found)
		{
			// no backup entries found!
			fseek(fp, 0, SEEK_END);
			*offset = ftell(fp);
			fclose(fp);
		}
		else
		{
			rewind(fp);
			fseek(fp, *offset, SEEK_SET);
			fgets(line, 2048, fp);
			*offset = ftell(fp);
			fclose(fp);
		}
	}

	return rc;
}

/* Executes log entries from BACKUP entry until the end
   of log file, or specified timestamp. 
   Only DROP, CREATE, INSERT, DELETE, UPDATE statements
   are considerted for execution */
int redo_logged_transactions(char *time_stamp)
{
	FILE *old_log = NULL;
	int count = 0, rc = 0;
	long offset = 0;
	char line[BIG_LINE] = {""}, *query = NULL, log[MAX_IDENT_LEN + 4], stamp[TIME_STAMP_LENGTH];

	strcpy(log, rename_log());
	if ((rc = copy_log(log)) == 0)
	{
		if ((old_log = fopen(log, "r")) == NULL)
		{
			printf("Error reading %s file!\n", log);
			rc = FILE_OPEN_ERROR;
		}
		else
		{
			if ((rc = get_backup_offset(&offset, log)) == 0)
			{
				fseek(old_log, offset, SEEK_SET);
				while(fgets(line, 2048, old_log))
				{
					if (time_stamp)
					{
						if (strstr(line, time_stamp))
							break;
					}
					if ((query = stristr(line, "insert")) ||
						(query = stristr(line, "update")) ||
						(query = stristr(line, "delete")) ||
						(query = stristr(line, "create")) ||
						(query = stristr(line, "drop")))
					{
						*(query + strlen(query) - 2) = '\0';
						if ((rc = process_input(query)) == 0)
							count++;
						else 
							break;
					}
					else if (stristr(line, "RF_START"))
					{
						break;
					}
				}
			}
			fclose(old_log);
		}
	}
	if (!rc) 
		printf("%d queries executed! ", count);

	return rc;
}

/* My implementations of case insensitive strstr(). It returns 
   a pointer to the first accurance of str2 in str1, or null if
   str2 is not a substring of str1. 
   This functions is used in execute_queries() when parsing db.log
   file. */
char *stristr(char *str1, char *str2)
{
	char *result = NULL, *p1, *p2;
	int found = 1;

	if (str2 == NULL)
		return str1;

	for (p1 = str1, p2 = str2; *p1; p1++)
	{
		if (toupper(*p1) == toupper(*p2))
		{
			result = p1;
			found = 1;
			for (; *p1, *p2; p1++, p2++)
			{
				if (toupper(*p1) != toupper(*p2))
				{
					p2 = str2;
					found = 0;
					break;
				}
			}

			if (found)
			{
				break;
			}
			else if (*p1)
			{
				result = NULL;
			}
			else
			{
				result = NULL;
				break;
			}


		}
	}

	return result;
}

/* */
int copy_log(char *log)
{
	FILE *new_log = NULL;
	FILE *old_log = NULL;
	int rc = 0;
	long offset;
	char line[BIG_LINE];

	// create new db.log
	if ((new_log = fopen(LOG_FILE, "a")) == NULL)
	{
		printf("Error: unable to create %s\n", LOG_FILE);
		rc = FILE_OPEN_ERROR;
	}
	// open old log file for copy
	if ((old_log = fopen(log, "r")) == NULL)
	{
		printf("Error: unable to open %s\n", log);
		rc = FILE_OPEN_ERROR;
	}

	if (!rc)
	{
		rc = get_backup_offset(&offset, log);
		long end = ftell(old_log) + offset;
		if (!rc)
		{
			// copies old log entries to new log
			while (ftell(old_log) < end)
			{
				fgets(line, BIG_LINE, old_log);
				fputs(line, new_log);
			}
		}
	}

	if (new_log)
		fclose(new_log);
	if (old_log)
		fclose(old_log);

	return rc;
}

/* */
int toggle_db_flag()
{
	FILE *fp = NULL;
	int rc = 0;

	if (g_tpd_list->db_flags == 0)
		g_tpd_list->db_flags = ROLLFORWARD_PENDING;
	else
		g_tpd_list->db_flags = 0;

	if ((fp = fopen(DB_FILE, "wbc")) == NULL)
	{
		printf("Error: unable to open %s!\n", DB_FILE);
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		fwrite(g_tpd_list, g_tpd_list->list_size, 1, fp);
		fclose(fp);
	}

	return rc;
}

/* */
int sem_rollforward(token_list *t_list)
{
	int rc = 0;
	long offset = 0;
	char *time_stamp = NULL;
	token_list *cur = t_list;

	// no timestamp specified
	if (cur->tok_value == EOC)
	{
		time_stamp = NULL;
	}
	else if (cur->tok_value == K_TO)
	{
		cur = cur->next;
		if (cur->tok_value == INT_LITERAL)
		{
			if (cur->next->tok_value != EOC)
			{
				rc = INVALID_STATEMENT;
				cur->next->tok_value = INVALID;
			}
			else
			{
				if (strlen(cur->tok_string) == TIME_STAMP_LENGTH)
				{
					time_stamp = (char *)malloc(TIME_STAMP_LENGTH);
					strcpy(time_stamp, cur->tok_string);
				}
				else
				{
					rc = INVALID_TIME_STAMP;
					cur->tok_value = INVALID;
				}
			}
		}
		else
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
		}
	}
	else 
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}

	if (!rc)
		if ((rc = toggle_db_flag()) == 0)
			rc = redo_logged_transactions(time_stamp);

	return rc;
}