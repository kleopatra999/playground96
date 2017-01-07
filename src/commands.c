/*
 * commands.c
 */
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#include "config.h"
#include "player.h"
#include "fix.h"

/* externs */

extern int      check_password(char *, char *, player *);
extern char    *crypt(char *, char *);
extern void     check_list_resident(player *);
extern void     check_list_newbie(char *);
extern void     destroy_player(), save_player(), password_mode_on(),
                password_mode_off(), sub_command(), extract_pipe_global(), tell_room(),
                extract_pipe_local(), pstack_mid(), clear_gag_logoff(), 
		purge_gaglist();
extern player  *find_player_global(), *find_player_absolute_quiet(),
               *find_player_global_quiet();
extern char    *end_string(), *tag_string(), *next_space(), *do_pipe(), *full_name(),
               *caps(), *sys_time();
extern int      global_tag(), emote_no_break();
extern file     idle_string_list[];
extern saved_player *find_saved_player();
extern char    *convert_time(time_t);
extern char    *gstring_possessive(player *);
extern char    *gstring(player *);
extern void     su_wall(char *);
extern char    *number2string(int);
extern char    *get_gender_string(player *);
extern char    *havehas(player *);
extern char    *isare(player *);
extern char    *waswere(player *);
extern char    *word_time(int);
extern char    *time_diff(int), *time_diff_sec(time_t, int);
extern char     sess_name[];
extern int      session_reset;
extern list_ent *fle_from_save();
#ifdef TRACK
extern int addfunction(char *);
#endif
/* fuckin intern */
void	newsetpw1(player*, char *);

/* birthday and age stuff */

void            set_age(player * p, char *str)
{
   char           *oldstack;
   int             new_age;
   oldstack = stack;

#ifdef TRACK
   sprintf(functionin,"set_age(%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (!*str)
   {
      tell_player(p, " Format: age <number>\n");
      return;
   }
   new_age = atoi(str);
   if (new_age < 0)
   {
      tell_player(p, " You can't be of a negative age !\n");
      return;
   }
   p->age = new_age;
   if (p->age)
   {
      sprintf(oldstack, " Your age is now set to %d years old.\n", p->age);
      stack = end_string(oldstack);
      tell_player(p, oldstack);
   } else
      tell_player(p, " You have turned off your age so no-one can see it.\n");
   stack = oldstack;
}


/* set birthday */

void            old_set_birthday(player * p, char *str)
{
   char           *oldstack;
   struct tm       bday;
   struct tm      *tm_time;
   time_t          the_time;
   int             t;

#ifdef TRACK
   sprintf(functionin,"set_birthday(%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   the_time = time(0);
   tm_time = localtime(&the_time);
   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: birthday <day>/<month>(/<year>)\n");
      return;
   }
   memset((char *) &bday, 0, sizeof(struct tm));
   bday.tm_year = tm_time->tm_year;
   bday.tm_mday = atoi(str);

   if (!bday.tm_mday)
   {
      tell_player(p, " Birthday cleared.\n");
      p->birthday = 0;
      return;
   }
   if (bday.tm_mday < 0 || bday.tm_mday > 31)
   {
      tell_player(p, " Not a valid day of the month.\n");
      return;
   }
   while (isdigit(*str))
      str++;
   str++;
   bday.tm_mon = atoi(str);
   if (bday.tm_mon <= 0 || bday.tm_mon > 12)
   {
      tell_player(p, " Not a valid month.\n");
      return;
   }
   bday.tm_mon--;

   while (isdigit(*str))
      str++;
   str++;
   while (strlen(str) > 2)
      str++;
   bday.tm_year = atoi(str);
   if (bday.tm_year == 0)
   {
      bday.tm_year = tm_time->tm_year;
      p->birthday = TIMELOCAL(&bday);
   } else
   {
      p->birthday = TIMELOCAL(&bday);
      t = time(0) - (p->birthday);
      if (t > 0)
    p->age = t / 31536000;
   }

   sprintf(oldstack, " Your birthday is set to the %s.\n",
      birthday_string(p->birthday));
   stack = end_string(oldstack);
   tell_player(p, oldstack);
   stack = oldstack;
}



/* recap someones name */

void recap(player * p, char *str)
{
   char *oldstack;
   char *space, *n;
   int found_lower;
   player *p2 = p;

#ifdef TRACK
   sprintf(functionin,"recap (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: recap <name>\n");
      return;
   }
   strcpy(stack, str);
   if (strcasecmp(stack, p->lower_name))
   {
      if (!(p->residency & SU))
      {
         tell_player(p, " But that isn't your name !!\n");
         return;
      }
      strcpy(stack, str);
      lower_case(stack);
      p2 = find_player_absolute_quiet(stack);
      if (!p2)
      {
         tell_player(p, " No-one of that name on the program.\n");
         return;
      }
      if ((p2->residency >= p->residency) && !(p == p2))
      {
         tell_player(p, " Nope, sorry, you can't!\n");
	   sprintf(stack, " -=*> %s TRIED to recap %s!\n     (fucking git)\n", 
		   p->name, p2->name);
	   stack = end_string(stack);
	   su_wall_but (p, stack);
	   stack = oldstack;
         return;
      }
   }
   found_lower = 0;
   n = str;
   while (*n)
   {
      if (*n >= 'a' && *n <= 'z')
      {
         found_lower = 1;
      }
      *n++;
   }
   if (!found_lower)
   {
      n = str;
      *n++;
      while (*n)
      {
         *n = *n - ('A' - 'a');
         *n++;
      }
   }
   strcpy(p2->name, str);
   sprintf(stack, " Name changed to '%s'\n", p2->name);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
   if (!(p == p2))
   { 
   sprintf(stack, " -=*> %s recapped %s!\n", p->name, p2->name);
   stack = end_string(stack);
   su_wall_but (p, stack);
   stack = oldstack;
   }
}


