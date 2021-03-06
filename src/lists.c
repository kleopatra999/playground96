/*
 * lists.c
 */

#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <memory.h>

#include "config.h"
#include "player.h"
#include "fix.h"

/* externs */

extern char    *upper_from_saved(saved_player * sp);
extern void     check_list_newbie(char *lowername);
extern void     check_list_resident(player * p);
extern char    *check_legal_entry(player *, char *, int);
extern char    *list_lines(list_ent *);
extern char    *store_string(), *store_int();
extern char    *get_string(), *get_int();
extern char    *gstring_possessive(), *end_string(), *next_space();
extern player  *find_player_global_quiet(char *), *find_player_global(char *);
extern player  *find_player_absolute_quiet(char *);
extern saved_player *find_saved_player(char *);
extern void     log(char *, char *);
extern void     tell_player(player *, char *);
extern char    *number2string(int);
extern int      get_flag(flag_list *, char *);
extern char    *full_name(player *);
extern char    *get_gender_string(player *);
extern void     save_player(player *);
extern void     pager(player *, char *, int);
extern int      global_tag(player *, char *);
extern void     cleanup_tag(player **, int);
extern int	match_gag(player *, player *);

extern saved_player **saved_hash[];

/* interns */

flag_list       list_flags[] = {
   {"noisy", NOISY},
   {"ignore", IGNORE},
   {"inform", INFORM},
   {"grab", GRAB},
   {"grabme", GRAB},
   {"friend", FRIEND},
   {"bar", BAR},
   {"invite", INVITE},
   {"beep", BEEP},
   {"block", BLOCK},
   {"key", KEY},
   {"find", FIND},
   {"friendblock", FRIENDBLOCK},
   {"mailblock", MAILBLOCK},
   {"shareroom", SHARE_ROOM},
   {"nofriend", NO_FACCESS_LIST},
   {"nsy", NOISY},
   {"ign", IGNORE},
   {"inf", INFORM},
   {"grb", GRAB},
   {"fnd", FRIEND},
   {"inv", INVITE},
   {"bep", BEEP},
   {"blk", BLOCK},
   {"key", KEY},
   {"fin", FIND},
   {"fbk", FRIENDBLOCK},
   {"mbk", MAILBLOCK},
   {"shr", SHARE_ROOM},
   {"nof", NO_FACCESS_LIST},
{0, 0}};

flag_list       show_flags[] = {
   {"echo", TAG_ECHO},
   {"echos", TAG_ECHO},
   {"room", TAG_ROOM},
   {"local", TAG_ROOM},
   {"shout", TAG_SHOUT},
   {"shouts", TAG_SHOUT},
   {"tell", TAG_PERSONAL},
   {"tells", TAG_PERSONAL},
   {"autos", TAG_AUTOS},
   {"auto", TAG_AUTOS},
   {"login", TAG_LOGINS},
   {"logout", TAG_LOGINS},
   {"item", TAG_ITEMS},
   {"items", TAG_ITEMS},
{0, 0}};

char           *message_string;

/* delete and entry from someones list */

