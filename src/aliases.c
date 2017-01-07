/*
 * aliases.c
 */

#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <memory.h>

#include "config.h"
#include "player.h"
#include "fix.h"
#include "aliases.h"

/* externs */

extern char    *upper_from_saved(saved_player * sp);
extern char    *check_legal_entry(player *, char *, int);
extern char    *store_string(), *store_int();
extern char    *get_string(), *get_int();
extern char    *gstring_possessive(), *end_string(), *next_space();
extern player  *find_player_global_quiet(char *), *find_player_global(char *);
extern player  *find_player_absolute_quiet(char *);
extern saved_player *find_saved_player(char *);
extern void     log(char *, char *);
extern void     tell_player(player *, char *);
extern char    *number2string(int);
extern char    *full_name(player *);
extern char    *get_gender_string(player *);
extern void     save_player(player *);
extern void     pager(player *, char *, int);
extern int      global_tag(player *, char *);
extern void     cleanup_tag(player **, int);

extern saved_player **saved_hash[];
int	strnomatch(char *, char *, int);


/* delete and entry from someones list */

void            delete_entry_alias(saved_player * sp, alias * l)
{
   alias       *scan;
   if (!sp)
      return;
   scan = sp->alias_top;
   if (scan == l)
   {
      sp->alias_top = l->next;
      FREE(l);
      return;
   }
   while (scan)
      if (scan->next == l)
      {
    scan->next = l->next;
    FREE(l);
    return;
      } else
    scan = scan->next;
   log("error", "Tried to delete alias that wasn't there.\n");
}


/* compress list */

void            tmp_comp_alias(saved_player * sp)
{
   char           *oldstack;
   alias          *l, *next;

   l = sp->alias_top;

   oldstack = stack;
   stack = store_int(stack, 0);

   while (l)
   {
      next = l->next;
      if (!l->cmd[0])
      {
    log("error", "Bad list entry on compress .. auto deleted.\n");
    delete_entry_alias(sp, l);
      } else
      {
    stack = store_string(stack, l->cmd);
    stack = store_string(stack, l->sub);
      }
      l = next;
   }
   store_int(oldstack, ((int) stack - (int) oldstack));
}
/* */
void            compress_alias(saved_player * sp)
{
   char           *oldstack;
   int             length;
   alias       *new, *l, *next;
   if (sp->system_flags & COMPRESSED_ALIAS)
      return;
   sp->system_flags |= COMPRESSED_ALIAS;
   oldstack = stack;
   tmp_comp_alias(sp);
   length = (int) stack - (int) oldstack;
   if (length == 4)
   {
      sp->alias_top = 0;
      stack = oldstack;
      return;
   }
   new = (alias *) MALLOC(length);
   memcpy(new, oldstack, length);

   l = sp->alias_top;
   while (l)
   {
      next = l->next;
      FREE(l);
      l = next;
   }
   sp->alias_top = new;
   stack = oldstack;
}

/* decompress list */

void            decompress_alias(saved_player * sp)
{
   alias          *l;
   char           *old, *end, *start;
   int             length;

   if (!(sp->system_flags & COMPRESSED_ALIAS))
      return;
   sp->system_flags &= ~COMPRESSED_ALIAS;

   old = (char *) sp->alias_top;
   start = old;
   if (!old)
      return;
   old = get_int(&length, old);
   end = old + length - 4;
   sp->alias_top = 0;
   while (old < end)
   {
      l = (alias *) MALLOC(sizeof(alias));
      old = get_string(stack, old);
      strncpy(l->cmd, stack, MAX_NAME - 3);
      old = get_string(stack, old);
      strncpy(l->sub, stack, MAX_DESC - 3);
      l->next = sp->alias_top;
      sp->alias_top = l;
   }
   FREE(start);
}



/* save list */

void            construct_alias_save(saved_player * sp)
{
   int             length;
   char           *where;

   if (!(sp->system_flags & COMPRESSED_ALIAS) &&
       (!find_player_absolute_quiet(sp->lower_name)))
      compress_alias(sp);

   if (sp->system_flags & COMPRESSED_ALIAS)
   {
      if (sp->alias_top)
      {
    where = (char *) sp->alias_top;
    (void) get_int(&length, where);
    memcpy(stack, where, length);
    stack += length;
      } else
    stack = store_int(stack, 4);
   } else
      tmp_comp_alias(sp);
}