/* see the time */

void            view_time(player * p, char *str)
{
   char           *oldstack;

   oldstack = stack;
   if (p->jetlag)
      sprintf(stack, " Your local time is %s.\n"
         " %s time is %s.\n"
         " The program has been up for %s.\n That is from %s.\n"
         " Total number of logins in that time is %s.\n"
	 " Most people on so far since the last reboot was: %d.\n",
         time_diff(p->jetlag), TALKER_NAME, sys_time(), word_time(time(0) - up_date),
         convert_time(up_date), number2string(logins), max_ppl_on_so_far);
   else
      sprintf(stack, " %s time is %s.\n"
         " The program has been up for %s.\n That is from %s.\n"
         " Total number of logins in that time is %s.\n"
	 " Most people on so far since the last reboot was: %d.\n",
         TALKER_NAME, sys_time(), word_time(time(0) - up_date),
         convert_time(up_date), number2string(logins),max_ppl_on_so_far);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

/* go quiet */

void            go_quiet(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"qo_quiet (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   earmuffs(p, str);
   blocktells(p, str);
}

/* the idle command */

void check_idle(player * p, char *str)
{
   player *scan, **list, **step;
   int i,n;
   char *oldstack, middle[80], namestring[40], *id;
   file *is_scan;
   int page, pages, count, not_idle = 0;

#ifdef TRACK
   sprintf(functionin,"check_idle (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   command_type |= SEE_ERROR;
   if (isalpha(*str) && !strstr(str, "everyone"))
   {
      align(stack);
      list = (player **) stack;
      n = global_tag(p, str);
      if (!n)
      {
         stack = oldstack;
         return;
      }
      id = stack;
      for (step = list, i = 0; i < n; i++, step++)
      {
         if (p->custom_flags & NOPREFIX)
         {
            strcpy(namestring, (*step)->name);
         } else if (*((*step)->pretitle))
         {
            sprintf(namestring, "%s %s", (*step)->pretitle, (*step)->name);
         } else
         {
            strcpy(namestring, (*step)->name);
         }
         if (!(*step)->idle)
         {
            sprintf(stack, "%s has just hit return.\n", namestring);
         } else
         {
            if ((*step)->idle_msg[0])
            {
               sprintf(stack, "%s %s\n%s %s %s idle\n", namestring,
                       (*step)->idle_msg, caps(gstring((*step))),
		       isare(*step),
                       word_time((*step)->idle));
            } else
            {
               sprintf(stack, "%s %s %s idle.\n", namestring,isare(*step),
                       word_time((*step)->idle));
            }
         }
         stack = end_string(stack);
         tell_player(p, id);
         stack = id;
      }
      cleanup_tag(list, n);
      stack = oldstack;
      return;
   }
   if (strstr(str, "everyone"))
   {
      id = str;
      str = strchr(str, ' ');
      if (!str)
      {
         str = id;
         *str++ = '1';
         *str = 0;
      }
   }

   page = atoi(str);
   if (page <= 0)
      page = 1;
   page--;

   pages = (current_players - 1) / (TERM_LINES - 2);
   if (page > pages)
      page = pages;

   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      if (scan->name[0] && scan->location && (scan->idle) < 300)
      {
         not_idle++;
      }
   }

   if (current_players == 1)
   {
      strcpy(middle, "There is only you on the program at the moment");
   } else if (not_idle == 1)
   {
      sprintf(middle, "There are %d people here, only one of whom "
                      "appears to be awake", current_players);
   } else
   {
      sprintf(middle, "There are %d people here, %d of which appear "
                      "to be awake", current_players, not_idle);
   }
   pstack_mid(middle);

   count = page * (TERM_LINES - 2);
   for (scan = flatlist_start; count; scan = scan->flat_next)
   {
      if (!scan)
      {
         tell_player(p, " Bad idle listing, abort.\n");
         log("error", "Bad idle list");
         stack = oldstack;
         return;
      } else if (scan->name[0])
      {
         count--;
      }
   }

   for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
   {
      if (scan->name[0] && scan->location)
      {
         if (p->custom_flags & NOPREFIX)
         {
            strcpy(namestring, scan->name);
         } else if ((*scan->pretitle))
         {
            sprintf(namestring, "%s %s", scan->pretitle, scan->name);
         } else
         {
            sprintf(namestring, "%s", scan->name);
         }
      } else
         continue;
      if (scan->idle_msg[0])
      {
		if (emote_no_break(*scan->idle_msg))
         sprintf(stack, "%3d:%.2d - %s%s\n", scan->idle/60, scan->idle%60, namestring, scan->idle_msg);
		else
         sprintf(stack, "%3d:%.2d - %s %s\n", scan->idle/60, scan->idle%60, namestring, scan->idle_msg);
      } else
      {
         for (is_scan = idle_string_list; is_scan->where; is_scan++)
         {
            if (is_scan->length >= scan->idle)
               break;
         }
         if (!is_scan->where)
            is_scan--;
         sprintf(stack, "%3d:%.2d - %s %s", scan->idle/60, scan->idle%60, namestring, is_scan->where);
      }
      stack = strchr(stack, 0);
      count++;
   }
   sprintf(middle, "Page %d of %d", page + 1, pages + 1);
   pstack_mid(middle);

   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}


/* set things */


void            set_idle_msg(player * p, char *str)
{

#ifdef TRACK
   sprintf(functionin,"set_idle_msg (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (*str)
   {
      strncpy(p->idle_msg, str, MAX_TITLE - 3);
      if (p->custom_flags & NOPREFIX)
      {
		if (emote_no_break(*p->idle_msg))
         TELLPLAYER(p, " Idle message set to ....\n%s%s\nuntil you type a"
            " new command.\n", p->name, p->idle_msg);
		else
         TELLPLAYER(p, " Idle message set to ....\n%s %s\nuntil you type a"
            " new command.\n", p->name, p->idle_msg);
      } else
      {
		if (emote_no_break(*p->idle_msg))
         TELLPLAYER(p, " Idle message set to ....\n%s%s\nuntil you type a"
            " new command.\n", full_name(p), p->idle_msg);
		else
         TELLPLAYER(p, " Idle message set to ....\n%s %s\nuntil you type a"
            " new command.\n", full_name(p), p->idle_msg);
      }
   } else
      strcpy(stack, " Please set an idlemsg of a greater than zero length.\n");
}


void            set_enter_msg(player * p, char *str)
{

#ifdef TRACK
   sprintf(functionin,"set_enter_msg (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'entermsg'\n"); 
	return;
   }
   if (!*str)
   {
      tell_player(p," Format: entermsg <entermsg>\n");
      return;
   }
   strncpy(p->enter_msg, str, MAX_ENTER_MSG - 3);

	if (emote_no_break(*str))
   TELLPLAYER(p, " This is what people will see when you enter the "
      "room.\n%s%s\n", p->name, p->enter_msg);
	else
   TELLPLAYER(p, " This is what people will see when you enter the "
      "room.\n%s %s\n", p->name, p->enter_msg);
}

void            set_title(player * p, char *str)
{

#ifdef TRACK
   sprintf(functionin,"set_title (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'title'\n"); 
	return;
   }
   strncpy(p->title, str, MAX_TITLE - 3);
	if (emote_no_break(*str))
   TELLPLAYER(p, " You change your title so that now you are known as "
      "...\n%s%s\n", p->name, p->title);
	else
   TELLPLAYER(p, " You change your title so that now you are known as "
      "...\n%s %s\n", p->name, p->title);
}