void            delete_entry(saved_player * sp, list_ent * l)
{
   list_ent       *scan;
   if (!sp)
      return;
   scan = sp->list_top;
   if (scan == l)
   {
      sp->list_top = l->next;
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
   log("error", "Tried to delete list entry that wasn't there.\n");
}


/* compress list */

void            tmp_comp_list(saved_player * sp)
{
   char           *oldstack;
   list_ent       *l, *next;

   l = sp->list_top;

   oldstack = stack;
   stack = store_int(stack, 0);

   while (l)
   {
      next = l->next;
      if (!l->name[0])
      {
    log("error", "Bad list entry on compress .. auto deleted.\n");
    delete_entry(sp, l);
      } else
      {
    stack = store_string(stack, l->name);
    stack = store_int(stack, l->flags);
      }
      l = next;
   }
   store_int(oldstack, ((int) stack - (int) oldstack));
}

void            compress_list(saved_player * sp)
{
   char           *oldstack;
   int             length;
   list_ent       *new, *l, *next;
   if (sp->system_flags & COMPRESSED_LIST)
      return;
   sp->system_flags |= COMPRESSED_LIST;
   oldstack = stack;
   tmp_comp_list(sp);
   length = (int) stack - (int) oldstack;
   if (length == 4)
   {
      sp->list_top = 0;
      stack = oldstack;
      return;
   }
   new = (list_ent *) MALLOC(length);
   memcpy(new, oldstack, length);

   l = sp->list_top;
   while (l)
   {
      next = l->next;
      FREE(l);
      l = next;
   }
   sp->list_top = new;
   stack = oldstack;
}

/* decompress list */

void            decompress_list(saved_player * sp)
{
   list_ent       *l;
   char           *old, *end, *start;
   int             length;

   if (!(sp->system_flags & COMPRESSED_LIST))
      return;
   sp->system_flags &= ~COMPRESSED_LIST;

   old = (char *) sp->list_top;
   start = old;
   if (!old)
      return;
   old = get_int(&length, old);
   end = old + length - 4;
   sp->list_top = 0;
   while (old < end)
   {
      l = (list_ent *) MALLOC(sizeof(list_ent));
      old = get_string(stack, old);
      strncpy(l->name, stack, MAX_NAME - 3);
      old = get_int(&(l->flags), old);
      l->next = sp->list_top;
      sp->list_top = l;
   }
   FREE(start);
}



/* save list */

void            construct_list_save(saved_player * sp)
{
   int             length;
   char           *where;

   if (!(sp->system_flags & COMPRESSED_LIST) &&
       (!find_player_absolute_quiet(sp->lower_name)))
      compress_list(sp);

   if (sp->system_flags & COMPRESSED_LIST)
   {
      if (sp->list_top)
      {
    where = (char *) sp->list_top;
    (void) get_int(&length, where);
    memcpy(stack, where, length);
    stack += length;
      } else
    stack = store_int(stack, 4);
   } else
      tmp_comp_list(sp);
}

/* retrieve list */

char           *retrieve_list_data(saved_player * sp, char *where)
{
   int             length;
   (void) get_int(&length, where);
   if (length == 4)
      sp->list_top = 0;
   else
   {
      sp->system_flags |= COMPRESSED_LIST;
      sp->list_top = (list_ent *) MALLOC(length);
      memcpy(sp->list_top, where, length);
   }
   where += length;
   return where;
}

/* general routine to output a flag field */

char           *bit_string(int flags)
{
   static char     out[33];
   int             i;
   for (i = 0; i < 32; i++, flags >>= 1)
      if (flags & 1)
    out[i] = '*';
      else
    out[i] = ' ';
   out[32] = 0;
   return out;
}


/* count list entries */

int             count_list(player * p)
{
   list_ent       *l;
   int             count = 0;
   if (!p->saved)
      return 0;
   if (!p->saved->list_top)
      return 0;
   for (l = p->saved->list_top; l; l = l->next)
      count++;
   return count;
}

/* find list entry for a person */

list_ent       *find_list_entry(player * p, char *name)
{
   list_ent       *l;

   if (!p->saved)
      return 0;
   decompress_list(p->saved);
   l = p->saved->list_top;
   while (l)
      if (!strcasecmp(l->name, name))
         return l;
      else
         l = l->next;
   return 0;
}

/* create a list entry */

list_ent       *create_entry(player * p, char *name)
{
   list_ent       *l, *e, *s, *n;
   if (!p->saved)
      return 0;
   if ((count_list(p)) >= (p->max_list))
   {
      tell_player(p, " Can't create new list entry, "
                     "because your list is full.\n");
      return 0;
   }
   l = (list_ent *) MALLOC(sizeof(list_ent));
   strncpy(l->name, name, MAX_NAME - 3);
   e = find_list_entry(p, "everyone");
   if (e && strcasecmp(l->name,"sus") && strcasecmp(l->name,"newbies"))
      l->flags = e->flags;
   else
      l->flags = 0;
   l->next = p->saved->list_top;
   p->saved->list_top = l;
   return l;
}


/* find list entry for a person from saved file */

list_ent       *fle_from_save(saved_player * sp, char *name)
{
   list_ent       *l;
   decompress_list(sp);
   l = sp->list_top;
   while (l)
      if (!strcasecmp(l->name, name))
    return l;
      else
    l = l->next;
   return 0;
}


/* clear bits of list */

void            clear_list(player * p, char *str)
{
   char           *oldstack, *text, msg[50];
   int             count = 0;
   list_ent       *l;

   oldstack = stack;

   if (!p->saved)
   {
      tell_player(p, " You do not have a list to alter, "
                     "since you have no save file\n");
      return;
   }
   if (!*str)
   {
      tell_player(p, " Format: clist <person>\n");
      return;
   }
   while (*str)
   {
      while (*str && *str != ',' && *str != ' ')
         *stack++ = *str++;
      if (*str)
         str++;
      *stack++ = 0;
      if (*oldstack)
      {
         l = find_list_entry(p, oldstack);
         if (!l)
         {
            text = stack;
            sprintf(stack, " Can't find any list entry for '%s'.\n", oldstack);
            stack = end_string(stack);
            tell_player(p, text);
         } else
         {
            count++;
            sprintf(msg, " Entry removed for '%s'\n", l->name);
            tell_player(p, msg);
            delete_entry(p->saved, l);
         }
      }
      stack = oldstack;
   }
   if (!count)
      tell_player(p, " No entries removed.\n");
   else
   {
      if (count != 1)
         sprintf(stack, " Deleted %s entries.\n", number2string(count));
      else
         strcpy(stack, " Deleted one entry.\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
   }
   stack = oldstack;
}


/* turn a flag list into an int */

int             get_flags(player * p, char *flags, flag_list * list)
{
   int             flag = 0, new;
   char           *oldstack, *text;

   oldstack = stack;
   while (*flags && *flags == ' ')
      flags++;
   while (*flags)
   {
      while (*flags && *flags != ',')
    *stack++ = *flags++;
      if (*flags)
    flags++;
      *stack++ = 0;
      new = get_flag(list, oldstack);
      if (!new)
      {
         text = stack;
         sprintf(stack, " Can't find flag '%s'.\n", oldstack);
         stack = end_string(stack);
         tell_player(p, text);
      } else
         flag |= new;
      stack = oldstack;
   }
   return flag;
}


/* force change on list */

void            change_list_absolute(player * p, char *str)
{
   char           *flag_list, *oldstack, msg[50], *dummyname;
   int             change, count = 0;
   list_ent       *l;
   player         *tell;

   if (!p->saved)
   {
      tell_player(p, " You haven't got a list since you have no save file.\n");
      return;
   }
   oldstack = stack;
   flag_list = next_space(str);
   if (!*flag_list)
   {
      tell_player(p, " Format: flist <player list> <flag list>\n");
      return;
   }
   *flag_list++ = 0;
   change = get_flags(p, flag_list, list_flags);
   if (!change)
   {
      tell_player(p, " Bad flag list, no changes made.\n");
      return;
   }
   while (*str)
   {
      while (*str && *str != ',')
         *stack++ = *str++;
      *stack++ = 0;
      if (*str)
         str++;
      if (*oldstack && (dummyname = check_legal_entry(p, oldstack, 0)))
      {
         sprintf(msg, " Flag set for '%s'\n", dummyname);
         tell_player(p, msg);
         l = find_list_entry(p, dummyname);
         if (!l)
            l = create_entry(p, dummyname);
         if (l)
         {
            l->flags = change;
            count++;
            if (message_string)
            {
               tell = find_player_absolute_quiet(oldstack);
               if (tell)
                  tell_player(tell, message_string);
            }
         }
      }
      stack = oldstack;
   }
   if (count)
      tell_player(p, " List entries force changed.\n");
   else
      tell_player(p, " No changes to make.\n");
}



/* set change on list */

void            set_list(player * p, char *str)
{
   char           *flag_list, *oldstack, msg[50], *dummyname;
   int             change, count = 0;
   list_ent       *l;
   player         *tell;
   if (!p->saved)
   {
      tell_player(p, " You haven't got a list since you have no save file.\n");
      return;
   }
   oldstack = stack;
   flag_list = next_space(str);
   if (!*flag_list)
   {
      tell_player(p, " Format: slist <player list> <flag list>\n");
      return;
   }
   *flag_list++ = 0;
   change = get_flags(p, flag_list, list_flags);
   if (!change)
   {
      tell_player(p, " Bad flag list, no changes made.\n");
      return;
   }
   while (*str)
   {
      while (*str && *str != ',')
    *stack++ = *str++;
      *stack++ = 0;
      if (*str)
    str++;
      if (*oldstack && (dummyname = check_legal_entry(p, oldstack, 0)))
      {
         sprintf(msg, " Flag set for '%s'\n", dummyname);
         tell_player(p, msg);
         l = find_list_entry(p, dummyname);
         if (!l)
            l = create_entry(p, dummyname);
         if (l)
         {
            l->flags |= change;
            count++;
            if (message_string)
            {
               tell = find_player_absolute_quiet(oldstack);
               if (tell)
                  tell_player(tell, message_string);
            }
         }
      }
      stack = oldstack;
   }
   if (count)
      tell_player(p, " List entries set.\n");
   else
      tell_player(p, " No changes to make.\n");
}


/* reset change on list */

void            reset_list(player * p, char *str)
{
   char           *flag_list, *oldstack, msg[50], *dummyname;
   int             change, count = 0;
   list_ent       *l;
   player         *tell;

   if (!p->saved)
   {
      tell_player(p, " You haven't got a list since you have no save file.\n");
      return;
   }
   oldstack = stack;
   flag_list = next_space(str);
   if (!*flag_list)
   {
      tell_player(p, " Format: rlist <player list> <flag list>\n");
      return;
   }
   *flag_list++ = 0;
   change = get_flags(p, flag_list, list_flags);
   if (!change)
   {
      tell_player(p, " Bad flag list, no changes made.\n");
      return;
   }
   while (*str)
   {
      while (*str && *str != ',')
         *stack++ = *str++;
      *stack++ = 0;
      if (*str)
         str++;
      if (*oldstack && (dummyname = check_legal_entry(p, oldstack, 0)))
      {
         sprintf(msg, " Flags reset for '%s'\n", dummyname);
         tell_player(p, msg);
         l = find_list_entry(p, dummyname);
         if (!l)
            l = create_entry(p, dummyname);
         if (l)
         {
            l->flags &= ~change;
            if (!l->flags)
               delete_entry(p->saved, l);
            count++;
            if (message_string)
            {
               tell = find_player_absolute_quiet(oldstack);
               if (tell)
                  tell_player(tell, message_string);
            }
         }
      }
      stack = oldstack;
   }
   if (count)
      tell_player(p, " List entries reset.\n");
   else
      tell_player(p, " No changes to make.\n");
}

/* toggle change on list */

void            toggle_list(player * p, char *str)
{
   char           *flag_list, *oldstack, msg[50], *dummyname;
   int             change, count = 0;
   list_ent       *l;
   player         *tell;

   if (!p->saved)
   {
      tell_player(p, " You haven't got a list since you have no save file.\n");
      return;
   }
   oldstack = stack;
   flag_list = next_space(str);
   if (!*flag_list)
   {
      tell_player(p, " Format: tlist <player list> <flag list>\n");
      return;
   }
   *flag_list++ = 0;
   change = get_flags(p, flag_list, list_flags);
   if (!change)
   {
      tell_player(p, " Bad flag list, no changes made.\n");
      return;
   }
   while (*str)
   {
      while (*str && *str != ',')
    *stack++ = *str++;
      *stack++ = 0;
      if (*str)
    str++;
      lower_case(oldstack);
      if (*oldstack && (dummyname = check_legal_entry(p, oldstack, 0)))
      {
         sprintf(msg, " Flag toggled for '%s'\n", dummyname);
         tell_player(p, msg);
         l = find_list_entry(p, dummyname);
         if (!l)
            l = create_entry(p, dummyname);
         if (l)
         {
            l->flags ^= change;
            if (!l->flags)
               delete_entry(p->saved, l);
            count++;
            if (message_string && l->flags & change)
            {
               tell = find_player_absolute_quiet(oldstack);
               if (tell)
                  tell_player(tell, message_string);
            }
         }
      }
      stack = oldstack;
   }
   if (count)
      tell_player(p, " List entries toggled.\n");
   else
      tell_player(p, " No changes to make.\n");
}



void            noisy(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: noisy <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " noisy");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s removes you from %s noisy list.\n",
              p->name, gstring_possessive(p));
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s adds you to %s noisy list.\n",
              p->name, gstring_possessive(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s adds you to %s noisy list)\n",
           p->name, gstring_possessive(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   message_string = 0;
   stack = oldstack;
}


void            ignore(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: ignore <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " ignore");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s starts listening to you again.\n", p->name);
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s ignores you.\n", p->name);
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s ignores you)\n", p->name);
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}


void            block(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: block <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " block");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s starts listening to you again.\n", p->name);
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s blocks you off from doing tells to %s.\n",
         p->name, get_gender_string(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s blocks you off from doing tells to %s)\n",
      p->name, get_gender_string(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}