/* retrieve list */

char           *retrieve_alias_data(saved_player * sp, char *where)
{
   int             length;
   (void) get_int(&length, where);
   if (length == 4)
      sp->alias_top = 0;
   else
   {
      sp->system_flags |= COMPRESSED_ALIAS;
      sp->alias_top = (alias *) MALLOC(length);
      memcpy(sp->alias_top, where, length);
   }
   where += length;
   return where;
}

/* count list entries */

int             count_alias(player * p)
{
   alias       *l;
   int             count = 0;
   if (!p->saved)
      return 0;
   if (!p->saved->alias_top)
      return 0;
   for (l = p->saved->alias_top; l; l = l->next)
      count++;
   return count;
}

/* find list entry for a person */

alias       *find_alias_entry(player * p, char *name)
{
   alias       *l;

   if (!p->saved)
      return 0;
   decompress_alias(p->saved);
   l = p->saved->alias_top;
   while (l)
      if (!strcasecmp(l->cmd, name))
         return l;
      else
         l = l->next;
   return 0;
}

/* create a list entry */

alias       *create_entry_alias(player * p, char *name)
{
   alias       *l;

   if (!p->saved)
      return 0;
   if ((count_alias(p)) >= (p->max_alias))
   {
      tell_player(p, " Can't create new alias, "
                     "because your alias list is full.\n");
      return 0;
   }
   l = (alias *) MALLOC(sizeof(alias));
   strncpy(l->cmd, name, MAX_NAME - 3);
   strncpy(l->sub, "", MAX_DESC - 3);
   
   l->next = p->saved->alias_top;
   p->saved->alias_top = l;
   return l;
}

/* view alias list */