void            set_pretitle(player * p, char *str)
{
   char           *oldstack, *scan;

#ifdef TRACK
   sprintf(functionin,"set_pretitle (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'prefix'\n"); 
	return;
   }
   oldstack = stack;
   if (strstr(str, "+[") != NULL)
   {
      tell_player(p, " You may not have \"+[\" in your prefix.\n");
      return;
   }

   for (scan = str; *scan; scan++)
   {
      switch (*scan)
      {
         case '^':
            tell_player(p, " You may not have colors in your prefix.\n");
            return;
            break;
         case ' ':
            tell_player(p, " You may not have spaces in your prefix.\n");
            return;
            break;
         case ',':
            tell_player(p, " You may not have commas in your prefix.\n");
            return;
            break;
      }
   }

   if (find_saved_player(str) || find_player_absolute_quiet(str))
   {
      tell_player(p, " That is the name of a player, so you can't use that "
                     "for a prefix.\n");
      return;
   }

   strncpy(p->pretitle, str, MAX_PRETITLE - 3);
   sprintf(stack, " You change your pretitle so you become: %s %s\n",
           p->pretitle, p->name);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

void            set_description(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_description (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'desc'\n"); 
	return;
   }
   oldstack = stack;
   strncpy(p->description, str, MAX_DESC - 3);
   sprintf(stack, " You change your description to read...\n%s\n",
           p->description);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

void            set_plan(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_plan (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'plan'\n"); 
	return;
   }
   oldstack = stack;
   strncpy(p->plan, str, MAX_PLAN - 3);
   sprintf(stack, " You set your plan to read ...\n%s\n", p->plan);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

void            set_prompt(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_prompt (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   strncpy(p->prompt, str, MAX_PROMPT - 3);
   sprintf(stack, " You change your prompt to %s\n", p->prompt);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

void            set_converse_prompt(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_converse_prompt (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   strncpy(p->converse_prompt, str, MAX_PROMPT - 3);
   sprintf(stack, " You change your converse prompt to %s\n",
           p->converse_prompt);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

void            set_term_width(player * p, char *str)
{
   char           *oldstack;
   int             n;

#ifdef TRACK
   sprintf(functionin,"set_term_width (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (!strcasecmp("off", str))
   {
      tell_player(p, " Linewrap turned off.\n");
      p->term_width = 0;
      return;
   }
   n = atoi(str);
   if (!n)
   {
      tell_player(p, " Format: linewrap off/<terminal_width>\n");
      return;
   }
   if (n <= ((p->word_wrap) << 1))
   {
      tell_player(p, " Can't set terminal width that small compared to "
                     "word wrap.\n");
      return;
   }
   if (n < 10)
   {
      tell_player(p, " Nah, you haven't got a terminal so small !!\n");
      return;
   }
   p->term_width = n;
   sprintf(stack, " Linewrap set on, with terminal width %d.\n", p->term_width);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}


void            set_word_wrap(player * p, char *str)
{
   char           *oldstack;
   int             n;

#ifdef TRACK
   sprintf(functionin,"set_word_wrap (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (!strcasecmp("off", str))
   {
      tell_player(p, " Wordwrap turned off.\n");
      p->word_wrap = 0;
      return;
   }
   n = atoi(str);
   if (!n)
   {
      tell_player(p, " Format: wordwrap off/<max_word_size>\n");
      return;
   }
   if (n >= ((p->term_width) >> 1))
   {
      tell_player(p, " Can't set max word length that big compared to term "
                     "width.\n");
      return;
   }
   p->word_wrap = n;
   sprintf(stack, " Wordwrap set on, with max word size set to %d.\n",
           p->word_wrap);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

/* quit from the program */

void            quit(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"quit (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (!str)
      p->flags |= PANIC;
   p->flags |= CHUCKOUT;

   /* Clean up lists */

   if (p->residency == NON_RESIDENT)
      check_list_newbie(p->lower_name);
   else
      check_list_resident(p);

   clear_gag_logoff(p);

   purge_gaglist(p, 0);

   if ((p->logged_in == 1) && (sess_name[0] != '\0'))
   {
      if (!strcasecmp(p->name, sess_name))
      {
         session_reset = 0;
      }
   }
}

/* command to change gender */

void            gender(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"gender (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   *str = tolower(*str);
   switch (*str)
   {
      case 'm':
         p->gender = MALE;
         tell_player(p, " Gender set to Male.\n");
         break;
      case 'f':
         p->gender = FEMALE;
         tell_player(p, " Gender set to Female.\n");
         break;
      case 'p':
         p->gender = PLURAL;
         tell_player(p, " Gender set to Plural.\n");
         break;
      case 'n':
         p->gender = OTHER;
         tell_player(p, " Gender set to well, erm, something.\n");
         break;
      default:
         tell_player(p, " No gender set, Format : gender m/f/p/n\n");
         break;
   }
}



/* save command */

void            do_save(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"do_save (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->residency == NON_RESIDENT)
   {
      log("error", "Tried to save a non-resi, (chris)");
      return;
   }
   save_player(p);
}

/* show email */

void            check_email(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"check_email (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (p->residency == NON_RESIDENT)
   {
      tell_player(p, " You are non resident and so cannot set an email "
                     "address.\n"
                     " Please ask a super user to make you resident.\n");
      return;
   }
   if (p->email[0] == -1)
      tell_player(p, " You have declared that you have no email address.\n");
   else
   {
      sprintf(stack, " Your email address is set to :\n%s\n", p->email);
      stack = end_string(oldstack);
      tell_player(p, oldstack);
   }
   if (p->custom_flags & FRIEND_EMAIL)
      tell_player(p, " Only your friends can see your email address.\n");
   else if (p->custom_flags & PRIVATE_EMAIL)
      tell_player(p, " Your email is private.\n");
   else
      tell_player(p, " Your email is public for all to read.\n");
   stack = oldstack;
}

/* change whether an email is public or private */

/* void            public_com(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"public_com (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (!strcasecmp("on", str))
      p->custom_flags &= ~PRIVATE_EMAIL;
   else if (!strcasecmp("off", str))
      p->custom_flags |= PRIVATE_EMAIL;
   else
      p->custom_flags ^= PRIVATE_EMAIL;

   if (p->custom_flags & PRIVATE_EMAIL)
      tell_player(p, " Your email is private, only the admin will be able "
                     "to see it.\n");
   else
      tell_player(p, "Your email address is public, so everyone can see "
                     "it.\n");
} */
void public_com(player *p, char *str) {
	tell_player(p, "This command is no longer used -- Use TOGGLE instead.\n");
}	

/* email command */

void            change_email(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"change_email (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (p->residency == NON_RESIDENT)
   {
      tell_player(p, " You may only use the email command once resident.\n"
                     " Please ask a superuser to become to grant you "
                     "residency.\n");
      return;
   }
   if (!*str)
   {
      check_email(p, str);
      return;
   }
   if (!strcasecmp(str, "private"))
   {
      p->custom_flags |= PRIVATE_EMAIL;
      tell_player(p, " Your email is private, only the Admin will be able "
                     "to see it.\n");
      return;
   }
   if (!strcasecmp(str, "public"))
   {
      p->custom_flags &= ~PRIVATE_EMAIL;
      tell_player(p, " Your email address is public, so everyone can see "
                     "it.\n");
      return;
   }
   if (!strstr(str, "@") || !strstr(str, ".") || strstr(str, "\'") 
	|| strstr(str, "\"") || strstr(str, " ") || strstr(str, "^")
	|| strstr(str, "email ")) {
	tell_player(p, " I believe you made a typo.. please try again.\n");
        tell_player(p, " Email address not changed.\n");
        return;
	}
   if (!strlen(p->email))
   {
      sprintf(stack, "%-18s %s (New)", p->name, str);
      stack = end_string(stack);
      log("help", oldstack);
      stack = oldstack;
   }
   strncpy(p->email, str, MAX_EMAIL - 3);
   if (p->saved) strncpy(p->saved->email, str, MAX_EMAIL - 3);
   sprintf(oldstack, " Email address has been changed to: %s\n", p->email);
   stack = end_string(oldstack);
   tell_player(p, oldstack);
   p->residency &= ~NO_SYNC;
   save_player(p);
   stack = oldstack;
   return;
}

/* password changing routines */

char           *do_crypt(char *entered, player * p)
{
   char            key[9];

#ifdef TRACK
   sprintf(functionin,"do_crypt (SOMETHING,%s)",p->name);
   addfunction(functionin);
#endif

   strncpy(key, entered, 8);
   return crypt(key, p->lower_name);
}


void            got_password2(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"got_password (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   password_mode_off(p);
   p->input_to_fn = 0;
   p->flags |= PROMPT;
   if (strcmp(p->password_cpy, str))
   {
      tell_player(p, "\n But that doesn't match !!!\n"
                     " Password not changed ...\n");
   } else
   {
      strcpy(p->password, do_crypt(str, p));
      tell_player(p, "\n Password has now been changed.\n");
      if (p->email[0] != 2) 
         p->residency &= ~NO_SYNC;
      save_player(p);
   }
}

void            got_password1(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"got_password1 (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (strlen(str) > (MAX_PASSWORD - 3))
   {
      do_prompt(p, "\n Password too long, please try again.\n"
                   " Please enter a shorter password:");
      p->input_to_fn = got_password1;
   } else
   {
      strcpy(p->password_cpy, str);
      do_prompt(p, "\n Enter password again to verify:");
      p->input_to_fn = got_password2;
   }
}

void            validate_password(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"validate_password (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (!check_password(p->password, str, p))
   {
      tell_player(p, "\n Hey ! thats the wrong password !!\n");
      password_mode_off(p);
      p->input_to_fn = 0;
      p->flags |= PROMPT;
   } else
   {
      do_prompt(p, "\n Now enter a new password:");
      p->input_to_fn = got_password1;
   }
}

void            change_password(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"change_password (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->residency == NON_RESIDENT)
   {
      tell_player(p, " You may only set a password once resident.\n"
                     " To become a resident, please ask a superuser.\n");
      return;
   }
   password_mode_on(p);
   p->flags &= ~PROMPT;
   if (p->password[0] && p->password[0] != -1)
   {
      do_prompt(p, " Please enter your current password:");
      p->input_to_fn = validate_password;
   } else
   {
      do_prompt(p, " You have no password.\n"
                   " Please enter a password:");
      p->input_to_fn = got_password1;
   }
}



/* show wrap info */

void            check_wrap(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"check_wrap (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (p->term_width)
   {
      sprintf(stack, " Line wrap on, with terminal width set to %d "
                     "characters.\n",
              p->term_width);
      stack = strchr(stack, 0);
      if (p->word_wrap)
         sprintf(stack, " Word wrap is on, with biggest word size "
                        "set to %d characters.\n",
                 p->word_wrap);
      else
         strcpy(stack, " Word wrap is off.\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
   } else
      tell_player(p, " Line wrap and word wrap turned off.\n");
   stack = oldstack;
}

/* Toggle the ignoreprefix flag () */

void            ignoreprefix(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"ignoreprefix (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (!strcasecmp("off", str))
      p->custom_flags &= ~NOPREFIX;
   else if (!strcasecmp("on", str))
      p->custom_flags |= NOPREFIX;
   else
      p->custom_flags ^= NOPREFIX;

   if (p->custom_flags & NOPREFIX)
      tell_player(p, " You are now ignoring prefixes.\n");
   else
      tell_player(p, " You are now seeing prefixes.\n");
}


/* Toggle the ignore emote flag () */
void            ignoreemoteprefix(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"ignoreemoteprefix (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (!strcasecmp("off", str))
      p->custom_flags &= ~NOEPREFIX;
   else if (!strcasecmp("on", str))
      p->custom_flags |= NOEPREFIX;
   else
      p->custom_flags ^= NOEPREFIX;

   if (p->custom_flags & NOEPREFIX)
      tell_player(p, " You are now ignoring prefixes specifically on emotes\n");
   else
      tell_player(p, " You are now seeing prefixes specifically on emotes again.\n");
}


void            set_time_delay(player * p, char *str)
{
   int             time_diff;
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_time_delay (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (!*str)
   {
      if (p->jetlag)
         sprintf(stack, " Your time difference is currently set at %d hours.\n",
                 p->jetlag);
      else
         sprintf(stack, " Your time difference is not currently set.\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   time_diff = atoi(str);
   if (!time_diff)
   {
      tell_player(p, " Time difference of 0 hours set. "
                     "(that was worth it, wasn't it... )\n");
      p->jetlag = 0;
      return;
   }
   if (time_diff < -23 || time_diff > 23)
   {
      tell_player(p, " That's a bit silly, isn't it?\n");
      return;
   }
   p->jetlag = time_diff;

   oldstack = stack;
   if (p->jetlag == 1)
      strcpy(stack, " Time Difference set to 1 hour.\n");
   else
      sprintf(stack, " Time Difference set to %d hours.\n", p->jetlag);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

void            set_ignore_msg(player * p, char *str)
{
   char           *oldstack;
   oldstack = stack;

#ifdef TRACK
   sprintf(functionin,"set_ignore_msg (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (!*str)
   {
      tell_player(p, " You reset your ignore message.\n");
      strcpy(p->ignore_msg, "");
      return;
   }
   strncpy(p->ignore_msg, str, MAX_IGNOREMSG - 3);
   strcpy(stack, " Ignore message now set to ...\n");
   stack = strchr(stack, 0);
   sprintf(stack, " %s\n", p->ignore_msg);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}



void            set_logonmsg(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"set_logonmsg (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'logonmsg'\n"); 
	return;
   }
   strncpy(p->logonmsg, str, MAX_ENTER_MSG - 3);
	if (emote_no_break(*str))
TELLPLAYER(p, " You set your logonmsg to ...\n%s%s\n", p->name, p->logonmsg);
	else
TELLPLAYER(p, " You set your logonmsg to ...\n%s %s\n", p->name, p->logonmsg);
}


void            set_logoffmsg(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"set_logoffmsg (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'logoffmsg'\n"); 
	return;
   }
   strncpy(p->logoffmsg, str, MAX_ENTER_MSG - 3);
	if (emote_no_break(*str))
   TELLPLAYER(p, " You set your logoffmsg to ...\n%s%s\n", p->name, p->logoffmsg);
	else
   TELLPLAYER(p, " You set your logoffmsg to ...\n%s %s\n", p->name, p->logoffmsg);
}


void            set_blockmsg(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_blockmsg (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   strncpy(p->blockmsg, str, MAX_IGNOREMSG - 3);
	if (emote_no_break(*str))
   sprintf(stack, " You set your blockmsg to ...\n{%s%s}\n", p->name, p->blockmsg);
	else
   sprintf(stack, " You set your blockmsg to ...\n{%s %s}\n", p->name, p->blockmsg);
}


void            set_exitmsg(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"set_exitmsg (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'exitmsg'\n"); 
	return;
   }
   strncpy(p->exitmsg, str, MAX_ENTER_MSG - 3);
	if (emote_no_break(*str))
 TELLPLAYER(p, " You set your exitmsg to ...\n%s%s\n", p->name, p->exitmsg);
	else
 TELLPLAYER(p, " You set your exitmsg to ...\n%s %s\n", p->name, p->exitmsg);
}


void            set_irl_name(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_irl_name (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'irl_name'\n"); 
	return;
   }
   oldstack = stack;
   strncpy(p->irl_name, str, MAX_NAME - 3);
   sprintf(stack, " You set your irl name to be: %s\n", p->irl_name);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}


/* decided to make this URL instead.. bear with and watch the 
   old 'alt_email' references until I fix it :P   -traP */
void            set_alt_email(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_alt_email (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'url'\n"); 
	return;
   }
   oldstack = stack;
   strncpy(p->alt_email, str, MAX_EMAIL - 3);
   sprintf(stack, " You set your URL to ...\n%s\n", p->alt_email);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}



void            set_hometown(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_hometown (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'hometown'\n"); 
	return;
   }
   oldstack = stack;
   strncpy(p->hometown, str, MAX_SPODCLASS - 3);
   sprintf(stack, " You set your hometown to ...\n%s\n", p->hometown);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}



void            set_spod_class(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_spod_class (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'spod_class'\n"); 
	return;
   }
   oldstack = stack;
   strncpy(p->spod_class, str, MAX_SPODCLASS - 3);
   sprintf(stack, " You set your spod_class to ...\n < %s >\n", p->spod_class);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}



void            set_favorites(player * p, char *str)
{
   char           *oldstack, firster[80] = "", *nexter, *arg_two;
   int		  choice;

#ifdef TRACK
   sprintf(functionin,"set_favorites (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'favorite'\n"); 
	return;
   }
   if (!*str)
   {
	tell_player(p, " Type 'help favorite' for how to use this.\n");
	return;
   }

   arg_two = next_space(str);
  if (*arg_two)
   *arg_two++ = 0;

   if (!strcasecmp(str,"list"))
   {
   oldstack = stack;

	sprintf(stack," Your favorites are set to...\n");
	stack = strchr(stack, 0);
 strcpy(firster, "");
	if (*p->favorite1) {
		strcpy(firster,p->favorite1);
		nexter = next_space(firster);
		*nexter++ = 0;
		sprintf(stack, " 1. %-12.12s : %s\n", firster, nexter);
		}
	else strcpy(stack, "1. Not set.\n");
	stack = strchr(stack, 0);
 strcpy(firster, "");

	if (*p->favorite2) {
		strcpy(firster,p->favorite2);
		nexter = next_space(firster);
		*nexter++ = 0;
		sprintf(stack, " 2. %-12.12s : %s\n", firster, nexter);
		}
	else strcpy(stack, " 2. Not set.\n");
	stack = strchr(stack, 0);
 strcpy(firster, "");
	if (*p->favorite3) {
		strcpy(firster,p->favorite3);
		nexter = next_space(firster);
		*nexter++ = 0;
		sprintf(stack, " 3. %-12.12s : %s\n", firster, nexter);
		}
	else strcpy(stack, " 3. Not set.\n");
	stack = strchr(stack, 0);

	stack = end_string(stack);
	tell_player(p, oldstack);
	stack = oldstack;
	return;
	}

   if (*arg_two && !strcasecmp(arg_two,"blank"))
   {
	/* arg 1 should be a number */

	choice = atoi(str);
	if ((!*arg_two) || choice < 1 || choice > 3)
		{
		tell_player(p, " Format: favorite <#> blank  (# = 1 2 or 3)\n");
		return;
		}
	switch (choice)
	{
		case 1:
			p->favorite1[0] = 0;
			break;
		case 2:
			p->favorite2[0] = 0;
			break;
		case 3:
			p->favorite3[0] = 0;
			break;
		default:
			tell_player(p, " Erk,,, got a bug here...\n");
			log("error","Error in set_favorite");
			return;
	}
	tell_player(p, " OK, blanked.\n");
	return;
   }

   /* ok... last variation... */
   /* arg 1 is a number, arg 2 is what to set it too... */
   choice = atoi(str);
   if (choice < 1 || choice > 3)
       {
	   tell_player(p, " Format: favorite <#> <set to> (# = 1 2 or 3)\n");
	   return;
       }
   oldstack = stack;
   switch (choice)
   {
	case 1:
		strncpy(p->favorite1, arg_two, MAX_SPODCLASS - 3);
		strcpy(firster,p->favorite1);
		nexter = next_space(firster);
		*nexter++ = 0;
		sprintf(stack, " You set your favorite %.12s to: %s\n", firster, nexter);
		break;
	case 2:
		strncpy(p->favorite2, arg_two, MAX_SPODCLASS - 3);
		strcpy(firster,p->favorite2);
		nexter = next_space(firster);
		*nexter++ = 0;
		sprintf(stack, " You set your favorite %.12s to: %s\n", firster, nexter);
		break;
	case 3:
		strncpy(p->favorite3, arg_two, MAX_SPODCLASS - 3);
		strcpy(firster,p->favorite3);
		nexter = next_space(firster);
		*nexter++ = 0;
		sprintf(stack, " You set your favorite %.12s to: %s\n", firster, nexter);
		break;
	default:
		log("error", "Error in setting a favorite...");
		return;
   }

   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}


/* Screen locking routines... spoon of password, basically... */

void            unlock_screen(player * p, char *str) {

	if(strcmp(p->slock_pw, str)) 
	{
		do_prompt(p, " Please enter CORRECT password to unlock screen:");
		p->input_to_fn = unlock_screen;
	}
	else
	{
		password_mode_off(p);
		p->input_to_fn = 0;
   		p->flags |= PROMPT;
      		strcpy(p->slock_pw, "");
		tell_player(p, " Screen is now unlocked.\n");
	}

}
 

void            confirm_slock2(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"got_password (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (strcmp(p->slock_pw, str))
   {
      password_mode_off(p);
      p->flags |= PROMPT;
      p->input_to_fn = 0;
      strcpy(p->slock_pw, "");
      tell_player(p, "\n But that doesn't match !!!\n"
                     " screen not locked ...\n");
   } else
   {
      do_prompt(p, "\n Screen is now locked - No one may type any commands"
		   " without unlocking it with the current password.\n"
		   " Please enter password to unlock screen:");
      p->input_to_fn = unlock_screen;
   }
}

void            confirm_slock1(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"got_password1 (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (strlen(str) > (MAX_PASSWORD - 3))
   {
      do_prompt(p, "\n Password too long, please try again.\n"
                   " Please enter a shorter password:");
      p->input_to_fn = confirm_slock1;
   } else
   {
      strcpy(p->slock_pw, str);
      do_prompt(p, "\n Enter password again to verify:");
      p->input_to_fn = confirm_slock2;
   }
}

void            set_screenlock(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"change_password (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   password_mode_on(p);
   p->flags &= ~PROMPT;
      do_prompt(p, " Entering screen lock mode:\n"
                   " Please enter a password:");
      p->input_to_fn = confirm_slock1;
}

void toggle_email_site_flags(player *p, char *str) {

	char *opt;
	int em = 0, si = 0;
	int pri = 0, pub = 0, fri = 0;

	opt = next_space(str);
        *opt++ = 0;

	if (!*str || !*opt) {
		tell_player(p, " Format: toggle <email/site/both> <public/private/friend>\n");
		return;
		}

	if (!strcasecmp(str, "email"))
		em = 1;
 	else if (!strcasecmp(str, "site"))
		si = 1;
	else if (!strcasecmp(str, "both")) {
		em = 1;
		si = 1; }
	else {
		tell_player(p, " Format: toggle <email/site/both> <public/private/friend>\n");
		return;
		}

	if (!strcasecmp(opt, "public"))
		pub = 1;
	else if (!strcasecmp(opt, "private"))
		pri = 1;
	else if (!strcasecmp(opt, "friend"))
		fri = 1;
	else {
		tell_player(p, " Format: toggle <email/site/both> <public/private/friend>\n");
		return;
		}

	if (em) {

		if (fri) {
			tell_player(p, " Setting email to friend viewable only.\n");
			p->custom_flags |= PRIVATE_EMAIL;
			p->custom_flags |= FRIEND_EMAIL;
		} else if (pub) {
			tell_player(p, " Setting email as public for all to see.\n");
			p->custom_flags &= ~PRIVATE_EMAIL;
			p->custom_flags &= ~FRIEND_EMAIL;
		} else {
			tell_player(p, " Setting email as private. Only you and the admin will be able to see it.\n");
			p->custom_flags |= PRIVATE_EMAIL;
			p->custom_flags &= ~FRIEND_EMAIL;
		}
	}
	if (si)	{
		if (fri) {
			tell_player(p, " Setting site to friend viewable only.\n");
			p->custom_flags &= ~PUBLIC_SITE;
			p->custom_flags |= FRIEND_SITE;
		} else if (pub) {
			tell_player(p, " Setting site as public for all to see.\n");
			p->custom_flags |= PUBLIC_SITE;
			p->custom_flags &= ~FRIEND_SITE;
		} else {
			tell_player(p, " Setting site as private. Only you and the admin will be able to see it.\n");
			p->custom_flags &= ~PUBLIC_SITE;
			p->custom_flags &= ~FRIEND_SITE;
		}
	}
}

void            newsetpw0(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"change_password (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   p->flags &= ~PROMPT;
   password_mode_on(p);
   {
      do_prompt(p, " I am now going to ask you to set a password on your character.\n"
		   " Pick something not easy to guess, but easy for you to remember.\n"
		   " \n Please enter a new password:");
      p->input_to_fn = newsetpw1;
   }
}

void	begin_ressie_doemail(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"change_email (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (!*str) {
	do_prompt(p, " You cannot have a blank email address.\n");
	do_prompt(p, " Please enter your email address now:");
	p->input_to_fn = begin_ressie_doemail;
   } else if (!strstr(str, "@") || !strstr(str, ".") || strstr(str, " ")
	|| strstr(str, "\'") || strstr(str, "\"") || strstr(str, "^")
	|| strstr(str, "email ")) {
	do_prompt(p, " I believe you made a typo.. please try again\n");
	do_prompt(p, " Please enter your email address now:");
	p->input_to_fn = begin_ressie_doemail;
   } else {
   if (!strlen(p->email))
   {
      sprintf(stack, "%-18s %s (New)", p->name, str);
      stack = end_string(stack);
      log("help", oldstack);
      stack = oldstack;
   }
   strncpy(p->email, str, MAX_EMAIL - 3);
   sprintf(oldstack, " Email address has been set to: %s\n", p->email);
   stack = end_string(oldstack);
   tell_player(p, oldstack);
   stack = oldstack;
   p->input_to_fn = 0;
   p->flags |= PROMPT;
   newsetpw0(p, 0);
   return;   
  }
}

void begin_ressie(player * p, char *str) {

   p->flags &= ~PROMPT;
	tell_player(p, "\nYou will now be asked a series of questions in order to \n"
		       " complete the residency process.\n");

	tell_player(p, "\n We will start with your EMAIL address.\n");

	do_prompt(p, "Enter your email address now:");
	p->input_to_fn = begin_ressie_doemail;
}

void            newsetpw2(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"got_password (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   password_mode_off(p);
   p->input_to_fn = 0;
   p->flags |= PROMPT;
   if (strcmp(p->password_cpy, str))
   {
      tell_player(p, "\n But that doesn't match !!!\n"
                     " Password not changed ...\n");
   	p->flags &= ~PROMPT;
	password_mode_on(p);
	do_prompt(p, " Please enter a new password:");
	p->input_to_fn = newsetpw1;
   } else
   {
      strcpy(p->password, do_crypt(str, p));
      tell_player(p, "\n Password has now been changed.\n");
      if (p->email[0] != 2)
         p->residency &= ~NO_SYNC;
      save_player(p);
         strncpy(p->saved->email, p->email, MAX_EMAIL - 3);
   }
}

void            newsetpw1(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"got_password1 (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (strlen(str) > (MAX_PASSWORD - 3))
   {
      do_prompt(p, "\n Password too long, please try again.\n"
                   " Please enter a shorter password:");
      p->input_to_fn = newsetpw1;
   } else if (*str)
   {
      strcpy(p->password_cpy, str);
      do_prompt(p, "\n Enter password again to verify:");
      p->input_to_fn = newsetpw2;
   } else {
      do_prompt(p, "\n\n Passwords of zero length aren't very secure. Please try again.\n");
      do_prompt(p, " Please enter a new password:");
      p->input_to_fn = newsetpw1;
  }
}


/* set birthday -- american style */

void            set_birthday(player * p, char *str)
{
   char           *oldstack;
   struct tm       bday;
   struct tm      *tm_time;
   time_t          the_time;
   int             t;

#ifdef TRACK
   sprintf(functionin,"set_birthday(%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   the_time = time(0);
   tm_time = localtime(&the_time);
   oldstack = stack;
   if (!*str)
   {
      tell_player(p, " Format: birthday <month>/<day>(/<year>)\n");
      return;
   }
   memset((char *) &bday, 0, sizeof(struct tm));
   bday.tm_year = tm_time->tm_year;
   bday.tm_mon = atoi(str);

   if (!bday.tm_mon)
   {
      tell_player(p, " Birthday cleared.\n");
      p->birthday = 0;
      return;
   }
   if (bday.tm_mon <= 0 || bday.tm_mday > 12)
   {
      tell_player(p, " Not a valid month.\n");
      return;
   }
   bday.tm_mon--;
   while (isdigit(*str))
      str++;
   str++;
   bday.tm_mday = atoi(str);
   if (bday.tm_mday <= 0 || bday.tm_mday > 31)
   {
      tell_player(p, " Not a valid day of the month.\n");
      return;
   }

   while (isdigit(*str))
      str++;
   str++;
   while (strlen(str) > 2)
      str++;
   bday.tm_year = atoi(str);
   if (bday.tm_year == 0)
   {
      bday.tm_year = tm_time->tm_year;
      p->birthday = TIMELOCAL(&bday);
   } else
   {
      p->birthday = TIMELOCAL(&bday);
      t = time(0) - (p->birthday);
      if (t > 0)
    p->age = t / 31536000;
   }

   sprintf(oldstack, " Your birthday is set to the %s.\n",
      birthday_string(p->birthday));
   stack = end_string(oldstack);
   tell_player(p, oldstack);
   stack = oldstack;
}

void            set_made_from(player * p, char *str)
{
   char           *oldstack;

#ifdef TRACK
   sprintf(functionin,"set_hometown (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (p->system_flags & NO_MSGS) {
	tell_player(p, " Cannot find command 'madefrom'\n"); 
	return;
   }
   oldstack = stack;
   strncpy(p->ingredients, str, MAX_SPODCLASS - 3);
   sprintf(stack, " You state that you are made from...\n%s\n", p->ingredients);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
}