void            inform(player * p, char *str)
{
   char           *oldstack;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: inform <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " inform");
   stack = strchr(stack, 0);
   *stack++ = 0;
   if (!strcasecmp("off", str))
   {
      reset_list(p, oldstack);
      stack = oldstack;
      return;
   }
   if (!strcasecmp("on", str))
   {
      set_list(p, oldstack);
      stack = oldstack;
      return;
   }
   toggle_list(p, oldstack);
   stack = oldstack;
}

void            grab(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: grab <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " grab");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s stops you from being able to grab %s.\n", p->name, get_gender_string(p));
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s allows you to grab %s.\n", p->name, get_gender_string(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s allows you to grab %s)\n", p->name, get_gender_string(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}

void            friend(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: friend <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " friend");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s doesn't like you any more.\n", p->name);
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s makes you %s friend.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s makes you %s friend)\n", p->name, gstring_possessive(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}

void            bar(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: bar <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " bar");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s allows you back into %s rooms.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s bars you from %s rooms.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s bars you from %s rooms)\n", p->name, gstring_possessive(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}

void            invite(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: invite <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " invite");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s strikes you off %s invite list.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s enters you onto %s invite list.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s enters you onto %s invite list)\n", p->name, gstring_possessive(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}

void            beep(player * p, char *str)
{
   char           *oldstack;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: beep <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " beep");
   stack = strchr(stack, 0);
   *stack++ = 0;
   if (!strcasecmp("off", str))
   {
      reset_list(p, oldstack);
      stack = oldstack;
      return;
   }
   if (!strcasecmp("on", str))
   {
      set_list(p, oldstack);
      stack = oldstack;
      return;
   }
   toggle_list(p, oldstack);
   stack = oldstack;
}

void            key(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: key <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " key");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s snatches %s key back off of you.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s presents you with the key to %s rooms.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s presents you with the key to %s rooms)\n", p->name, gstring_possessive(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}

/* toggle whether iacga's are sent or not */

void            toggle_iacga(player * p, char *str)
{
   if (!strcasecmp("off", str))
   {
      p->system_flags &= ~IAC_GA_ON;
      p->flags &= ~IAC_GA_DO;
   } else if (!strcasecmp("on", str))
   {
      p->system_flags |= IAC_GA_ON;
      if (p->flags & EOR_ON)
    p->flags |= IAC_GA_DO;
   } else
   {
      p->system_flags ^= IAC_GA_ON;
      p->flags ^= IAC_GA_DO;
      if (p->flags & EOR_ON)
    p->flags &= ~IAC_GA_DO;
   }

   if (p->system_flags & IAC_GA_ON)
   {
      if (p->flags & IAC_GA_DO)
         tell_player(p, " You will get IAC GA controls after each prompt.\n");
      else
         tell_player(p, " You will get IAC GA's on prompts if IAC EOR is "
                        "unavailable.\n");
   } else
      tell_player(p, " You will not get IAC GA controls after each prompt.\n");
}


/* toggle earmuffs and blocktells on and off */

void            block_ftells(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~BLOCK_FRIENDS;
   else if (!strcasecmp("on", str))
      p->tag_flags |= BLOCK_FRIENDS;
   else
      p->tag_flags ^= BLOCK_FRIENDS;

   if (p->tag_flags & BLOCK_FRIENDS)
      tell_player(p, " You are now ignoring the gossip.\n");
   else
      tell_player(p, " You decide that gossip isn't that bad...\n");
}

void            earmuffs(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~BLOCK_SHOUT;
   else if (!strcasecmp("on", str))
      p->tag_flags |= BLOCK_SHOUT;
   else
      p->tag_flags ^= BLOCK_SHOUT;

   if (p->tag_flags & BLOCK_SHOUT)
      tell_player(p, " You are wearing earmuffs.\n");
   else
      tell_player(p, " You are not wearing earmuffs.\n");
}

/* toggle the pager on and off */

void            nopager(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->custom_flags &= ~NO_PAGER;
   else if (!strcasecmp("on", str))
      p->custom_flags |= NO_PAGER;
   else
      p->custom_flags ^= NO_PAGER;
   if (p->custom_flags & NO_PAGER)
      tell_player(p, " You will not get paged output.\n");
   else
      tell_player(p, " You will get paged output.\n");
}


void            blocktells(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~BLOCK_TELLS;
   else if (!strcasecmp("on", str))
      p->tag_flags |= BLOCK_TELLS;
   else
      p->tag_flags ^= BLOCK_TELLS;

   if (p->tag_flags & BLOCK_TELLS)
      tell_player(p, " Tells to you will get blocked.\n");
   else
      tell_player(p, " Tells to you will not get blocked.\n");

}

/* toggle whether someone is hiding or not */


void            hide(player * p, char *str)
{

   if (!strcasecmp("off", str))
      p->custom_flags &= ~HIDING;
   else if (!strcasecmp("on", str))
      p->custom_flags |= HIDING;
   else
      p->custom_flags ^= HIDING;

   if (p->custom_flags & HIDING)
      tell_player(p, " You hide yourself away from prying eyes.\n");
   else
      tell_player(p, " People can find out where you are.\n");
}

/* toggle whether someone gets trans straight home */

void            straight_home(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->custom_flags &= ~TRANS_TO_HOME;
   else if (!strcasecmp("on", str))
      p->custom_flags |= TRANS_TO_HOME;
   else
      p->custom_flags ^= TRANS_TO_HOME;

   if (p->custom_flags & TRANS_TO_HOME)
      tell_player(p, " You will be taken to your home when you log in.\n");
   else
      tell_player(p, " You get taken to the entrance room when you log in.\n");
   strcpy(p->room_connect, "");
}

/* toggle see echo on and off */

void            see_echo(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~SEEECHO;
   else if (!strcasecmp("on", str))
      p->tag_flags |= SEEECHO;
   else
      p->tag_flags ^= SEEECHO;

   if (p->tag_flags & SEEECHO)
      if (p->residency & (SU | ADMIN))
         tell_player(p, " You will now see who echos what.\n");
      else
         tell_player(p, " You will now see who echos what in public rooms.\n");
   else
      tell_player(p, " You will no longer see who echos what.\n");
}

/* toggle whether someone can receive anonymous mail */

void            toggle_anonymous(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~NO_ANONYMOUS;
   else if (!strcasecmp("on", str))
      p->tag_flags |= NO_ANONYMOUS;
   else
      p->tag_flags ^= NO_ANONYMOUS;

   if (p->tag_flags & NO_ANONYMOUS)
      tell_player(p, " You will not be able to receive anonymous mail.\n");
   else
      tell_player(p, " You will be able to receive anonymous mail.\n");
   save_player(p);
}


/* toggle whether someone gets informed of news */

void            toggle_news_inform(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->custom_flags &= ~NEWS_INFORM;
   else if (!strcasecmp("on", str))
      p->custom_flags |= NEWS_INFORM;
   else
      p->custom_flags ^= NEWS_INFORM;

   if (p->custom_flags & NEWS_INFORM)
      tell_player(p, " You will be informed of new news.\n");
   else
      tell_player(p, " You will not be informed of new news.\n");
}


/* toggle whether someone gets informed of mail */

void            toggle_mail_inform(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->custom_flags &= ~MAIL_INFORM;
   else if (!strcasecmp("on", str))
      p->custom_flags |= MAIL_INFORM;
   else
      p->custom_flags ^= MAIL_INFORM;

   if (p->custom_flags & MAIL_INFORM)
      tell_player(p, " You will be informed when you receive mail.\n");
   else
      tell_player(p, " You will not be informed when you receive mail.\n");
}

void            toggle_friend_inform(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->custom_flags &= ~YES_QWHO_LOGIN;
   else if (!strcasecmp("on", str))
      p->custom_flags |= YES_QWHO_LOGIN;
   else
      p->custom_flags ^= YES_QWHO_LOGIN;

   if (p->custom_flags & YES_QWHO_LOGIN)
      tell_player(p, " You will get fwho automatically on login.\n");
   else
      tell_player(p, " You will not be informed of your fwho automatically.\n");
}

/* see single entry */