void            view_alias(player * p, char *str)
{
  char           *oldstack, li[]="Logon Script", lo[]="Logoff Script", re[]="Reconnect Script";
  alias          *l, *logon = 0, *logoff = 0, *recon = 0;
  int             count;
  
  oldstack = stack;
  if (*str)
    {
      tell_player(p, " Format: lsa  (lists all aliases)\n");
      return;
    }
  
  if (!p->saved)
    {
      tell_player(p, " You have no alias list to view.\n");
      return;
    }
  count = count_alias(p);
  sprintf(stack, " You are using %d of your maximum %d aliases.\n",
	  count, p->max_alias);
  stack = strchr(stack, 0);
  if (count)
    {
      for (l = p->saved->alias_top; l; l = l->next)
	{
	    if (!strcmp(l->cmd, "_logon"))
		logon = l;
	    else if (!strcmp(l->cmd, "_logoff"))
		logoff = l;
	    else if (!strcmp(l->cmd, "_recon"))
		recon = l;
	    else 
	    {
	      sprintf(stack, "^B%-20s^N = %s", l->cmd, l->sub);
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	}
	if (logon || logoff || recon) {
		strcpy(stack, "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
     		stack = strchr(stack, 0);
		if (logon) 
	    {
	      sprintf(stack, "^B%-20s^N = %s", li, logon->sub);
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
		if (logoff) 
	    {
	      sprintf(stack, "^B%-20s^N = %s", lo, logoff->sub);
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
		if (recon) 
	    {
	      sprintf(stack, "^B%-20s^N = %s", re, recon->sub);
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	}
    }
  *stack++ = 0;
  pager(p, oldstack, 0);
  stack = oldstack;
}

/* undefine a macro? */

void            undefine_alias(player * p, char *str)
{
   char           *oldstack, *text, msg[50];
   int             count = 0;
   alias          *l;

   oldstack = stack;

   if (!p->saved)
   {
      tell_player(p, " You do not have an alias list to alter, "
                     "since you are not saved\n");
      return;
   }
   if (!*str)
   {
      tell_player(p, " Format: undefine <alias to undefine>\n");
      return;
   }
      {
         l = find_alias_entry(p, str);
         if (!l)
         {
            text = stack;
            sprintf(stack, " Can't find any alias list entry for '%s'.\n", str);
            stack = end_string(stack);
            tell_player(p, text);
         } else
         {
            count++;
            sprintf(msg, " Entry removed for '%s'\n", l->cmd);
            tell_player(p, msg);
            delete_entry_alias(p->saved, l);
         }
      }
      stack = oldstack;
   if (!count)
      tell_player(p, " No entries removed.\n");
   else
   {
      sprintf(stack, " Deleted alias.\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
   }
   stack = oldstack;
}

/* define an alias -- woowoo */

void            define_alias(player * p, char *str)
{
   char           *doh, *oldstack, *scanned;
   int             count = 0;
   alias          *l;

   if (!p->saved)
   {
      tell_player(p, " You can't alias because you have no save file.\n");
      return;
   }
   oldstack = stack;
   doh = next_space(str);
   if (!*doh)
   {
      tell_player(p, " Format: define <alias> <command to do instead>\n");
      return;
   }
   *doh++ = 0;
   /* strip extra spaces from str */
   scanned = str;
   while (*str && !(isspace(*str)))
	*str++; 
   *str++ = 0;
   str = scanned;

   l = find_alias_entry(p, str);
   if (!l)
   l = create_entry_alias(p, str);
   if (l) {
        count++;
	strncpy(l->sub, doh, MAX_DESC - 3);
   }
   if (count)
      tell_player(p, " Alias defined.\n");
   else
      tell_player(p, " Error - could not create alias.\n");
}

char           *do_alias_match(player * p, char *str)
{
   char           *t;
   alias          *scan;
   int            g;

   if (!p->saved)
	return "\n";
   scan = p->saved->alias_top;
   while (scan) {
      g = 1;
      if (strnomatch(scan->cmd, str, 0))
		g=0;
      if (g) {
         while (*str && *str != ' ')
            str++;
	 while (*str && isspace(*str))	
	    str++;
	 return str;
	 }
      scan = scan->next;
   }
   return "\n";
}

alias           *get_alias(player * p, char *str)
{
   char           *t;
   alias          *scan;
   int            g;

   if (!p->saved)
	return 0;
   scan = p->saved->alias_top;
   while (scan) {
      g = 1;
      if (strnomatch(scan->cmd, str, 0))
		g=0;
      if (g) {
         return scan;
	 }
      scan = scan->next;
   }
   return 0;
}

char	*splice_argument(player *p, char *str, char *arg, int conti) {

	static char  	BUFFER[1000], arghold[1000], strhold[1000], num[2];
	static char     RARG[10][100];
	static int  	b, a, s;
	int		no, word, r1, r2;

	if (!(conti)) {   /* reset variables */
		s=0;
		a=0;
		if (*arg)
			strncpy(arghold, arg, 999);
		else
			strncpy(arghold, "", 999);
		strncpy(strhold, str, 999);
		num[0] = 0;
		num[1] = 0;
		}
	for (b=0; b<1000; b++)
		BUFFER[b] = 0;
	b=0;
	
	while (1) {

		if (!strhold[s]) { /* true end of function */
			BUFFER[b++] = 0;
			return BUFFER;
		     }
		else if (strhold[s] != '%') { 
			BUFFER[b++] = strhold[s++];
		     }
		else {
		     if (strhold[s+1] == ';') {
			BUFFER[b++] = 0;
			s+=2;
			return BUFFER;
		        }
		     else if (strhold[s+1] == '%') {
			BUFFER[b++] = '%';
			s+=2;
		        }
		     else if (strhold[s+1] == '0') {
			s+=2;
			a=0;
			while (arghold[a])
				BUFFER[b++] = arghold[a++];
			}
		     else if (strhold[s+1] == '-') {
			s+=3;
			num[0] = strhold[s-1];
			num[1] = 0;
			no = atoi(num);
			a=0;
			for (word=0; word != no; a++) {
				if (!arghold[a])  {
					word=no;
					}
				else if (isspace(arghold[a]))
					word++;  			
				}
			while (arghold[a])
				BUFFER[b++] = arghold[a++];
			}
		     else if (isdigit(strhold[s+1])) {
			s+=2;
			num[0] = strhold[s-1];
			num[1] = 0;
			no=atoi(num);
			a=0;
			for (word=1; word != no; a++) {
		
				if (!arghold[a]) {
					word=no;	
					}
				else if (isspace(arghold[a]))
					word++;  			
				}
			while (arghold[a] && !(isspace(arghold[a])))
				BUFFER[b++] = arghold[a++];
			}
		     else if (strhold[s+1] == '{') 
			{
			/* first, clear all the old rargs */
			for (r1=0; r1 < 10; r1++)
				for (r2=0; r2 < 100; r2++)
					RARG[r1][r2] = 0;	
			
			/* now, fill in the data into RARG */
			r1 = 0;
			r2 = 0;
			s+=2; 
			while (strhold[s] != '}') {
				if (r1 < 10 && r2 < 99) {
					if (strhold[s] == '@') {
						RARG[r1++][r2] = 0;
						r2 = 0;
						}
					else
						RARG[r1][r2++] = strhold[s];
					}
				else if (r1 < 10) {
					RARG[r1++][r2] = 0;
					r2 = 0;	
				}
				s++;
				}	
			/* this should ALWAYS (finger crossed) be in range */
			RARG[r1][r2] = 0;
				
			/* now we have args -- choose one at random */
			s++; /* get the s to the space after the %{...} */
			r2 = (rand() % (r1 + 1)); 
			r1 = 0;
			while (RARG[r2][r1]) {
				BUFFER[b++] = RARG[r2][r1++];	
			  }
			}
		     else
			s+=2;
		}
	}
	return "";
}


int strnomatch(char *str1, char *str2, int unanal) {

	char *s1p, *s2p;

	s1p = str1;
	s2p = str2;

	for (;*s1p;s1p++,s2p++) {
		if (unanal && *s1p != *s2p)
			return 1;
		else
		   if (tolower(*s1p) != tolower(*s2p))
			return 1;
		}
	if (!unanal && *s2p && (!isspace(*s2p)))
			return 1;
	return 0;
}	




/* view someone else's alias list, if you are Admin */

void            view_others_aliases(player * p, char *str)
{
   char           *oldstack, li[] = "Login", lo[]="Logout", re[] = "Reconnect";
   int             count;
   player         *p2, dummy;
   alias          *l, *logon = 0, *logoff = 0, *recon = 0;

   oldstack = stack;
   memset(&dummy, 0, sizeof(player));

   if (!*str)
   {
      tell_player(p, " Format: val <player>\n");
      return;
   }

   lower_case(str);
   p2 = find_player_global_quiet(str);
   if (!p2)
   {
      strcpy(dummy.lower_name, str);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
      {
         tell_player(p, " That person does not exist!\n");
         return;
      }
      p2=&dummy;
   }

   if (!p2->saved)
   {
       tell_player(p, " That person has no alias list to view.\n");
       return;
   }
   count = count_alias(p2);
   sprintf(stack, "%s is using %d of %s maximum %d alias list entries.\n",
      p2->name, count, their_player(p2), p2->max_alias);
   stack = strchr(stack, 0);

  if (count)
    {
      for (l = p2->saved->alias_top; l; l = l->next)
	{
	    if (!strcmp(l->cmd, "_logon"))
		logon = l;
	    else if (!strcmp(l->cmd, "_logoff"))
		logoff = l;
	    else if (!strcmp(l->cmd, "_recon"))
		recon = l;
	    else 
	    {
	      sprintf(stack, "^B%-20s^N - %s", l->cmd, l->sub);
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	}

	if (logon || logoff || recon) {
		strcpy(stack, "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
     		stack = strchr(stack, 0);
		if (logon) 
	    {
	      sprintf(stack, "^B%-20s^N = %s", li, logon->sub);
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
		if (logoff) 
	    {
	      sprintf(stack, "^B%-20s^N = %s", lo, logoff->sub);
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
		if (recon) 
	    {
	      sprintf(stack, "^B%-20s^N = %s", re, recon->sub);
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	}
    }
  *stack++ = 0;
   pager(p, oldstack, 0);
   stack = oldstack;
}


void 	define_logon_macro(player * p, char *str) {

	char *oldstack;

	if (!*str) {

		undefine_alias(p, "_logon");
		return;		
		}
	oldstack = stack;
	sprintf(stack, "_logon %s", str);
	stack = end_string(stack);
	define_alias(p, oldstack);
	stack = oldstack;
}

void 	define_logoff_macro(player * p, char *str) {

	char *oldstack;

	if (!*str) {

		undefine_alias(p, "_logoff");
		return;		
		}
	oldstack = stack;
	sprintf(stack, "_logoff %s", str);
	stack = end_string(stack);
	define_alias(p, oldstack);
	stack = oldstack;
}

void 	define_recon_macro(player * p, char *str) {

	char *oldstack;

	if (!*str) {

		undefine_alias(p, "_recon");
		return;		
		}
	oldstack = stack;
	sprintf(stack, "_recon %s", str);
	stack = end_string(stack);
	define_alias(p, oldstack);
	stack = oldstack;
}

void 	library_list(player * p, char *str) {

	char *oldstack;
	int i = 0;
	
	oldstack = stack;
	strcpy(stack, " Listing all aliases currently in the library.\n");
	stack = strchr(stack, 0);
	strcpy(stack, " Please submit your favorite aliases that you'd like everyone to share by internal mail to traP.\n");
	stack = strchr(stack, 0);

	while (library[i].command)
	{
	if (!(library[i].privs) || p->residency & PSU) {
	   sprintf(stack, "%s, ", library[i].command);
	   stack = strchr(stack, 0);
	   }
	i++;
	}
	stack-=2;
	*stack++ = '\n';
	*stack++ = 0;
	
	tell_player(p, oldstack);
	stack = oldstack;
}	

alias_library get_lib_alias(char *str) {

	int i = 0;

	while (library[i].command) {
	
		if (!strcasecmp(str, library[i].command))
			return library[i];
		i++;
		}

	return no_library_here;
}

void library_copy(player * p, char *str) {

	char *oldstack, *new_alias_name;
	alias_library to_be_copied;

	if (!*str) {
		tell_player(p, " Format: libcopy <library alias> [<Your name for alias>]\n");
		return;
		}

	new_alias_name = next_space(str);
	if (*new_alias_name)
		*new_alias_name++ = 0;
	
	to_be_copied = get_lib_alias(str);

	if (!to_be_copied.command || (to_be_copied.privs && !(p->residency & PSU))) {
		tell_player(p, " That alias not found in the library.\n");
		return;
	} 

	else {
		oldstack = stack;
		if (*new_alias_name)
			sprintf(stack, "%s %s", new_alias_name, to_be_copied.alias_string);
		else
			sprintf(stack, "%s %s", to_be_copied.command, to_be_copied.alias_string);
		stack = end_string(stack);

		define_alias(p, oldstack);
		
		stack = oldstack;
	}
}

void library_examine(player * p, char *str) {

	alias_library see_this;
	char *oldstack;

	if (!*str) {
		tell_player(p, " Format: Libexam <library alias to examine>\n");
		return;
		}

	see_this = get_lib_alias(str);

	if (!see_this.command || (see_this.privs && !(p->residency & PSU))) {

		tell_player(p, " Couldn't find that alias in the library.\n");
		return;
		}

	oldstack = stack;

	sprintf(stack, "Command: %s\nDoes   : %s\nUsage  : %s\nAuthor : %s\n",
	 see_this.command, see_this.alias_string, see_this.description, see_this.author);
	stack = end_string(stack);

	tell_player(p, oldstack);
	stack = oldstack;
}

/* well, we learned that aliases ain't perfect neither */

void blank_all_aliases(player * p, char *str) {

	if (!p->saved) {
		tell_player(p, "You can't -- you have no alias list anyway");
		return;
	}

	p->saved->alias_top = 0;
	tell_player(p, "Aliases deleted.\n");
}