void view_single_list(player * p, char *str)
{
   char *oldstack;
   list_ent *scan;
   int wibble = 0;

   oldstack = stack;
   if (p->saved)
   {
      for (scan = p->saved->list_top; scan && !wibble; scan = scan->next)
      {
         if (!strcasecmp(scan->name, str))
         {
            sprintf(stack, "Your list entry for '%s'\n\n"
"                  : Nsy   Inf   Fnd   Inv   Blk   Fin   MBk   NoF\n"
" = Name =         :  | Ign | Grb | Bar | Bep | Key | FBk | ShR |\n\n", scan->name);
            stack = strchr(stack, 0);
            strcpy(stack, list_lines(scan));
            wibble = 1;
         }
      }
   } else
   {
      tell_player(p, " But you don't have a list!\n");
      stack = oldstack;
      return;
   }
   if (!wibble)
   {
      tell_player(p, " You dont have that name on your list ...\n");
      stack = oldstack;
      return;
   }
   stack = strchr(stack, 0);
   *stack++ = '\n';
   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}

/* print out p2's list to p */

void            do_list(player * p, saved_player * sp, int count)
{
  char           *oldstack;
  list_ent       *l, *every = 0, *sus=0, *newbies=0;
  
  if (count)
    {
      strcpy(stack, 
"\n                  : Nsy   Inf   Fnd   Inv   Blk   Fin   MBk   NoF\n"
  " = Name =         :  | Ign | Grb | Bar | Bep | Key | FBk | ShR |\n\n");
      stack = strchr(stack, 0);
      for (l = sp->list_top; l; l = l->next)
	{
	  if (!strcmp(l->name, "everyone"))
	    every=l;
	  else if (!strcmp(l->name, "sus"))
	    sus=l;
	  else if (!strcmp(l->name, "newbies"))
	    newbies=l;
	  else
	    {
	      strcpy(stack, list_lines(l));
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	}
      
      if (every || sus || newbies)
	{
	  strcpy(stack, "---------------------------------------------"
		 "-------------------\n");
	  stack = strchr(stack, 0);
	  if (sus)
	    {
	      strcpy(stack, list_lines(sus));
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	  if (newbies)
	    {
	      strcpy(stack, list_lines(newbies));
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	  if (every)
	    {
	      strcpy(stack, list_lines(every));
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	}
    }
}

/* view someone else's list, if you are Admin */

void            view_others_list(player * p, char *str)
{
   char           *oldstack, *arg2;
   int             count;
   player         *p2, dummy;
   list_ent       *l;

   oldstack = stack;
   memset(&dummy, 0, sizeof(player));

   if (!*str)
   {
      tell_player(p, " Format: vlist <player> [<entry>]\n");
      return;
   }

   arg2 = strchr(str, ' ');
   if (arg2)
   {
      *arg2++ = 0;
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
       tell_player(p, " That person has no list to view.\n");
       return;
   }
   count = count_list(p2);
   sprintf(stack, "%s is using %d of %s maximum %d list entries.\n",
      p2->name, count, their_player(p2), p2->max_list);
   stack = strchr(stack, 0);

   if (arg2)
   {
      l = find_list_entry(p2, arg2);
      if (l)
      {
         strcpy(stack, 
"\n                  : Nsy   Inf   Fnd   Inv   Blk   Fin   MBk   NoF\n"
  " = Name =         :  | Ign | Grb | Bar | Bep | Key | FBk | ShR |\n\n");
         stack = strchr(stack, 0);
         strcpy(stack, list_lines(l));
         stack = strchr(stack, 0);
         *stack++ = '\n';
         *stack++ = '\0';
      } else
      {
         sprintf(stack, " %s has no list entry for '%s'.\n", p2->name, str);
         stack = end_string(stack);
      }
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   do_list(p, p2->saved, count);
   *stack++ = 0;
   pager(p, oldstack, 0);
   stack = oldstack;
}

/* view list */

void            view_list(player * p, char *str)
{
  char           *oldstack;
  list_ent       *l, *every = 0, *sus=0, *newbies=0;
  int             count;
  player         *p2 = 0;
  
  oldstack = stack;
  /*
   * This is the bit that means that an Admin doing 'list <person>' gets that
   * person's list, rather than just that person's entry on their own list.
   * Maybe something to stick in another command?
   */
  /*
   * if (*str && p->residency&ADMIN) p2=find_player_global(str);
    */
  if (*str)
    {
      view_single_list(p, str);
      return;
    }
  if (!p2)
    p2 = p;
  
  if (!p2->saved)
    {
      tell_player(p2, " You have no list to view.\n");
      return;
    }
  count = count_list(p2);
  sprintf(stack, " You are using %d of your maximum %d list entries.\n",
	  count, p2->max_list);
  stack = strchr(stack, 0);
  if (count)
    {
      strcpy(stack, 
"\n                  : Nsy   Inf   Fnd   Inv   Blk   Fin   MBk   NoF\n"
  " = Name =         :  | Ign | Grb | Bar | Bep | Key | FBk | ShR |\n\n");
      stack = strchr(stack, 0);
      for (l = p2->saved->list_top; l; l = l->next)
	{
	  if (!strcmp(l->name, "everyone"))
	    every=l;
	  else if (!strcmp(l->name, "sus"))
	    sus=l;
	  else if (!strcmp(l->name, "newbies"))
	    newbies=l;
	  else
	    {
	      strcpy(stack, list_lines(l));
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	}
      
      if (every || sus || newbies)
	{
	  strcpy(stack, "---------------------------------------------"
		 "-------------------\n");
	  stack = strchr(stack, 0);
	  if (sus)
	    {
	      strcpy(stack, list_lines(sus));
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	  if (newbies)
	    {
	      strcpy(stack, list_lines(newbies));
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	  if (every)
	    {
	      strcpy(stack, list_lines(every));
	      stack = strchr(stack, 0);
	      *stack++ = '\n';
	    }
	}
      
    }
  *stack++ = 0;
  pager(p, oldstack, 0);
  stack = oldstack;
}

/* Print out flags for the list entries */

char           *list_lines(list_ent * l)
{
   int             count;
   static char     string[80];
   string[0] = 0;
   sprintf(string, "%-18s:", l->name);
   for (count = 1; count < 32768; count <<= 1)
   {
      if (l->flags & count)
    strcat(string, "  Y");
      else
    strcat(string, "  -");
   }
   return string;
}

int	entry_exists_for_player(player * p, player * p2) 
{
    list_ent	*l;

    if (!p2->saved) return 0;
    l = find_list_entry(p2, p->lower_name);
    if (l) return 1;
    l = find_list_entry(p2, "everyone");
    if (l) return 1;
    l = find_list_entry(p2, "sus");
    if (l && p->residency & PSU) return 1;
    l = find_list_entry(p2, "newbies");
    if (l && !(p->residency)) return 1;

    return 0;
}
/* view list of someone else */


void            do_entry(player * p, player * p2)
{
  int             count;
  list_ent       *l;
  
  *stack++ = '\n';
  
  strcpy(stack, p2->name);
  for (count = MAX_NAME + 1; *stack; count--)
    stack++;
  for (; count > 3; count--)
    *stack++ = ' ';
  
  if (!p2->saved)
    {
      strcpy(stack, "    : No list");
      stack = strchr(stack, 0); 
      return;
    }
  l = find_list_entry(p2, p->lower_name);
  if (l)
    {
      *stack++ = ' ';
      *stack++ = ' ';
      *stack++ = ' ';
      *stack++ = ' ';
    }
  
  if (!l && p->residency & SU)
    {
      l = find_list_entry(p2,"sus");
      if (l)
	{
	  *stack++ = '(';
	  *stack++ = 'S';
	  *stack++ = ')';
	  *stack++ = ' ';
	}
    }
  if (!l && p->residency == NON_RESIDENT)
    {
      l = find_list_entry(p2,"newbies");
      if (l)
	{
	  *stack++ = '(';
	  *stack++ = 'N';
	  *stack++ = ')';
	  *stack++ = ' ';
	}
    }
  if (!l)
    {
      l = find_list_entry(p2, "everyone");
      if (!l)
	{
	  strcpy(stack, "    : No entry for you");
	  stack = strchr(stack, 0); 
	  return;
	}
      *stack++ = '(';
      *stack++ = 'E';
      *stack++ = ')';
      *stack++ = ' ';
    }
  
  *stack++ = ':';
  for (count = 1; count < 32768; count <<= 1)
    {
      if (l->flags & count)
	strcpy(stack, "  Y");
      else
	strcpy(stack, "  -");
      stack = strchr(stack, 0);
    }
}



void            check_entry(player * p, char *str)
{
   char           *oldstack, *text;
   player        **list, **scan;
   int             n, i, every = 0;

   oldstack = stack;
   command_type = SEE_ERROR;

   if (!*str)
   {
      tell_player(p, " Format: check entry <player(list)>\n");
      return;
   }
   if (!strcasecmp(str, "everyone"))    every = 1;
   align(stack);
   list = (player **) stack;

   n = global_tag(p, str);
   if (!n)
   {
      stack = oldstack;
      return;
   }
   text = stack;

   strcpy(stack, 
"                      : Nsy   Inf   Fnd   Inv   Blk   Fin   MBk   NoF\n"
" = Name =             :  | Ign | Grb | Bar | Bep | Key | FBk | ShR |");
   stack = strchr(stack, 0);
   for (scan = list, i = 0; i < n; i++, scan++)
      if (*scan != p && (!every || entry_exists_for_player(p, *scan)))
    do_entry(p, *scan);
   *stack++ = '\n';
   *stack++ = 0;
   cleanup_tag(list, n);
   pager(p, text, 0);
   stack = oldstack;
}


/*
 * void construct_list_save(saved_player *sp) { list_ent *l,*next;
 * 
 * l=sp->list_top; while(l) { next=l->next; if (!l->name[0]) { log("error","Bad
 * list entry on save .. auto deleted.\n"); delete_entry(sp,l); } else {
 * stack=store_string(stack,l->name); stack=store_int(stack,l->flags); }
 * l=next; } stack=store_string(stack,""); } */

/* retrieve list */
/*
 * char *retrieve_list_data(saved_player *sp,char *where) { list_ent *l;
 * stack=1; sp->list_top=0; while(*stack) { where=get_string(stack,where); if
 * (*stack) { l=(list_ent *)MALLOC(sizeof(list_ent));
 * strncpy(l->name,stack,MAX_NAME-2); where=get_int(&(l->flags),where);
 * l->next=sp->list_top; sp->list_top=l; } } return where; }
 */

/* see if a particular player can recieve a particular message */

int             test_receive(player * p)
{
  int             cerror = 1;
  list_ent       *l;
  char           *oldstack;
  
  oldstack = stack;
  
  if (!current_player)
    return 1;
  if (p == current_player)
    return 1;
  if (current_player->residency & ADMIN && command_type & PERSONAL &&
      (!(sys_flags & EVERYONE_TAG) && !(sys_flags & FRIEND_TAG) && !(sys_flags & OFRIEND_TAG)))
    return 1;
  
  if (command_type & EMERGENCY)
    return 1;
  
  if (command_type & PERSONAL)
    {
      if (command_type & WARNING)
	{
	  return 1;
	}
      l = find_list_entry(current_player, p->lower_name);
      if (!l)
	if (p->residency & SU)
	  l = find_list_entry(current_player, "sus");
      if (!l)
	if (p->residency == NON_RESIDENT)
	  l = find_list_entry(current_player, "newbies");
      if (!l)
	l = find_list_entry(current_player, "everyone");
      if ((l && (l->flags & IGNORE || l->flags & BLOCK) && !(l->flags & NOISY))
		|| match_gag(current_player,p))
	{
	  if (command_type & SEE_ERROR)
	    { 
	      sprintf(stack, "You can't send to %s because you "
		      "can't hear %s.\n", p->name, get_gender_string(p));
	      stack = end_string(stack);
	      tell_current(oldstack);
	    }
	  sys_flags |= FAILED_COMMAND;
	  stack = oldstack;
	  return 0;
	}
      if (p->location && p->location->flags & ISOLATED_ROOM) 
	{
	  if (command_type & SEE_ERROR)
	    { 
	      sprintf(stack, "You can't get to %s because %s "
		      "%s in an isolated room.\n", p->name, gstring(p), isare(p));
	      stack = end_string(stack);
	      tell_current(oldstack);
	    }
	  sys_flags |= FAILED_COMMAND;
	  stack = oldstack;
	  return 0;
	}
    }
  l = find_list_entry(p, current_player->lower_name);
  if (!l)
    if (current_player->residency & SU)
      l = find_list_entry(p, "sus");
  if (!l)
    if (current_player->residency == NON_RESIDENT)
      l = find_list_entry(p, "newbies");
  if (!l)
    l = find_list_entry(p, "everyone");
  
  /* if (p->location != current_player->location)
    { */
      if (sys_flags & ITEM_TAG && p->tag_flags & BLOCK_ITEMS)
	return 0;
      if (command_type & (LOGIN_TAG|LOGOUT_TAG|RECON_TAG) 
		&& p->tag_flags & BLOCK_LOGINS)
	return 0;
      if ((p->tag_flags & BLOCK_TELLS) && (!l || !(l->flags & NOISY)) &&
          (command_type & PERSONAL))
	{
	  if (command_type & SEE_ERROR)
	    {
	      sprintf(stack, "%s foils you with a blocktell.\n", p->name);
	      stack = end_string(stack);
	      tell_current(oldstack);
	      stack = oldstack;
	    }
	  sys_flags |= FAILED_COMMAND;
	  return 0;
	}
      if ((p->tag_flags & BLOCK_FRIENDS || (l && l->flags & FRIENDBLOCK))
	 && (!l || !(l->flags & NOISY)) && (sys_flags & (FRIEND_TAG|OFRIEND_TAG)))
	{
	  sys_flags |= FAILED_COMMAND;
	  return 0;
	}
     if (p->tag_flags & SINGBLOCK && command_type & BAD_MUSIC)
	{
		if(command_type & SEE_ERROR)
		{
		  sprintf(stack, "%s is blocking all singing\n", p->name);
	      stack = end_string(stack);
	      tell_current(oldstack);
	      stack = oldstack;
		}
	  sys_flags |= FAILED_COMMAND;
	  return 0;
	}
     if (p->location && p->location->flags & ANTISING && command_type & BAD_MUSIC)
	{
		if(command_type & SEE_ERROR)
		{
		  sprintf(stack, "%s is in a sing-proof room.\n", p->name);
	      stack = end_string(stack);
	      tell_current(oldstack);
	      stack = oldstack;
		}
	  sys_flags |= FAILED_COMMAND;
	  return 0;
	}
	
	
      if ((p->tag_flags & BLOCK_SHOUT || (p->location && p->location->flags & SOUNDPROOF)) 
		&& (command_type & EVERYONE || sys_flags & EVERYONE_TAG))
	/*
	   && (!l || !(l->flags & NOISY)))
	   */
	return 0;
  /* } else { */
      if ( p->tag_flags & BLOCK_SHOUT && command_type & EVERYONE )
	return 0;
	/* } */

  if (match_gag(p, current_player))
    {
      if (command_type & SEE_ERROR && cerror)
	{
	  if (strlen(p->ignore_msg) > 0)
            sprintf(stack, "%s\n", p->ignore_msg);
	  else
            sprintf(stack, "You are gagged by %s.\n", p->name);
	  stack = end_string(stack);
	  tell_current(oldstack);
	}
      sys_flags |= FAILED_COMMAND;
      stack = oldstack;
      return 0;
    }

  if (!l)
    return 1;
  
  if (l->flags & BLOCK && !(l->flags & NOISY)
      && command_type & PERSONAL)
    {
      if (command_type & SEE_ERROR)
	{
	  sprintf(stack, "You are blocked by %s.\n", p->name);
	  stack = end_string(stack);
	  tell_current(oldstack);
	}
      sys_flags |= FAILED_COMMAND;
      stack = oldstack;
      return 0;
    }
  if (sys_flags & ROOM_TAG || sys_flags & EVERYONE_TAG
      || sys_flags & (FRIEND_TAG|OFRIEND_TAG))
    cerror = 0;
  
  if ((l->flags & IGNORE && !(l->flags & NOISY)))
    {
      if (command_type & SEE_ERROR && cerror)
	{
	  if (strlen(p->ignore_msg) > 0)
            sprintf(stack, "%s\n", p->ignore_msg);
	  else
            sprintf(stack, "You are ignored by %s.\n", p->name);
	  stack = end_string(stack);
	  tell_current(oldstack);
	}
      sys_flags |= FAILED_COMMAND;
      stack = oldstack;
      return 0;
    }
  return 1;
}

/* An attempt at getting the bugs out of test_receive() by junking it and
   starting from scratch *8-P */
/*

int new_test_receive(player *p)
{
  list_ent *l;
  char *oldstack;
  
  oldstack = stack;
  
  if (!current_player)
    return(1);
  if (current_player == p)
    return (1);
  if (current_player->residency & ADMIN && !(sys_flags & EVERYONE_TAG)
      && command_type & PERSONAL)
    return (1);
  
  if (command_type & PERSONAL)
    {
      l = find_list_entry(current_player, p->lower_name);
      if (!l)
	if (p->residency & SU)
	  l = find_list_entry(current_player, "sus");
      if (!l)
	if (p->residency == NON_RESIDENT)
	  l = find_list_entry(current_player, "newbies");
      if (!l)
	l = find_list_entry(current_player, "everyone");
      if (l)
	{
	  if (l->flags & IGNORE)
	    {
	      sprintf(stack, "You can't get a message to %s because you are"
		      " ignoring %s.\n", p->name, get_gender_string(p));
	      stack = end_string(stack);
	      tell_current(oldstack);
	      stack = oldstack;
	      return (0);
	    }
	}
      if (current_player->tag_flags & BLOCK_TELLS)
	{
	  l = find_list_entry(current_player, p->lower_name);
	  if (!l)
	    if (p->residency & SU)
	      l = find_list_entry(current_player, "sus");
	  if (!l)
	    if (p->residency == NON_RESIDENT)
	      l = find_list_entry(current_player, "newbies");
	  if (!l)
	    l = find_list_entry(current_player, "everyone");
	  if(l)
	    {
	      if (!(l->flags & NOISY))
		return (0);
	    }
	}
      if (p->tag_flags & BLOCK_TELLS)
	{
	  l = find_list_entry(p, current_player->lower_name);
	  if (!l)
	    if (current_player->residency & SU)
	      l = find_list_entry(p, "sus");
	  if (!l)
	    if (current_player->residency == NON_RESIDENT)
	      l = find_list_entry(p, "newbies");
	  if (!l)
            l = find_list_entry(p, "everyone");
	  if (l)
            if (l->flags & NOISY)
	      return (1);
	  return (0);
	}
      return (1);
    }
  
  if (command_type & ROOM)
	{
      l = find_list_entry(p, current_player->lower_name);
      if (!l)
	if (current_player->residency & SU)
	  l = find_list_entry(p, "sus");
      if (!l)
	if (current_player->residency == NON_RESIDENT)
	  l = find_list_entry(p, "newbies");
      if (!l)
	l = find_list_entry(p, "everyone");
      if (l)
	if (l->flags & IGNORE)
	  return(0);
      return (1);      
    }

  if (command_type & EVERYONE)
    {
      if (p->tag_flags & BLOCK_SHOUT)
	return (0);
      return (1);
    }
  return(1);
  
} */


/* do inform bits */

void            do_inform(player * p, char *msg)
{
  player         *scan;
  char           *oldstack;
  list_ent       *l,*s;
  int type=0, f=0;
   
  oldstack = stack;
  
  if (!(p->location) || !(p->name[0]))
    return;
  
  for (scan = flatlist_start; scan; scan = scan->flat_next)
    {
      if (scan != p && !(scan->flags & PANIC) && scan->name[0] &&
	  scan->location)
	{
          type=0;   /* reinit it */
	  f=0;

	  if (!type && (p->residency & PSU || p->git_string[0]))
	    {
	      s=find_list_entry(scan,"sus");
	      if (s && s->flags & INFORM)
		type = 1;
	    }
	  l = find_list_entry(scan, p->lower_name);

	  /* if there is an individual entry, set f on */
          if (l)   f=1;

	  if (!l && (p->residency & PSU || p->git_string[0]))
	    {
	      l=find_list_entry(scan,"sus");
	    }
	  if (!l && (p->residency==NON_RESIDENT || p->git_string[0]))
	    {
	      l=find_list_entry(scan,"newbies");
	      if (l)
		type = 2;
	    }
	  if (!l)
	    {
	      l = find_list_entry(scan, "everyone");
	      command_type = LIST_EVERYONE;
	    }
	  if (l && l->flags & INFORM)
	    {
	      command_type = 0;
	      if (scan->residency & (TRACE | SU | PSU | ADMIN))
		sprintf(stack, msg, p->name, p->inet_addr);
	      else
		sprintf(stack, msg, p->name, "");
	      stack = strchr(stack, 0);

	      if (type==1)
              {
                 if (p->residency & ADMIN)
		    sprintf(stack," [Admin]");
                 else if (p->residency & LOWER_ADMIN)  
                    sprintf(stack," [Lower Admin]");
                 else if (p->residency & SU)  
                    sprintf(stack," [Super User]");
                 else if (p->residency & PSU)
                    sprintf(stack," [SU Channel]");
		 stack = strchr(stack, 0);
                 if (p->git_string[0])
                    sprintf(stack," [git] ");
	         stack=strchr(stack,0);
              }
              if (type==2 && !(find_list_entry(scan,"sus")))
                sprintf(stack," [git] ");
	      else if (type==2)
		sprintf(stack," [Newbie] ");

              /* Do the Surfers Fwiend Thingy - Nogs, 29/4/95 */
              /* Lets see if I dont screw this up too bad - asty 9/27/95 */ 
             if (f && (l->flags & FRIEND) && (scan->system_flags & MARRIED) && 
			(!strcasecmp(p->name, scan->married_to)))
             {
                if (p->gender == MALE)
                 sprintf(stack," [Husband]");
                else if (p->gender == FEMALE)
                 sprintf(stack," [Wife]");
                else
                 sprintf(stack," [Spouse]");
             }
             else if (f && l->flags & FRIEND)
                 sprintf(stack," [Friend]");

	      stack=strchr(stack,0);

	      if (l->flags & BEEP)
		*stack++ = 7;
	      *stack++ = '\n';
	      *stack++ = 0;
	      if (l->flags & FRIEND){
		sys_color_atm = FRTsc;
		command_type = (PERSONAL|NO_HISTORY);
		}
              tell_player(scan, oldstack);
	      command_type = 0;
	sys_color_atm = SYSsc;
	      stack = oldstack;
	    }
	}
    }
}


/* toggle what someone sees */

void            toggle_tags(player * p, char *str)
{
   char           *oldstack;
   int             change;

   oldstack = stack;
   if (!*str)
   {
      change = p->tag_flags & (TAG_ECHO | TAG_PERSONAL | TAG_ROOM | TAG_SHOUT | TAG_AUTOS | TAG_LOGINS | TAG_ITEMS);
      if (!change)
    tell_player(p, " You have no show flags enabled.\n");
      else
      {
    strcpy(stack, " You will get shown : ");
    stack = strchr(stack, 0);
    if (change & TAG_ECHO)
    {
       strcpy(stack, "echos, ");
       stack = strchr(stack, 0);
    }
    if (change & TAG_PERSONAL)
    {
       strcpy(stack, "tells to you, ");
       stack = strchr(stack, 0);
    }
    if (change & TAG_ROOM)
    {
       strcpy(stack, "room commands, ");
       stack = strchr(stack, 0);
    }
    if (change & TAG_ITEMS)
    {
       strcpy(stack, "item messages, ");
       stack = strchr(stack, 0);
    }
    if (change & TAG_SHOUT)
    {
       strcpy(stack, "shouts, ");
       stack = strchr(stack, 0);
    }
    if (change & TAG_LOGINS)
    {
       strcpy(stack, "logins, ");
       stack = strchr(stack, 0);
    }
    if (change & TAG_AUTOS)
    {
       strcpy(stack, "autos, ");
       stack = strchr(stack, 0);
    }
    *stack = 0;
    *(--stack) = '\n';
    *(--stack) = '.';
    stack = end_string(stack);
    tell_player(p, oldstack);
      }
      stack = oldstack;
      return;
   }
   change = get_flags(p, str, show_flags);
   if (!change)
   {
      tell_player(p, " Bad flag list, no changes made. \n"
                     " Possible flags are echo,tell,room,shout,login,item,autos\n");
      return;
   }
   p->tag_flags ^= change;
   tell_player(p, " Show flags entries toggled.\n");
}

/*
 * checks whether str is a person on the program (inc newbie) or if they are
 * in the playerfiles  Returns their uppercase name
 */

char           *check_legal_entry(player * p, char *str, int verbose)
{
   char           *oldstack, *store;
   static char     wibble[20];
   player         *plyr;
   saved_player   *splyr;

   oldstack = stack;
   store = str;
   while (*store)
      *store++ = tolower(*store);
   if (store = strchr(str, ' '))
      *store = 0;
   if (!strcasecmp(str, "newbies") && p->residency & PSU)
   {
      strcpy(wibble, "newbies");
      return wibble;
   }
   if (!strcasecmp(str, "sus") && p->residency & PSU)
   {
      strcpy(wibble, "sus");
      return wibble;
   }
   if (!strcasecmp(str, "everyone"))
   {
      strcpy(wibble, "everyone");
      return wibble;
   }

   if (strlen(str) > 20)
   {
      sprintf(stack, " Too long a name '%s'\n", str);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return 0;
   }
   if ((plyr = find_player_absolute_quiet(str)))
   {
      strcpy(wibble, plyr->name);
      return wibble;
   }
   if (!(splyr = find_saved_player(str)))
   {
      sprintf(stack, " There is no such person '%s'\n", str);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return 0;
   }
   if (!splyr)
   {
      log("error", "Spooned again chris. check_legal_entry");
      return 0;
   }
   if ((splyr->residency == SYSTEM_ROOM))
   {
      sprintf(stack, " Sorry, %s is a system name.\n", str);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return 0;
   }
   if (splyr->residency & BANISHD)
   {
      if (splyr->residency == BANISHD)
      {
         sprintf(stack, " Sorry, %s has been banished. (Name Only)\n", str);
      } else
      {
         sprintf(stack, " Sorry, %s has been banished.\n", str);
      }
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return 0;
   }
   strcpy(wibble, upper_from_saved(splyr));

   return wibble;
}

/* Look up a saved player, and return his/her uppercase name */

char           *upper_from_saved(saved_player * sp)
{
   char           *r;
   static char     uname[20];
   r = sp->data.where;
   get_string(uname, r);
   return uname;
}


/*
 * Need some stuff to delete entries for newbies who have logged off and to
 * wipe newbies from residents lists when they log off
 */


/*
 * This one is for when a resident logs off... all the newbies are stripped
 * from their list
 */


void            check_list_resident(player * p)
{
   player         *scan;
   list_ent       *l;

   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      l = find_list_entry(p, scan->lower_name);
      if (l && scan->residency == NON_RESIDENT)
    delete_entry(p->saved, l);
   }
}

/*
 * Now to strip all the entries of a newbie, when (s)he logs off
 */

void            check_list_newbie(char *lowername)
{
   player         *scan;
   list_ent       *l;

   for (scan = flatlist_start; scan; scan = scan->flat_next)
   {
      l = find_list_entry(scan, lowername);
      if (l)
    delete_entry(scan->saved, l);
   }
}

void            check_alist(player * p, char *str)
{
   char           *oldstack;
   int             count;
   list_ent       *l;
   player         *p2, dummy;

   oldstack = stack;
   memset(&dummy, 0, sizeof(player));

   if (!*str)
   {
      p2 = p;
      if (p->saved)
      {
    count = count_list(p);
    sprintf(stack, "You are using %d of your maximum %d list entries.\n",
       count, p->max_list);
    stack = strchr(stack, 0);
      } else
      {
    tell_player(p, " You have no list to check.\n");
    return;
      }
   } 

   else if (p->residency & (ADMIN | SU))
   {
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
	   } else
	       p2 = &dummy;
       }
       if (!p2->saved)
       {
	   tell_player(p, " That person has no list to check.\n");
	   return;
       }
       count = count_list(p2);
       sprintf(stack, " %s %s using %d of %s maximum %d entries.\n",
	       p2->name, isare(p2) ,count, their_player(p2), p2->max_list);
       stack = strchr(stack, 0);
   } else
   {
       tell_player(p, " You can't do that!\n");
       return;
   }
   
   
   for (l = p2->saved->list_top; l; l = l->next)
       if (!strcmp(l->name, "everyone"))
       {
	   strcpy(stack, 
"                  : Nsy   Inf   Fnd   Inv   Blk   Fin   MBk   NoF\n"
" = Name =         :  | Ign | Grb | Bar | Bep | Key | FBk | ShR |\n");
	   stack = strchr(stack, 0);
	   strcpy(stack, list_lines(l));
	   stack = strchr(stack, 0);
	   *stack++ = '\n';
       }
   *stack++ = 0;
   tell_player(p, oldstack);
   stack = oldstack;
}

void blank_list(player *p, char *str)
{
  if (!p->saved)
    {
      tell_player(p, " You do not have a list to alter, "
		  "since you have no save file\n");
      return;
    }
  
  /*Null the head of the list*/
  /*DONT free the other list, cos the reason the list is being blanked
    is probably due to a mislaid pointer and we dont want to go about
    deleting other useful memory*/
  p->saved->list_top=0;
  
  tell_player(p," List deleted\n");

  return;
}

 void            remove_from_others_list(player * p, char *str)
 {
    char            msg[50];
    int             change;
    list_ent       *l;
    player         *p2;
 
    if (!*str)
    {
       tell_player(p, " Format: rm_list <player(s)>\n");
       return;
    }
    p2 = find_player_absolute_quiet(str);
    if (!p2)
    {
       tell_player(p, " No one by that name on at the moment.\n");
       return;
    }
    
    /* un-subtle way to do it - BUT... it outta be more flexable... */
    change = get_flags(p2, "friend,find,invite,key,noisy,shareroom", list_flags);
    
    if (!change)
    {
       tell_player(p, " Bad flag list, no changes made.\n");
       return;
    }
    
    l = find_list_entry(p2, p->lower_name);
    if (!l)
    {
       tell_player(p, " Never mind - you're not on that person's list.\n");
       return;
    }
    
    if (l)
    {
       sprintf(msg, " OK. You're now cleared from %s's list.\n", p2->name);
       tell_player(p, msg);
       l->flags &= ~change;
       /* check to see if there's anything left. */
       if (!l->flags)
 	 delete_entry(p2->saved, l);
    }
    else tell_player(p, " Sorry. No changes to make.\n");
 }
 
void            toggle_singblock(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~SINGBLOCK;
   else if (!strcasecmp("on", str))
      p->tag_flags |= SINGBLOCK;
   else
      p->tag_flags ^= SINGBLOCK;

   if (p->tag_flags & SINGBLOCK)
      tell_player(p, " You are now blocking all music.\n");
   else
      tell_player(p, " You are no longer blocking music.\n");
}

void            block_beeps(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~NOBEEPS;
   else if (!strcasecmp("on", str))
      p->tag_flags |= NOBEEPS;
   else
      p->tag_flags ^= NOBEEPS;

   if (p->tag_flags & NOBEEPS)
      tell_player(p, " You are now ignoring beeps.\n");
   else
      tell_player(p, " You are no longer ignoring beeps.\n");
}

void            list_friendblock(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: friendblock <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " friendblock");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s resumes listening to your friendtells.\n", p->name);
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s blocks you off from doing friendtells to %s.\n", p->name, get_gender_string(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s blocks you off from doing friendtells to %s)\n", p->name, get_gender_string(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}
 
void            list_mailblock(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: mailblock <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " mailblock");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s unblocks your mail.\n", p->name);
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s redirects all your mail to /dev/null\n", p->name);
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s redirects all your mail to /dev/null)\n", p->name);
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}
 
void            list_shareroom(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: shareroom <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " shareroom");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s tears up your lease.\n", p->name);
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s grants you a lease to use %s rooms.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s grants you a lease to %s rooms)\n", p->name, gstring_possessive(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}
 
void            list_nofaccess(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: nofriend <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " nof");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s allows you to talk to %s friends.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s stops you from talking to %s friends.\n", p->name, gstring_possessive(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   sprintf(stack, " (%s stops you from talking to %s friends)\n", p->name, gstring_possessive(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}
 
void            hide_bachelor_status(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->system_flags &= ~BACHELOR_HIDE;
   else if (!strcasecmp("on", str))
      p->system_flags |= BACHELOR_HIDE;
   else
      p->system_flags ^= BACHELOR_HIDE;

   if (p->system_flags & BACHELOR_HIDE)
      tell_player(p, " You are hiding your bachelor status.\n");
   else
      tell_player(p, " You are no longer hiding bachelor status.\n");
}

void            global_nofriend(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~NO_FACCESS;
   else if (!strcasecmp("on", str))
      p->tag_flags |= NO_FACCESS;
   else
      p->tag_flags ^= NO_FACCESS;

   if (p->tag_flags & NO_FACCESS)
      tell_player(p, " You stop others from telling to your friends.\n");
   else
      tell_player(p, " You permit others to tell to your friends.\n");
}

void            declare_flirt(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->system_flags &= ~FLIRT_BACHELOR;
   else if (!strcasecmp("on", str))
      p->system_flags |= FLIRT_BACHELOR;
   else
      p->system_flags ^= FLIRT_BACHELOR;

   if (p->system_flags & FLIRT_BACHELOR)
      tell_player(p, " You declare yourself a net.flirt.\n");
   else
      tell_player(p, " You are no longer a net.flirt (or are hiding the fact).\n");
}
void            friend_mailblock(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~BLOCK_FRIEND_MAIL;
   else if (!strcasecmp("on", str))
      p->tag_flags |= BLOCK_FRIEND_MAIL;
   else
      p->tag_flags ^= BLOCK_FRIEND_MAIL;

   if (p->tag_flags & BLOCK_FRIEND_MAIL)
      tell_player(p, " You block all friendmail.\n");
   else
      tell_player(p, " You unblock friendmail.\n");

   save_player(p);
}








void            listfind(player * p, char *str)
{
   char           *oldstack, *cmd;
   oldstack = stack;

   if (!*str)
   {
      tell_player(p, " Format: find <player(s)> on/off/[blank]\n");
      return;
   }
   if (!check_legal_entry(p, str, 1))
      return;

   while (*str && *str != ' ')
      *stack++ = *str++;
   if (*str)
      str++;
   strcpy(stack, " find");
   stack = strchr(stack, 0);
   *stack++ = 0;
   cmd = stack;
   message_string = cmd;
   if (!strcasecmp("off", str))
   {
      sprintf(stack, " %s hides from your evil eyes.\n", p->name);
      stack = end_string(stack);
      reset_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
   if (!strcasecmp("on", str))
   {
      sprintf(stack, " %s lets you see where %s is, even when %s is hidden.\n",
              p->name, get_gender_string(p), get_gender_string(p));
      stack = end_string(stack);
      set_list(p, oldstack);
      stack = oldstack;
      message_string = 0;
      return;
   }
      sprintf(stack, " (%s lets you see where %s is, even when %s is hidden)\n",
              p->name, get_gender_string(p), get_gender_string(p));
   stack = end_string(stack);
   toggle_list(p, oldstack);
   stack = oldstack;
   message_string = 0;
}

void           toggle_color(player * p, char *str)
{
   if (!p->term) {
	tell_player(p, " You must have a term type to use color.\n");
	return;
	}
   if (!strcasecmp("off", str))
      p->misc_flags |= NOCOLOR;
   else if (!strcasecmp("on", str))
      p->misc_flags &= ~NOCOLOR;
   else
      p->misc_flags ^= NOCOLOR;

   if (p->misc_flags & NOCOLOR)
      tell_player(p, " You disable all loud and annoying colors.\n");
   else
      tell_player(p, " You start viewing the world in millions..er.. ^Y15^N colors.\n");

   save_player(p);
}

void           toggle_system_color(player * p, char *str)
{
   if (!p->term) {
	tell_player(p, " You must have a term type to use color.\n");
	return;
	}
   if (!strcasecmp("off", str))
      p->misc_flags &= ~SYSTEM_COLOR;
   else if (!strcasecmp("on", str))
      p->misc_flags |= SYSTEM_COLOR;
   else
      p->misc_flags ^= SYSTEM_COLOR;

   if (p->misc_flags & SYSTEM_COLOR) {
      tell_player(p, " You have activated ^Btechni^Rcolor^N mode.\n");
	}
   else
      tell_player(p, " You restore the standard ^ablack^N and ^Hwhite^N summink \"colors\".\n");

   save_player(p);
}

void	examine_current_colors(player * p, char *str) {

	char *oldstack;

   if (!p->term) {
	tell_player(p, " You must have a term type to use color.\n");
	return;
	}
	oldstack = stack;

	strcpy(stack, " ------ Your Color Set ------ \n");
	stack = strchr(stack, 0);
	sprintf(stack, " ^%cTells^N, ^%cFriend tells^N, ^%cRoom^N, ^%cShouts^N,", 
		p->colorset[TELsc], p->colorset[FRTsc], p->colorset[ROMsc], p->colorset[SHOsc]);
	stack = strchr(stack, 0);
	sprintf(stack, " ^%cSystem^N, ^%cOdd^N/^%cEven^N", 
		p->colorset[SYSsc], p->colorset[UCOsc], p->colorset[UCEsc]);
	stack = strchr(stack, 0);
	if (p->residency & PSU) {
		sprintf(stack, "/^%cSU^N", p->colorset[SUCsc]);
		stack = strchr(stack, 0);
	} if (p->residency & (TESTCHAR|LOWER_ADMIN|ADMIN)) {
		sprintf(stack, "/^%cAdmin^N", p->colorset[ADCsc]);
		stack = strchr(stack, 0);
	} strcpy(stack, " Channels.\n -----------------------------\n");
	stack = end_string(stack);
	
	tell_player(p, oldstack);
	stack = oldstack;
}

void 	customize_colors(player * p, char *str) {

	char *oldstack, *str2, col;
	int	ici;

   if (!p->term) {
	tell_player(p, " You must have a term type to use color.\n");
	return;
	}
	if (!(p->misc_flags & SYSTEM_COLOR)) {
		tell_player(p, " You can't modify colors unless they're active.\n");
		return;
		}
	if (!*str) {
		examine_current_colors(p, str);
		return;
		}
	oldstack = stack;

	str2 = next_space(str);
	if (*str2){
		*str2++ = 0;
         while (*str2 && *str2 == '^')
  		str2++;
         }


	if (!*str2) {
		tell_player(p, " Format: colorize <section> <Color character>\n");
		if (p->residency & (TESTCHAR|LOWER_ADMIN|ADMIN)) {
			if (p->residency & PSU)
			tell_player(p, " Sections: tell, room, shout, oddchan, evenchan, friend, system, su, admin.\n");
			else
			tell_player(p, " Sections: tell, room, shout, oddchan, evenchan, friend, system, admin.\n");
		} else if (p->residency & PSU)
			tell_player(p, " Sections: tell, room, shout, oddchan, evenchan, friend, system, su.\n");
		else
			tell_player(p, " Sections: tell, room, shout, oddchan, evenchan, friend, system.\n");
		return;
		}

 	col = *str2;

	if (strstr(str, "roo")) {
		p->colorset[ROMsc] = col;
		sprintf(stack, " ^%cRoom color changed to ^^%c^N\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else if(strstr(str, "tel")) {
		p->colorset[TELsc] = col;
		sprintf(stack, " ^%cTell color changed to ^^%c^N\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else if (strstr(str, "sho")) {
		p->colorset[SHOsc] = col;
		sprintf(stack, " ^%cShout color changed to ^^%c^N\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else if (strstr(str, "odd")) {
		p->colorset[UCOsc] = col;
		sprintf(stack, " ^%cOdd numbered channel color changed to ^^%c^N\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else if (strstr(str, "eve")) {
		p->colorset[UCEsc] = col;
		sprintf(stack, " ^%cEven numbered channel color changed to ^^%c^N\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else if (strstr(str, "fri")) {
		p->colorset[FRTsc] = col;
		sprintf(stack, " ^%cFriend tell color changed to ^^%c^N\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else if (strstr(str, "sys")) {
		p->colorset[SYSsc] = col;
		sprintf(stack, " ^%cSystem messages color changed to ^^%c\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else if (p->residency & PSU && !strcasecmp(str, "su")) {
		p->colorset[SUCsc] = col;
		sprintf(stack, " ^%cSU channel color changed to ^^%c^N\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else if (p->residency & (TESTCHAR|LOWER_ADMIN|ADMIN) && !strcasecmp(str, "admin")) {
		p->colorset[ADCsc] = col;
		sprintf(stack, " ^%cAdmin channel color changed to ^^%c^N\n", col, col);
		stack = end_string(stack);
		tell_player(p, oldstack);
		stack = oldstack;
	}
	else {
		tell_player(p, " Format: colorize <section> <Color character>\n");
		if (p->residency & (TESTCHAR|LOWER_ADMIN|ADMIN)) {
			if (p->residency & PSU)
			tell_player(p, " Sections: tell, room, shout, oddchan, evenchan, friend, system, su, admin.\n");
			else
			tell_player(p, " Sections: tell, room, shout, oddchan, evenchan, friend, system, admin.\n");
		} else if (p->residency & PSU)
			tell_player(p, " Sections: tell, room, shout, oddchan, evenchan, friend, system, su.\n");
		else
			tell_player(p, " Sections: tell, room, shout, oddchan, evenchan, friend, system.\n");
		stack = oldstack;
	}

}

void            block_room_descriptions(player * p, char *str)
{
   if (!strcasecmp("off", str))
      p->tag_flags &= ~BLOCK_ROOM_DESC;
   else if (!strcasecmp("on", str))
      p->tag_flags |= BLOCK_ROOM_DESC;
   else
      p->tag_flags ^= BLOCK_ROOM_DESC;

   if (p->tag_flags & BLOCK_ROOM_DESC)
      tell_player(p, " You are blocking room descriptions...\n");
   else
      tell_player(p, " You are no longer blocking room descriptions.\n");
}

void            block_blinks(player * p, char *str)
{
   p->misc_flags ^= STOP_BAD_COLORS;

   if (p->misc_flags & STOP_BAD_COLORS)
      tell_player(p, " You are blocking blinking and inverse text\n");
   else
      tell_player(p, " You are no longer blocking ^Kblink^N and ^Iinverse^N.\n");
}



