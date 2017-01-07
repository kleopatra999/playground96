/*
 * admin.c
 */

#include <string.h>
#include <memory.h>
#include <time.h>
#include <malloc.h>

#include "config.h"
#include "player.h"
#include "admin.h"

/* is this person a hard coded admin? */
int ishcadmin(char *name) {

	int i;
	for (i=0; i<NUM_ADMINS; i++)
		if (!strcasecmp(name, HCAdminList[i]))
			return 1;

	return 0;
}

/* net stats */

void netstat(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"netstat(%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   TELLPLAYER(p, "Total bytes:\t\t(I) %d\t(O) %d\n"
                  "Average bytes:\t\t(I) %d\t\t(O) %d\n"
                  "Bytes per second:\t(I) %d\t\t(O) %d\n"
                  "Total packets:\t\t(I) %d\t(O) %d\n"
                  "Average packets:\t(I) %d\t\t(O) %d\n"
                  "Packets per second:\t(I) %d\t\t(O) %d\n",
                  in_total, out_total, in_average, out_average, in_bps, out_bps,
                  in_pack_total, out_pack_total, in_pack_average,
                  out_pack_average, in_pps, out_pps);
}

/* crash ! */

void crash(player * p, char *str)
{
   char *flop = 0;
#ifdef TRACK
   sprintf(functionin,"crash (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   LOGF("shutdown","Crash used by %s",p->name);

   *flop = -1;
}


/* reload everything */

void reload(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"reload (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   tell_player(p, " Loading help\n");
   init_help();
   tell_player(p, " Loading messages\n");
   if (newban_msg.where)
      FREE(newban_msg.where);
   if (nonewbies_msg.where)
      FREE(nonewbies_msg.where);
   if (version_msg.where)
      FREE(version_msg.where);
   if (connect_msg.where)
      FREE(connect_msg.where);
   if (connect2_msg.where)
      FREE(connect2_msg.where);
   if (connect3_msg.where)
      FREE(connect3_msg.where);
   if (motd_msg.where)
      FREE(motd_msg.where);
   if (spodlist_msg.where)
	FREE(spodlist_msg.where);
   if (banned_msg.where)
      FREE(banned_msg.where);
   if (banish_file.where)
      FREE(banish_file.where);
   if (banish_msg.where)
      FREE(banish_msg.where);
   if (full_msg.where)
      FREE(full_msg.where);
   if (hitells_msg.where)
      FREE(hitells_msg.where);
   if (newbie_msg.where)
      FREE(newbie_msg.where);
   if (newpage1_msg.where)
      FREE(newpage1_msg.where);
   if (newpage2_msg.where)
      FREE(newpage2_msg.where);
   if (disclaimer_msg.where)
      FREE(disclaimer_msg.where);
   if (splat_msg.where)
      FREE(splat_msg.where);
   if (sumotd_msg.where)
      FREE(sumotd_msg.where);
   if (fingerpaint_msg.where)
      FREE(fingerpaint_msg.where);
#ifdef PC
   newban_msg = load_file("files\\newban.msg");
   nonewbies_msg = load_file("files\\nonew.msg");
   connect_msg = load_file("files\\connect.msg");
   motd_msg = load_file("files\\motd.msg");
   banned_msg = load_file("files\\banned.msg");
#else
   newban_msg = load_file("files/newban.msg");
   nonewbies_msg = load_file("files/nonew.msg");
   connect_msg = load_file("files/connect.msg");
   connect2_msg = load_file("files/connect2.msg");
   connect3_msg = load_file("files/connect3.msg");
   motd_msg = load_file("files/motd.msg");
   spodlist_msg = load_file("files/spodlist.msg");
   banned_msg = load_file("files/banned.msg");
#endif
   banish_file = load_file("files/banish");
   banish_msg = load_file("files/banish.msg");
   full_msg = load_file("files/full.msg");
   newbie_msg = load_file("files/newbie.msg");
   newpage1_msg = load_file("files/newpage1.msg");
   newpage2_msg = load_file("files/newpage2.msg");
   disclaimer_msg = load_file("files/disclaimer.msg");
   splat_msg = load_file("files/splat.msg");
   sumotd_msg = load_file("files/sumotd.msg");
   fingerpaint_msg = load_file("files/color_test.msg");
   version_msg = load_file("files/version.msg");
   hitells_msg = load_file("files/hitells.msg");
   tell_player(p, " Done\n");
}


/* edit the banish file from the program */

void quit_banish_edit(player * p)
{
#ifdef TRACK
   sprintf(functionin,"quit_banish_edit (%s)",p->name);
   addfunction(functionin);
#endif

   tell_player(p, " Leaving without changes.\n");
}

void end_banish_edit(player * p)
{
#ifdef TRACK
   sprintf(functionin,"end_banish_edit (%s)",p->name);
   addfunction(functionin);
#endif

   if (banish_file.where)
      FREE(banish_file.where);
   banish_file.length = p->edit_info->size;
   banish_file.where = (char *) MALLOC(banish_file.length);
   memcpy(banish_file.where, p->edit_info->buffer, banish_file.length);
   tell_player(p, " Banish file temp changed.\n");
}

void            banish_edit(player * p, char *str)
{
   start_edit(p, 10000, end_banish_edit, quit_banish_edit, banish_file.where);
}

/* the eject command , muhahahahaa */

void sneeze(player * p, char *str)
{
   time_t t;
   int nologin = 0;
   char *oldstack, *text, *num;
   player*e;

#ifdef TRACK
   sprintf(functionin,"sneeze (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   CHECK_DUTY(p);
   FORMAT(str, " Format: sneeze <person> <time>\n");

   t = time(0);
   if (num = strrchr(str, ' '))
      nologin = atoi(num) * 60;
   if (nologin > (60 * 10) && !(p->residency & ADMIN))
   {
      tell_player(p, " Thats too strict.. setting to 10 minutes.\n");
      nologin = (60 * 10);
   }
   if (!nologin)
      nologin = 300;
   else
      *num = 0;
   while (*str)
   {
      while (*str && *str != ',')
         *stack++ = *str++;
      if (*str)
         str++;
      *stack++ = 0;
      if (*oldstack)
      {
         e = find_player_global(oldstack);
         if (e)
         {
            text = stack;
            if (!check_privs(p->residency, e->residency))
            {
               tell_player(p, " No way pal !!!\n");
               sprintf(stack, " -=*> %s TRIED to sneeze you !!\n", p->name);
               stack = end_string(stack);
               tell_player(e, text);
               stack = text;
               sprintf(stack, "%s failed to sneeze all over %s", p->name, e->name);
               stack = end_string(stack);
               log("sneeze", text);
               stack = text;
            } else
            {
               strcpy(stack, YOU_BEEN_SNEEZED);
               stack = end_string(stack);
               tell_player(e, text);
               e->sneezed = t + nologin;
	       e->eject_count++;
	       p->num_ejected++;
               stack = text;
               quit(e, 0);
               sprintf(stack, SNEEZED_ROOM, e->name, isare(e));
               stack = end_string(stack);
               tell_room(e->location, text);
               stack = text;
	       sprintf(stack, " -=*> %s sneezes on %s for %d mins.\n"
			      " -=*> %s was from %s\n", 
			p->name, e->name, nologin/60, e->name, e->inet_addr);
               stack = end_string(stack);
               su_wall(text);
               stack = text;
               sprintf(text, " %s sneezed on %s for %d [%s]", 
			p->name, e->name, nologin/60, e->inet_addr);
               stack = end_string(text);
               log("sneeze", text);
               stack = text;
               sync_to_file(*(e->lower_name), 0);
            }
         }
      }
      stack = oldstack;
   }
}


/*
 * reset person (in case the su over does it (which wouldn't be like an su at
 * all.. nope no no))
 */

void reset_sneeze(player * p, char *str)
{
   char *oldstack, *newtime;
   time_t t, nologin;
   player dummy;

#ifdef TRACK
   sprintf(functionin,"reset_sneeze (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   CHECK_DUTY(p);
   FORMAT(str, " Format: reset_sneeze <player> <new time>\n");

   newtime = next_space(str);
   if (*newtime)
   {
      t = time(0);
      *newtime++ = 0;
      nologin = atoi(newtime) * 60;
      if (nologin > (60 * 10) && !( p->residency & ADMIN))
      {
         tell_player(p, " Thats too strict.. setting to 10 minutes.\n");
         nologin = 60 * 10;
      }
      nologin += t;
   } else
   {
      nologin = 0;
   }
   memset(&dummy, 0, sizeof(player));
   strcpy(dummy.lower_name, str);
   lower_case(dummy.lower_name);
   dummy.fd = p->fd;
   if (!load_player(&dummy))
   {
      tell_player(p, WHO_IS_THAT);
      return;
   }
   switch (dummy.residency)
   {
      case SYSTEM_ROOM:
         tell_player(p, " That's a system room.\n");
         return;
      default:
         if (dummy.residency & BANISHD)
         {
            if (dummy.residency == BANISHD)
               tell_player(p, " That Name is banished.\n");
            else
               tell_player(p, " That Player is banished.\n");
            return;
         }
         break;
   }
   dummy.sneezed = nologin;
   dummy.location = (room *) - 1;
   save_player(&dummy);
   stack = oldstack;

   /* tell the SUs, too */
   if (!nologin)
     sprintf(stack, " -=*> %s reset%s the sneeze time on %s...\n", 
	p->name,single_s(p), dummy.name);
   else
     sprintf(stack, " -=*> %s change%s the the sneeze time on %s to %d more seconds.\n", 
	p->name, single_s(p), dummy.name, nologin - t);
   stack = end_string(stack);
   su_wall(oldstack);
   LOGF("sneeze", "%s reset the sneeze on %s to %d", p->name, 
	dummy.name, nologin - t);
   stack = oldstack;
}


/* SPLAT!!!! Wibble plink, if I do say so myself */

void soft_splat(player * p, char *str)
{
   char *oldstack, *reason;
   player *dummy;
   int no1, no2, no3, no4;

#ifdef TRACK
   sprintf(functionin,"soft_splat (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   
   if (!(reason = strchr(str, ' ')))
   {
      tell_player(p, " Format: splat <person> <reason>\n");
      return;
   }

   CHECK_DUTY(p);
   FORMAT(str, " Format: splat <person> <reason>\n");
   *reason++ = 0;
   dummy = find_player_global(str);
   if (!dummy)
      return;
   sprintf(stack, "%s SPLAT: %s", str, reason);
   stack = end_string(stack);
   soft_eject(p, oldstack);
   *reason = ' ';
   stack = oldstack;
   if (!(dummy->flags & CHUCKOUT))
      return;
   soft_timeout = time(0) + (5 * 60);
   sscanf(dummy->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);
   soft_splat1 = no1;
   soft_splat2 = no2;
   sprintf(stack, " -=*> Site %d.%d.*.* banned to newbies for 5 mins.\n", no1, no2);
   stack = end_string(stack);
   su_wall(oldstack);
   stack = oldstack;
}


void splat_player(player * p, char *str)
{
   time_t t;
   char *oldstack, *space;
   player *dummy;
   int no1, no2, no3, no4, tme = 0;

   tme=0;

   oldstack = stack;
   if (!(p->residency & (SU | ADMIN)))
   {
      soft_splat(p, str);
      return;
   }
   FORMAT(str, " Format: splat <person> <time>\n");
   CHECK_DUTY(p);
   if (space = strchr(str, ' '))
   {
       *space++ = 0;
       tme = atoi(space);
   }
   if (((p->residency & SU && !(p->residency & ADMIN)) && (tme < 0 || tme > 10)) ||
       (p->residency & ADMIN && (tme < 0)))
     {
       tell_player(p, " Thats too strict.. setting to 10 minutes.\n");
       tme = 10;
     }
   else
     {
       /* when no time specified */
       if (!tme)
	 {
	   tell_player(p, "Time set to 5 minutes.\n");
	   tme = 5;
	 }
     }

   dummy = find_player_global(str);
   if (!dummy)
       return;
   sneeze(p, dummy->lower_name);
   if (!(dummy->flags & CHUCKOUT))
       return;
   t = time(0);
   splat_timeout = t + (tme * 60);
   sscanf(dummy->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);
   splat1 = no1;
   splat2 = no2;
   sprintf(stack, " -=*> %d.%d.*.* banned for %d minutes cause of %s\n", 
		no1, no2, tme, dummy->name);
   stack = end_string(stack);
   su_wall(oldstack);
   stack = oldstack;
}

void unsplat(player * p, char *str)
{
   char *oldstack, *spc;
   time_t t;
   int number = -1;

#ifdef TRACK
   sprintf(functionin,"unsplat (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   oldstack = stack;
   t = time(0);
   if (*str)
   {
      spc = strchr(str, ' ');
      if (spc)
      {
         *spc++;
         number = atoi(spc);
      } else
      {
         number = 0;
      }
   }
   if (!*str || number < 0)
   {
      number = splat_timeout - (int) t;
      if (number <= 0)
      {
         tell_player(p, " No site banned atm.\n");
         return;
      }
      sprintf(stack, " Site %d.%d.*.* is banned for %d more seconds.\n",
              splat1, splat2, number);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   if (splat1 == 0 && splat2 == 0)
   {
      tell_player(p, " No site banned atm.\n");
      return;
   }
   if (number == 0)
   {
      sprintf(stack, " -=*> %s has unbanned site %d.%d.*.*\n", 
	p->name, splat1, splat2);
      splat_timeout = (int) t;
      stack = end_string(stack);
      su_wall(oldstack);
      stack = oldstack;
      return;
   }
   if (number > 600)
   {
      tell_player(p, " Thats too strict.. setting to 10 minutes.\n");
      number = 600;
   }
   sprintf(stack, " -=*> %s changes the ban on site %d.%d.*.* to a further %d seconds.\n", 
	p->name, splat1, splat2, number);
   splat_timeout = (int) t + number;
   stack = end_string(stack);
   su_wall(oldstack);
   stack = oldstack;
}


/* the eject command (too) , muhahahahaa */

void soft_eject(player * p, char *str)
{
   char *oldstack, *text, *reason;
   player *e;

#ifdef TRACK
   sprintf(functionin,"tell_player (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   reason = next_space(str);
   if (*reason)
      *reason++ = 0;
   FORMAT(reason, " Format: drag <person> <reason>\n");

   CHECK_DUTY(p);

   e = find_player_global(str);
   if (e)
   {
      text = stack;
      if (!check_privs(p->residency, e->residency))
      {
         tell_player(p, " Sorry, you can't...\n");
	 TELLPLAYER(e, " -=*> %s TRIED to drag you away!!\n", p->name);
         LOGF("drag", " -=*> %s TRIED to drag %s away", p->name, e->name);
         stack = text;
      } else
      {
         tell_player(e, YOU_BEEN_DRAGGED);
         stack = text;
	 e->eject_count++;
	 p->num_ejected++;
         quit(e, 0);
         sprintf(stack, DRAG_ROOM, e->name);
         stack = end_string(stack);
         tell_room(e->location, text);
         stack = text;
	 sprintf(stack, " -=*> %s gets rid of %s because %s.\n"
		" -=*> %s was from %s\n", 
		p->name, e->name, reason, e->name, e->inet_addr);
         stack = end_string(stack);
         su_wall(text);
         stack = text;
         LOGF("drag", " %s dragged %s because: %s", p->name, e->name, reason);
      }
   }
   stack = oldstack;
}

/* Privs Checking - compare 2 privs levels */

int  check_privs(int p1, int p2)
{
   p1 &= ~NONSU;
   p2 &= ~NONSU;
   if (p1 & HCADMIN)
      return 1;            /* else if (p2 & PROTECT) return 0; */
   else if (p1 > p2)
      return 1;
   else 
      return 0;
}

/* Blankpass, done _right_ */

void new_blankpass(player *p, char *str)
{
   char *oldstack;
   char *pass,*size;
   player *p2, dummy;
   saved_player *sp; 
   char bpassd[MAX_NAME] = "";
   
   CHECK_DUTY(p); 
   FORMAT(str, " Format: blankpass <person> [new password]\n");
     {
   
   oldstack = stack;
   pass = 0;
   pass = strchr(str, ' ');
   if (pass)
   {  
      *pass++ = 0;
      if (strlen(pass) > (MAX_PASSWORD - 2) || strlen(pass) < 3)
      {  
         tell_player(p, " Try a reasonable length password.\n");
         return;
      }
   }
   lower_case(str);
   p2 = find_player_absolute_quiet(str);

   /* Hell, if their not on the program you SHOULD know */

   if (!p2)
      tell_player(p, NOT_HERE_ATM);
   
   if (p2)  
   {
      if (!check_privs(p->residency, p2->residency))
      {
         tell_player(p, " You can't blankpass THAT person!\n");
         sprintf(stack, " -=*> %s TRIED to blankpass %s!\n", p->name, p2->name);
         stack = end_string(stack);
         su_wall_but(p, oldstack);
         stack = oldstack;
               sprintf(stack, "%s failed to blankpass %s (Nuke the ass for trying *grin*)", p->name, p2->name);
               stack = end_string(stack);
               log("blanks", oldstack);
               stack = oldstack;  
               return;
            }      
       
      if (!pass)
      {
         sprintf(stack, " -=*> %s has just blanked your password.\n", p->name);
         stack = end_string(stack);
         tell_player(p2, oldstack);
         stack = oldstack;
         p2->password[0] = 0;
         tell_player(p, "Password blanked.\n");
         sprintf(stack, "%s blanked %s's password (logged in)", p->name, p2->name);
         stack = end_string(stack);
         log("blanks", oldstack);
         stack = oldstack;
    } else
      {
         sprintf(stack, " -=*> %s has just changed your password.\n", 
p->name);
         stack = end_string(stack);
         tell_player(p2, oldstack);
         stack = oldstack;
         strcpy(p2->password, do_crypt(pass, p2));
         tell_player(p, " Password changed. They have NOT been informed of"
                        " what it is.\n");
         sprintf(stack, "%s changed %s's password (logged in)", p->name,
                 p2->name);  
         stack = end_string(stack);
         log("blanks", oldstack);
         stack = oldstack; 
      }
      set_update(*str);
      return;
    } 
   else
     strcpy(bpassd, str);

   lower_case(bpassd);

   /* This is the setup for the saved priv check */

   sp = find_saved_player(bpassd);
   if (!sp)
   {
      sprintf(stack, " Couldn't find saved player '%s'.\n", str);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }

   /* This is what needed to be added (thanks to Mantis of Resort) */

   if (!check_privs(p->residency, sp->residency))
   {
      tell_player(p, " You can't blankpass that save file !\n");
      stack = oldstack;
      return;
   }     
     
     {
       strcpy(dummy.lower_name, str);
       dummy.fd = p->fd;
       if (load_player(&dummy))
       {
           if (dummy.residency & BANISHD)
           {
               tell_player(p, " By the way, this player is currently BANISHD.");
               if (dummy.residency == BANISHD)
               {
                   tell_player(p, " (Name Only)\n");
               } else
               {
                   tell_player(p, "\n");
               }
           }
          if (pass)
           {
               strcpy(dummy.password, do_crypt(pass, &dummy));
               tell_player(p, " Password changed in saved files.\n");
               sprintf(stack, "%s changed %s's password (logged out)", p->name, dummy.name);
               stack = end_string(stack);
               log("blanks", oldstack);
               stack = oldstack;
           } else
           {
               dummy.password[0] = 0;
               tell_player(p, " Password blanked in saved files.\n");
               sprintf(stack, "%s changed %s's password (logged out)", p->name, dummy.name);               
               stack = end_string(stack);
               log("blanks", oldstack);
               stack = oldstack;
           }
           dummy.script = 0;
           dummy.script_file[0] = 0;
           dummy.flags &= ~SCRIPTING;
           dummy.location = (room *) -1;
           save_player(&dummy);
       } else
         tell_player(p, " Can't find that player in saved files.\n");
    }
  }
}

void rm_shout_saved(player *p, char *str, int for_time)
{
   player *p2, dummy;

#ifdef TRACK
   sprintf(functionin,"unjail (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (for_time != 0 && for_time != -1) {
	tell_player(p, " Can only rm_shout a saved player forever, or clear one.\n");
	return;
	}

      tell_player(p, " Checking saved files... ");
      strcpy(dummy.lower_name, str);
      lower_case(dummy.lower_name);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
      {
         tell_player(p, " Not found.\n");
         return;
      } else
      {
         tell_player(p, "\n");
         p2 = &dummy;
         p2->location = (room *) -1;
      }


   if (!(p2->system_flags & SAVENOSHOUT) && for_time == 0)
      {
         tell_player(p, " That player is not rm_shoutted.\n");
         return;
      }
   else if (p2->system_flags & SAVENOSHOUT && for_time == -1)
      {
         tell_player(p, " That player is already rm_shoutted forever.\n");
         return;
      }
   
   p2->system_flags ^= SAVENOSHOUT;

   if (for_time == 0) { 
     SUWALL(" -=*> %s regrants shouts to %s.\n", p->name, p2->name);
     LOGF("rm_shout", "%s regranted shouts to %s", p->name, p2->name);
   } else {
     SUWALL(" -=*> %s removes shouts from %s -- forever!!\n", p->name, p2->name);
     LOGF("rm_shout", "%s removed shouts from %s for -1", p->name, p2->name);
     LOGF("forever", "%s removed shouts from %s", p->name, p2->name);
     }
   save_player(&dummy);
}


void rm_sing_saved(player *p, char *str, int for_time)
{
   player *p2, dummy;

#ifdef TRACK
   sprintf(functionin,"unjail (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   if (for_time != 0 && for_time != -1) {
	tell_player(p, " Can only rm_sing a saved player forever, or clear one.\n");
	return;
	}

      tell_player(p, " Checking saved files... ");
      strcpy(dummy.lower_name, str);
      lower_case(dummy.lower_name);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
      {
         tell_player(p, " Not found.\n");
         return;
      } else
      {
         tell_player(p, "\n");
         p2 = &dummy;
         p2->location = (room *) -1;
      }


   if (!(p2->system_flags & SAVE_NO_SING) && for_time == 0)
      {
         tell_player(p, " That player is not rm_singed.\n");
         return;
      }
   else if (p2->system_flags & SAVE_NO_SING && for_time == -1)
      {
         tell_player(p, " That player is already rm_singed forever.\n");
         return;
      }
   
   p2->system_flags ^= SAVE_NO_SING;

   if (for_time == 0) { 
     SUWALL(" -=*> %s regrants singing to %s.\n", p->name, p2->name);
     LOGF("rm_shout", "%s regranted singing to %s", p->name, p2->name);
   } else {
     SUWALL(" -=*> %s removes singing from %s -- forever!!\n", p->name, p2->name);
     LOGF("rm_sing", "%s removed singing from %s for -1", p->name, p2->name);
     LOGF("forever", "%s removed singing from %s", p->name, p2->name);
   } 
   save_player(&dummy);
}


/* remove shout from someone for a period */

void remove_shout(player * p, char *str)
{
   char *size = 0;
   int new_size = 5;
   player *p2;
   
#ifdef TRACK
   sprintf(functionin,"remove_shout (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   FORMAT(str, " Format: rm_shout <player> <how long> (-1 for eternal, 0 for restore)\n");
   CHECK_DUTY(p);

   size = strchr(str, ' ');
   if (size)
   {
      *size++ = 0;
      new_size = atoi(size);
   }
   p2 = find_player_global(str);
   if (!p2) {
      if (size)
	rm_shout_saved(p, str, new_size);
      return;
   }
   if (!check_privs(p->residency, p2->residency))
   {
      tell_player(p, " You can't do that !!\n");
      TELLPLAYER(p2, " -=*> %s tried to remove shout from you.\n", p->name);
      return;
   }
   p2->system_flags &= ~SAVENOSHOUT;
   if (new_size) {
      tell_player(p2, YOU_BEEN_RMSHOUT);
      p->num_rmd++;
      }
   else
      tell_player(p2, YOU_BEEN_UNRMD);
   if (new_size > 30)
     if (!(p->residency & ADMIN))
       new_size = 5;
   switch (new_size)
   {
      case -1:
         SUWALL(" -=*> %s removes shouts from %s forever!\n", 
		p->name, p2->name);
         p2->system_flags |= SAVENOSHOUT;
         p2->no_shout = -1;
         break;
      case 0:
         SUWALL(" -=*> %s restores shouts to %s.\n", p->name, p2->name);
         break;
      case 1:
         SUWALL(" -=*> %s just remove shouted %s for 1 minute.\n",
                 p->name, p2->name);
         break;
      default:
         SUWALL(" -=*> %s just remove shouted %s for %d minutes.\n",
                 p->name, p2->name, new_size);
         break;
   }
   new_size *= 60;
   if (new_size >= 0)
      p2->no_shout = new_size;

   if (new_size != 0)
     LOGF("rm_shout", "%s removed %s's shout for %d.",p->name,p2->name,
	     new_size);
   else
     LOGF("rm_shout", "%s regranted shouts to %s.",p->name,p2->name);
   if (new_size < 0)
	/* log in the "forever" log */
	LOGF("forever", "%s removes %s shout forever.", p->name, p2->name);

}

/* cut-and-paste of rm_shout -- someone shoot me, I'm a spooner --traP */
void remove_sing(player * p, char *str)
{
   char *size = 0;
   int new_size = 5;
   player *p2;
   
#ifdef TRACK
   sprintf(functionin,"remove_sing (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   FORMAT(str, " Format: rm_sing <player> [how long]\n");
   CHECK_DUTY(p);

   size = strchr(str, ' ');
   if (size)
   {
      *size++ = 0;
      new_size = atoi(size);
   }
   p2 = find_player_global(str);
   if (!p2) {
      if (size)
	rm_sing_saved(p, str, new_size);
      return;
   }
   if (!check_privs(p->residency, p2->residency))
   {
      tell_player(p, " You can't do that !!\n");
      TELLPLAYER(p2, " -=*> %s tried to remove sing from you.\n", p->name);
      return;
   }
   p2->system_flags &= ~SAVE_NO_SING;
   if (new_size) {
      tell_player(p2, YOU_BEEN_RMSING);
      p->num_rmd++;
      }
   else
      tell_player(p2, YOU_BEEN_UNRSING);
   if (new_size > 30)
     if (!(p->residency & ADMIN))
       new_size = 5;
   switch (new_size)
   {
      case -1:
	 SUWALL(" -=*> %s just remove singed %s. (permanently!)\n",
		 p->name, p2->name);
	 p2->system_flags |= SAVE_NO_SING;
	 p2->no_sing = -1;
	 break;
      case 0:
	 SUWALL(" -=*> %s just allowed %s to sing again.\n", p->name,
		 p2->name);
	 break;
      case 1:
	 SUWALL(" -=*> %s just remove singed %s for 1 minute.\n",
		 p->name, p2->name);;
	 break;
      default:
	 SUWALL(" -=*> %s just remove singed %s for %d minutes.\n",
		 p->name, p2->name, new_size);
	 break;
   }
   new_size *= 60;
   if (new_size >= 0)
      p2->no_sing = new_size;

   if (new_size < 0)
    LOGF("rm_sing", "%s removed %s's sing for %d.",p->name,p2->name,
	     new_size);
   else if (new_size == 0) 
    LOGF("rm_sing", "%s regranted sings to %s.",p->name,p2->name);
   else {
    LOGF("rm_sing", "%s gave %s a permenant remove sing...",p->name,p2->name);
    LOGF("forever", "%s gave %s a permenant remove sing...",p->name,p2->name);
   }
}


/* remove trans movement from someone for a period */

void remove_move(player * p, char *str)
{
   char *size;
   int new_size = 5;
   player         *p2;

#ifdef TRACK
   sprintf(functionin,"remove_move (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   FORMAT(str, " Format: rm_move <player> [<for how long>]\n");
   CHECK_DUTY(p);

   size = strchr(str, ' ');
   if (size)
   {
      *size++ = 0;
      new_size = atoi(size);
   } else
      new_size = 1;
   p2 = find_player_global(str);
   if (!p2)
      return;
   if (!check_privs(p->residency, p2->residency))
   {
      tell_player(p, " You can't do that !!\n");
      TELLPLAYER(p2, " -=*> %s tried to remove move from you.\n", p->name);
      return;
   }
   p2->system_flags &= ~SAVED_RM_MOVE;
   if (new_size) {
      tell_player(p2, " -=*> You step on some chewing-gum, and you suddenly "
                      "find it very hard to move ...\n");
      p->num_rmd++;
      }
   else
      tell_player(p2, " -=*> Someone hands you a new pair of hi-tops ...\n");
   if (new_size > 30)
      new_size = 5;
   new_size *= 60;
   if (new_size >= 0)
      p2->no_move = new_size;
   else {
	p2->system_flags |= SAVED_RM_MOVE;
	p2->no_move = 100; /* temp dummy value */
	}
   if ((new_size/60) == 1)
      SUWALL(" -=*> %s remove moves %s for 1 minute.\n", p->name,
              p2->name);
   else if (new_size == 0) {
      SUWALL(" -=*> %s allows %s to move again.\n", p->name, p2->name);
	LOGF("rm_move", "%s lets %s move again", p->name, p2->name);
   }
   else if (new_size <0 )
      SUWALL(" -=*> %s remove moves %s. Permanently!\n", p->name,
              p2->name);
   else
      SUWALL(" -=*> %s remove moves %s for %d minutes.\n", p->name,
              p2->name, new_size/60);

   if (new_size != 0)
	LOGF("rm_move", "%s rm_moves %s for %d minutes", p->name, 
		p2->name, new_size/60);
}

/* permission changes routines */

/* the resident command */

void resident(player * p, char *str)
{
   player *p2;
   char *oldstack, *scan, *first;
   int ressie = 0;
   int validi = 0;

#ifdef TRACK
   sprintf(functionin,"resident (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   FORMAT(str, " Format: res <whoever>\n");
   CHECK_DUTY(p);
   
   if (!strcasecmp(str, "me"))
      p2 = p;
   else
      p2 = find_player_global(str);
   if (!p2)
   {
      stack = oldstack;
      return;
   }
   if (!strcasecmp(p2->name, "guest"))
   {
      tell_player(p, "\n The name 'Guest' is reserved because people may use "
                     "that when first logging in before using the name they "
                     "REALLY want to use. So get this person to choose another "
                     "name, THEN make them resident.\n\n");
      stack = oldstack;
      return;
   }

   first = first_char (p);
   if (strstr(first, "validate"))
	validi = 1; 

   if ((p2->residency != NON_RESIDENT) && p2 != p)
   {
      if (p2->saved)
      {
         if (p2->saved->last_host)
         {
            if (p2->saved->last_host[0] != 0)
            {
               tell_player(p, " That player is already resident, and has "
                              "re-logged in\n");
               stack = oldstack;
               return;
            }
         }
      }
      ressie = 1;
   }
   if (ressie)
   {
      sprintf(oldstack, "\n\n -=*> You are now a resident.\n");
   } else
   {
     if (p->gender==PLURAL)
       sprintf(oldstack, "\n\n -=*> %s have made you a resident.\n", p->name);
     else
       sprintf(oldstack, "\n\n -=*> %s has made you a resident.\n", p->name);
   }
   stack = strchr(oldstack, 0);


   sprintf(stack, WELCOME_TO_PG);
   stack = end_string(stack);
   tell_player(p2, oldstack);
   if (ressie)
   {
      stack = oldstack;
      sprintf(stack, " You repeat the message about setting email and "
                     "password to %s\n", p2->name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   if (p2 != p)
   {
      p2->residency |= get_flag(permission_list, "residency");
      p2->residency |= NO_SYNC;
      p2->email[0] = 2;
      p2->email[1] = 0;
      p2->flags &= ~SCRIPTING;
      p2->script = 0;
      p2->script_file[0] = 0;
      strcpy(p2->script_file, "dummy");
      strcpy(p2->ressied_by, p->name);
      p->num_ressied++;
      tell_player(p, " Residency granted ...\n");
      stack = oldstack;

      if (validi)
	sprintf(oldstack, " -=*> %s grants residency (validated) to %s\n", p->name,
		p2->name);
      else
	sprintf(oldstack, " -=*> %s grants residency to %s\n", p->name,
		p2->name);
      stack = end_string(oldstack);
      su_wall(oldstack);
      stack = oldstack;
      p2->saved_residency = p2->residency;
      p2->saved = 0;
      if (validi)
          sprintf(stack, "%s made %s a resident (validated) [%s]", p->name, p2->name, p2->inet_addr);
      else
          sprintf(stack, "%s made %s a resident [%s]", p->name, p2->name, p2->inet_addr);
      stack = end_string(stack);
      log("resident", oldstack);
      if (validi) {
   	p2->email[0] = ' ';
   	p2->email[1] = 0;
	newsetpw0(p2, 0);
	}
      else
        begin_ressie(p2, 0);
   }
   stack = oldstack;
}


/* the grant command */

void grant(player * p, char *str)
{
   char *permission;
   player *p2;
   saved_player *sp;
   int change;
   char *oldstack;
   int count;

#ifdef TRACK
   sprintf(functionin,"grant (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);
   
   oldstack = stack;
   permission = next_space(str);
   if (!*permission)
   {
      tell_player(p, " Format: grant <whoever> <whatever>\n");
      tell_player(p, " Grantable privs are: ");
      for (count=0;permission_list[count].text!=0;count++)
      {
         sprintf(stack, "%s, ", permission_list[count].text);
         stack = strchr(stack, 0);
      }
      while (*stack != ',')
         *stack--;
      *stack++ = '.';
      *stack++ = '\n';
      *stack++ = 0;
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   *permission++ = 0;

   change = get_flag(permission_list, permission);
   if (!change)
   {
      tell_player(p, " Can't find that permission.\n");
      return;
   }
   if (!(p->residency & change) )
   {
     if ( !(p->residency & HCADMIN))
      {
         tell_player(p, " You can't give out permissions you haven't got "
           "yourself.\n");
         return;
      }
   }
   p2 = find_player_global(str);
   if (!p2)
   {
      lower_case(str);
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " Couldn't find player.\n");
         stack = oldstack;
         return;
      }
      if (sp->residency == BANISHD || sp->residency == SYSTEM_ROOM)
      {
         tell_player(p, " That is a banished NAME, or System Room.\n");
         stack = oldstack;
         return;
      }
      if ((change == SYSTEM_ROOM) && !(sp->residency == 0))
      {
         tell_player(p, " You can't grant sysroom to anything but a blank"
                        "playerfile.\n");
         stack = oldstack;
         return;
      }
   if (!check_privs(p->residency, sp->residency))
      {
         tell_player(p, " You can't alter that save file\n");
         sprintf(oldstack, "%s failed to grant %s to %s\n", p->name,
                 permission, str);
         stack = end_string(oldstack);
         log("grant", oldstack);
         stack = oldstack;
         /* why doesn't this work? :P */
         sprintf(stack, " -=*) %s FAILED to grant %s to %s\n",
		p->name, permission, str);
	 stack = end_string(stack);
	 au_wall_but(p, stack);
         stack = oldstack;
         return;
      }
      tell_player(p, " Permission changed in player files.\n");
      stack = oldstack;
      sprintf(stack, "%s granted %s to %s", p->name, permission, 
	      sp->lower_name);
      stack = end_string(stack);
      log("grant",oldstack);
      sp->residency |= change;
      set_update(*str);
      stack = oldstack;
         sprintf(stack, " -=*) %s granted %s to %s\n",
		p->name, permission, sp->lower_name);
	 stack = end_string(stack);
	 au_wall_but(p, stack);
      stack = oldstack;
      return;
   } else
   {
      if (p2->residency == NON_RESIDENT)
      {
         tell_player(p, " That player is non-resident!\n");
         stack = oldstack;
         return;
      }
      if (p2->residency == BANISHD || p2->residency == SYSTEM_ROOM)
      {
         tell_player(p, " That is a banished NAME, or System Room.\n");
         stack = oldstack;
         return;
      }
   if (!check_privs(p->residency, p2->residency))
      {
         tell_player(p, " No Way Pal !!\n");
         sprintf(oldstack, " -=*> %s tried to grant your permissions.\n"
                 ,p->name);
         stack = end_string(oldstack);
         tell_player(p2, oldstack);
         stack = oldstack;
         return;
      }
      sprintf(oldstack, "\n%s has altered your commands.\n", p->name);
      p2->saved_residency |= change;
      p2->residency = p2->saved_residency;
      stack = strchr(stack, 0);
      if (p2->residency & SU)
      {
         strcpy(stack, "Read the appropriate files please ( shelp "
                       "commands and shelp res, and help <command> "
		       "where <command> is listed under \"commands su\")\n\n");
      }
      stack = end_string(oldstack);
      tell_player(p2, oldstack);
      stack = oldstack;
      sprintf(stack, "%s granted %s to %s", p->name, permission, p2->name);
      stack = end_string(oldstack);
      log("grant",oldstack);
      stack = oldstack;
         sprintf(stack, " -=*) %s granted %s to %s\n",
		p->name, permission, p2->name);
	 stack = end_string(stack);
	 au_wall_but(p, oldstack);
      save_player(p2);
      tell_player(p, " Permissions changed ...\n");
   }
   stack = oldstack;
}


/* the remove command */

void remove_privs(player * p, char *str)
{
   char *permission;
   player *p2;
   saved_player *sp;
   int change, count;
   char *oldstack;

#ifdef TRACK
   sprintf(functionin,"remove (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   oldstack = stack;
   permission = next_space(str);
   if (!*permission)
   {
      tell_player(p, " Format: remove <whoever> <whatever>\n");
      tell_player(p, " Remove-able privs are: ");
      for (count=0;permission_list[count].text!=0;count++)
      {
         sprintf(stack, "%s, ", permission_list[count].text);
         stack = strchr(stack, 0);
      }
      while (*stack != ',')
         *stack--;
      *stack++ = '.';
      *stack++ = '\n';
      *stack++ = 0;
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   *permission++ = 0;
   if (!(strcasecmp("everyone", str))
         && !(strcasecmp("everything", permission))
         && (p->residency & (1 << 27)))
   {
      tell_player(p, "\n What are you doing, hal?\n\n");
      su_wall("\n -=*> Some admin is goofing off again...\n\n");
      return;
   }
   change = get_flag(permission_list, permission);
   if (!change)
   {
      tell_player(p, " Can't find that permission.\n");
      return;
   }
   if (!(p->residency & change))
   {
      if ( !(p->residency & HCADMIN) )
      {
         tell_player(p, " You can't remove permissions you haven't got "
                        "yourself.\n");
         return;
      }
   }

   p2 = find_player_global(str);
   if (!p2)
   {
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " Couldn't find player.\n");
         return;
      }
   if (!check_privs(p->residency, sp->residency))
      {
         tell_player(p, " You cant change that save file !!!\n");
         sprintf(oldstack, "%s failed to remove %s from %s", p->name,
                 permission, str);
         stack = end_string(oldstack);
         log("grant", oldstack);
      	 stack = oldstack;
         sprintf(stack, " -=*> %s FAILED to remove %s from %s\n",
		p->name, permission, str);
	 stack = end_string(stack);
	 au_wall_but(p, oldstack);
         stack = oldstack;
         return;
      }
      sp->residency &= ~change;
      if (sp->residency == NON_RESIDENT)
         remove_player_file(sp->lower_name);
      set_update(*str);
      tell_player(p, " Permissions changed in save files.\n");
      stack = oldstack;
      sprintf(oldstack, "%s removes %s from %s", p->name,
           permission, sp->lower_name);
      stack = end_string(oldstack);
      log("grant", oldstack);
      	 stack = oldstack;
         sprintf(stack, " -=*> %s removed %s from %s\n",
		p->name, permission, sp->lower_name);
	 stack = end_string(stack);
	 au_wall_but(p, stack);
      stack=oldstack;
      return;
   } else
   {
   if (!check_privs(p->residency, p2->residency))
      {
         tell_player(p, " No Way Pal !!\n");
         sprintf(oldstack, " -=*> %s tried to remove your permissions. Get 'em!!\n", p->name);
         stack = end_string(oldstack);
         tell_player(p2, oldstack);
         stack = oldstack;
         return;
      }
      p2->residency &= ~change;
      p2->saved_residency = p2->residency;
      sprintf(oldstack, " -=*> %s has altered your commands.\n", p->name);
      stack = end_string(oldstack);
      tell_player(p2, oldstack);
      stack = oldstack;
      sprintf(stack, "%s removed %s from %s", p->name, permission, p2->name);
      stack = end_string(stack);
      log("grant",oldstack);
      	 stack = oldstack;
         sprintf(stack, " -=*> %s removes %s from %s\n",
		p->name, permission, p2->name);
	 stack = end_string(stack);
	 au_wall_but(p, stack);
      if (p2->residency != NON_RESIDENT)
         save_player(p2);
      else
         remove_player_file(p2->lower_name);
      tell_player(p, " Permissions changed ...\n");
   }
   stack = oldstack;
}

/* remove player completely from the player files */

void nuke_player(player * p, char *str)
{
   player *p2, dummy;
   saved_player *sp;
   char nuked[MAX_NAME] = "";
   char nukee[MAX_NAME] = "";
   char naddr[MAX_INET_ADDR] = "";
   int mesg_done = 0;
   int *scan, *scan_count, mcount = 0, sscan;
   note *smail, *snext;

#ifdef TRACK
   sprintf(functionin,"nuke_player (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif


   CHECK_DUTY(p);

   p2 = find_player_absolute_quiet(str);
   if (!p2)
      tell_player(p, "No such person on the program.\n");
   if (p2)
   {
   if (!check_privs(p->residency, p2->residency))
      {
         tell_player(p, " You can't nuke them !\n");
         TELLPLAYER(p2, " -=*> %s tried to nuke you.  Yipes!!.\n", p->name);
         return;
      }
      if (p2->saved)
         all_players_out(p2->saved);
      /* time for a new nuke screen */
      TELLPLAYER(p2, NUKE_SCREEN);
      p->num_ejected++;
      p2->saved = 0;
      p2->residency = 0;
      strcpy(nuked, p2->name);
      strcpy(naddr, p2->inet_addr);
      quit(p2, 0);
      if (p->gender==PLURAL)
	SUWALL(" -=*> %s nuke %s to a crisp, TOASTY!!\n -=*> %s "
		"was from %s\n",p->name, nuked, nuked, naddr);
      else
	SUWALL(" -=*> %s nukes %s to a crisp, TOASTY!!\n -=*> %s "
		"was from %s\n",p->name, nuked, nuked, naddr);
      mesg_done = 1;
   }
   strcpy(nukee, str);
   lower_case(nukee);
   sp = find_saved_player(nukee);
   if (!sp)
   {
      TELLPLAYER(p, " Couldn't find saved player '%s'.\n", str);
      return;
   }
   if (!check_privs(p->residency, sp->residency))
   {
      tell_player(p, " You can't nuke that save file !\n");
      return;
   }
/* TRY to clean up notes */
   strcpy(dummy.lower_name, sp->lower_name);
   dummy.fd = p->fd;
   load_player(&dummy);
   if (!*nuked)
   {
      strcpy(nuked, dummy.name);
      strcpy(naddr, sp->last_host);
   }
   /* No choice but to comment out due to wibbles 
      The bug COULD be that you can't use a command when offline? 
      I have no fucking clue, but its better to leave deletion out,
      (you can mail your friends before you leave) IMHO anyway */
   /* Yeah Mike, its not a bug, its a feature!! 

   scan = dummy.saved->mail_received;
   if (scan)
   {
      for (scan_count = scan; *scan_count; scan_count++)
      {
         mcount++;
      }
      for (;mcount;mcount--)
      {
         delete_received(&dummy, "1");
      }
   }
   mcount = 1;
   sscan = dummy.saved->mail_sent;
   smail = find_note(sscan);
   if (smail)
   {
      while (smail)
      {
         mcount++;
         sscan = smail->next_sent;
         snext = find_note(sscan);
         if (!snext && sscan)
         {
            smail->next_sent = 0;
            smail = 0;
         } else
         {
            smail = snext;
         }
      }
      for(;mcount;mcount--)
      {
         delete_sent(&dummy, "1");
      }
   }
   save_player(&dummy);
 END clean up notes */
   all_players_out(sp);
   remove_player_file(nukee);
   tell_player(p, " Files succesfully nuked.\n");
   if(!mesg_done)
   {
     if (p->gender==PLURAL)
       SUWALL(" -=*> %s nuke \'%s\' to a crisp, TOASTY!!\n",
	       p->name,nuked);  /* sp->lower_name - BLEAH!!  Nogard. */
     else
       SUWALL(" -=*> %s nukes \'%s\' to a crisp, TOASTY!!\n",
	       p->name,nuked);
   }
   LOGF("nuke", "%s nuked %s [%s]", p->name, nuked, naddr);
}

/* you think that supernews.c is a fucking bodge??   check THIS shit */
/* Note: have a good time with this little program.   Its something that Mike
and I had trouble throwing back and forth.   Try not to laugh too hard 
however :^) - chris */
void suicide3 (player * p, char *str)
{
   player *p2, dummy;
   saved_player *sp;
   char nuked[MAX_NAME] = "";
   char nukee[MAX_NAME] = "";
   char naddr[MAX_INET_ADDR] = "";
   int mesg_done = 0;
   int *scan, *scan_count, mcount = 0, sscan;
   note *smail, *snext;

   p2 = find_player_absolute_quiet(str);
   if (p2)
   {
      LOGF("suicide_debug", "%s is suiciding (1)", p2->name);
      if (p2->saved)
         all_players_out(p2->saved);
      p2->saved = 0;
      p2->residency = 0;
      strcpy(nuked, p2->name);
      strcpy(naddr, p2->inet_addr);
      /* quit(p2, 0); */
      LOGF("suicide_debug", "%s is suiciding (2)", nuked);
	SUWALL(" -=*> %s suicides, WHOOPSIE!!\n -=*> %s "
		"was from %s\n",nuked, nuked, naddr);
      mesg_done = 1;
   }
   strcpy(nukee, str);
   lower_case(nukee);
   sp = find_saved_player(nukee);
   strcpy(dummy.lower_name, sp->lower_name);
   dummy.fd = p->fd;
   LOGF("suicide_debug", "%s is suiciding (3)", nuked);
   quit(p, 0);
   load_player(&dummy);
   if (!*nuked)
   {
      LOGF("suicide_debug", "%s is suiciding (4)", nuked);
      strcpy(nuked, dummy.name);
      strcpy(naddr, sp->last_host);
   }
   /* commented out due to wibbles en masse  -- see above
   if (dummy.saved)
    scan = dummy.saved->mail_received;
   else scan = 0;
   LOGF("suicide_debug", "%s is suiciding (5)", nuked);
   if (scan)
   {
      LOGF("suicide_debug", "%s is suiciding (5b)", nuked);
      for (scan_count = scan; *scan_count; scan_count++)
      {
         mcount++;
         LOGF("suicide_debug", "%s is suiciding (5b.x)", nuked);
      }
      LOGF("suicide_debug", "%s is suiciding (5c)", nuked);
      for (;mcount;mcount--)
      {
         delete_received(&dummy, "1");
         LOGF("suicide_debug", "%s is suiciding (5c.x)", nuked);
      }
   }
   LOGF("suicide_debug", "%s is suiciding (6)", nuked);
   mcount = 1;
   if (dummy.saved) sscan = dummy.saved->mail_sent;
   else		    sscan = 0;
   smail = find_note(sscan);
   if (smail)
   {
      LOGF("suicide_debug", "%s is suiciding (6b)", nuked);
      while (smail)
      {
         mcount++;
         sscan = smail->next_sent;
         snext = find_note(sscan);
         if (!snext && sscan)
         {
            smail->next_sent = 0;
            smail = 0;
         } else
         {
            smail = snext;
         }
      }
      LOGF("suicide_debug", "%s is suiciding (7)", nuked);
      for(;mcount;mcount--)
      {
         delete_sent(&dummy, "1");
      }
   }
   LOGF("suicide_debug", "%s is suiciding (8)", nuked);
   save_player(&dummy);
 ain't comments great? */
   LOGF("suicide_debug", "%s is suiciding (9)", nuked);
   all_players_out(sp);
   LOGF("suicide_debug", "%s is suiciding (10)", nuked);
   remove_player_file(nukee);
   /* chris, um.. there IS no P at this time... */
   /* tell_player(p, " Files succesfully nuked.\n"); */
   LOGF("nuke", "%s suicided [%s]", nuked, naddr);
}

/* suicide try # 6254246352 -- asty (feeling useless) */

void            confirm_suicide2(player * p, char *str)
{
   char *oldstack;
   player dummy;
   char *dummyname[MAX_NAME];
   saved_player *sp;
   char to_nuke[MAX_NAME] = "";
   char nuked[MAX_NAME] = "";
   char nukee[MAX_NAME] = "";
   char naddr[MAX_INET_ADDR] = "";
   int mesg_done = 0;
   int *scan, *scan_count, mcount = 0, sscan;
   int suicide = 0;
   note *smail, *snext;
   char *tempp;

   oldstack = stack;
   if (check_password (p->password, str, p))
   {
    tell_player (p, 
            "\n\n -=>              You have suicided!                  \n\n");
   suicide3(p,  (p->lower_name));
   return;
   } else 
   {
      password_mode_off(p);
      p->flags |= PROMPT;
      p->input_to_fn = 0;
      tell_player(p, "\n\n Password does not match.\n"
                     " You back away from the edge...\n");
   }
   stack = oldstack;
}

void            suicide(player * p, char *str)
{
    char *test_pw, *oldstack;
   FORMAT(str, " Format : suicide <reason>\n");
   if (ishcadmin(p->name)) {
	TELLPLAYER(p, " Um.. that'd be a security risk. No.\n");
	return;
   }
	
   LOGF("suicide", " %s [%s] %s trying to suicide for the reason: %s", p->name, p->email, isare(p), str);
   password_mode_on(p);
   p->flags &= ~PROMPT;
   do_prompt(p, "\007\n\n WARNING: You will lose ALL data including lists, rooms, etc.\n"
		"To ABORT the process, type an incorrect password, and you will still live.\n"
                " \n\n Enter your current password if you want to be deleted: ");
   p->input_to_fn = confirm_suicide2;
}


/* banish a player from the program */

void banish_player(player * p, char *old_str)
{
   char *oldstack, str[20], *i, ban_name[MAX_NAME + 1] = "";
   player *p2;
   saved_player *sp;
   int newbie=0;

#ifdef TRACK
   sprintf(functionin,"banish_player (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   oldstack = stack;
   FORMAT(old_str, " Format: banish <player>\n");

   strncpy(str, old_str, MAX_NAME - 3);
   sprintf(oldstack, "%s %s trying to banish %s.", p->name, isare(p), str);
   stack = end_string(oldstack);
   log("banish", oldstack);
   lower_case(str);
   p2 = find_player_absolute_quiet(str);
   if (!p2)
      tell_player(p, " No such person on the program.\n");
   if (p2)
   {
   if (!check_privs(p->residency, p2->residency))
      {
         tell_player(p, " You can't banish them !\n");
         sprintf(oldstack, " -=*> %s tried to banish you.\n", p->name);
         stack = end_string(oldstack);
         tell_player(p2, oldstack);
         stack = oldstack;
         return;
      }
      if ( p2->residency == 0 )
         newbie=1;
      sprintf(oldstack, "\n\n -=*> You have been banished !!!.\n\n\n");
      stack = end_string(oldstack);
      tell_player(p2, oldstack);
      p2->saved_residency |= BANISHD;
      p2->residency = p2->saved_residency;
      quit(p2, 0);
      strcpy(ban_name, p2->name);
   }
   if (!newbie)
   {
      strcpy(oldstack, str);
      lower_case(oldstack);
      stack = end_string(oldstack);
      sp = find_saved_player(oldstack);
      if (sp)
      {
         if (sp->residency & BANISHD)
         {
            tell_player(p," Already banished!\n");
            stack = oldstack;
            return;
         }
   if (!check_privs(p->residency, sp->residency))
         {
            tell_player(p, " You can't banish that save file !\n");
            stack = oldstack;
            return;
         }
         sp->residency |= BANISHD;
         set_update(*str);
         tell_player(p, " Player successfully banished.\n");
      } else
      {
      /* Create a new file with the BANISHD flag set */
         i = str;
         while (*i)
         {
            if (!isalpha(*i++))
            {
               tell_player(p, " Banished names must only contain letters and must not be too large.\n");
               return;
            }
         }
         create_banish_file(str);
         tell_player(p, " Name successfully banished.\n");
      }
      if (ban_name[0] == '\0')
      {
         sprintf(ban_name, "\'%s\'", str);
      }
   }
   if (ban_name[0] != '\0')
   {
      stack = oldstack;
      if (p->gender==PLURAL)
	sprintf(stack, " -=*> %s smite the name %s with a banish.\n", p->name, ban_name);
      else
	sprintf(stack, " -=*> %s smites the name %s with a banish.\n", p->name, ban_name);
      stack = end_string(stack);
      su_wall(oldstack);
   }
   stack = oldstack;
}


/* Unbanish a player or name */

void unbanish(player *p, char *str)
{
   saved_player *sp;
   char *oldstack;

#ifdef TRACK
   sprintf(functionin,"unbanish (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   oldstack = stack;
   lower_case(str);
   sp = find_saved_player(str);
   if (!sp)
   {
      tell_player(p, " Can't find saved player file for that name.\n");
      return;
   }
   if ( !(sp->residency & BANISHD) )
   {
      tell_player(p, " That player isn't banished!\n");
      return;
   }
   if ( sp->residency == BANISHD || sp->residency == BANISHED )
   {
      remove_player_file(str);
      if (p->gender==PLURAL)
	sprintf(stack, " -=*> %s unbanish the Name \'%s\'\n", p->name, str);
      else
	sprintf(stack, " -=*> %s unbanishes the Name \'%s\'\n", p->name, str);
      stack = end_string(stack);
      su_wall(oldstack);
      stack = oldstack;
      return;
   }
   sp->residency &= ~BANISHD;
   set_update(*str);
   sync_to_file(str[0], 0);
   sprintf(stack, " -=*> %s unbanishes the Player \'%s\'\n", p->name, str);
   stack = end_string(stack);
   su_wall(oldstack);
   log("banish",oldstack);
   stack = oldstack;
}


/* create a new character */
/* WARNING: this command isn't very relyable -- best to logon a newbie
   and ressie that way, than to use this. Just a warning to the wise */

void make_new_character(player * p, char *str)
{
   char *oldstack, *cpy, *email, *password=0;
   player *np;
   int length = 0;

#ifdef TRACK
   sprintf(functionin,"make_new_character (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   oldstack = stack;
   email = next_space(str);

   if (!*str || !*email)
   {
      tell_player(p, " Format: make <character name> <email addr> "
                     "<password>\n");
      return;
   }

/* chop the argument into "name\000email\000password\000" with ptrs as
   appropriate */
   *email++ = 0;

   password = end_string(email);
   while (*password != ' ')
      *password--;
   *password++ = 0;

   for (cpy = str; *cpy; cpy++)
   {
      if (isalpha(*cpy))
      {
         *stack++ = *cpy;
         length++;
      }
   }
   *stack++ = 0;
   if (length > (MAX_NAME - 2))
   {
      tell_player(p, " Name too long.\n");
      stack = oldstack;
      return;
   }
   if (find_saved_player(oldstack))
   {
      tell_player(p, " That player already exists.\n");
      stack = oldstack;
      return;
   }
   np = create_player();
   np->flags &= ~SCRIPTING;
   strcpy(np->script_file, "dummy");
   np->fd = p->fd;
   np->location = (room *) -1;

   restore_player(np, oldstack);
   np->flags &= ~SCRIPTING;
   strcpy(np->script_file, "dummy");
   strcpy (np->inet_addr, "NOT YET LOGGED ON");
   np->residency = get_flag(permission_list, "residency");
   np->saved_residency = np->residency;

   /* Crypt that password, why don't you */

   strcpy(np->password, do_crypt(password, np));

   /* strncpy(np->password,oldstack,(MAX_PASSWORD-2)); */

   strncpy(np->email, email, (MAX_EMAIL - 3));
   save_player(np);
   np->fd = 0;
   np->location = 0;
   destroy_player(np);
   cpy = stack;
   sprintf(cpy, "%s creates %s.", p->name, oldstack);
   stack = end_string(cpy);
   log("make", cpy);
   tell_player(p, " Player created.\n");
   stack = oldstack;
   return;
}


/* port from EW dump file */

void port(player * p, char *str)
{
   char *oldstack, *scan;
   player *np;
   file old;

#ifdef TRACK
   sprintf(functionin,"port (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   old = load_file("files/old.players");
   scan = old.where;

   while (old.length > 0)
   {
      while (*scan != ' ')
      {
         *stack++ = *scan++;
         old.length--;
      }
      scan++;
      *stack++ = 0;
      strcpy(stack, oldstack);
      lower_case(stack);
      if (!find_saved_player(stack))
      {
         np = create_player();
         np->fd = p->fd;
         restore_player(np, oldstack);
         np->residency = get_flag(permission_list, "residency");
         stack = oldstack;
         while (*scan != ' ')
         {
            *stack++ = *scan++;
            old.length--;
         }
         *stack++ = 0;
         scan++;
         strncpy(np->password, oldstack, MAX_PASSWORD - 3);
         stack = oldstack;
         while (*scan != '\n')
         {
            *stack++ = *scan++;
            old.length--;
         }
         *stack++ = 0;
         scan++;
         strncpy(np->email, oldstack, MAX_EMAIL - 3);
         sprintf(oldstack, "%s [%s] %s\n", np->name, np->password, np->email);
         stack = end_string(oldstack);
         tell_player(p, oldstack);
         stack = oldstack;
         save_player(np);
         np->fd = 0;
         destroy_player(np);
      } else
      {
         while (*scan != '\n')
         {
            scan++;
            old.length--;
         }
         scan++;
      }
   }
   if (old.where)
      FREE(old.where);
   stack = oldstack;
}



/*
 * rename a person (yeah, right, like this is going to work .... )
 * 
 */

void do_rename(player * p, char *str, int verbose)
{
   char *oldstack, *firspace, name[MAX_NAME + 2], *letter, *oldlist;
   char oldname[MAX_NAME+2];
   int *oldmail;
   int hash;
   player *oldp, *scan, *previous;
   saved_player *sp,*oldsp;
   room *oldroom,*rscan;

#ifdef TRACK
   sprintf(functionin,"do_rename (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   oldstack = stack;
   FORMAT(str, " Format: rename <person> <new-name>\n");
   if (!(firspace = strchr(str, ' ')))
      return;
   *firspace = 0;
   firspace++;
   letter = firspace;
   if (!(oldp = find_player_global(str)))
      return;
   if (oldp->residency & BASE)
   {
      sprintf(stack, " But you cannot rename %s. They are a resident.\n"
	      , oldp->name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
/*   if (oldp->residency > p->residency)
   {
      sprintf(stack, " But you cannot rename %s. They have more privs than "
	      "you.\n", oldp->name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }*/
   strcpy(oldname,oldp->lower_name);
   scan = find_player_global_quiet(firspace);
   if (scan)
   {
      sprintf(stack, " There is already a person with the name '%s' "
                     "logged on.\n", scan->name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   strcpy(name, firspace);
   lower_case(name);
   sp = find_saved_player(name);
   if (sp)
   {
      sprintf(stack, " There is already a person with the name '%s' "
                     "in the player files.\n", sp->lower_name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
   }
   /* Test for a nice inputted name */

   if (strlen(letter) > MAX_NAME - 2 || strlen(letter) < 2)
   {
      tell_player(p, " Try picking a name of a decent length.\n");
      stack = oldstack;
      return;
   }
   while (*letter)
   {
      if (!isalpha(*letter))
      {
         tell_player(p, " Letters in names only, please ...\n");
         stack = oldstack;
         return;
      }
      *letter++;
   }

   /* right, newname doesn't exist then, safe to make a change (I hope) */
   /* Remove oldp from hash list */

   scan = hashlist[oldp->hash_top];
   previous = 0;
   while (scan && scan != oldp)
   {
      previous = scan;
      scan = scan->hash_next;
   }
   if (!scan)
      log("error", "Bad hash list (rename)");
   else if (!previous)
      hashlist[oldp->hash_top] = oldp->hash_next;
   else
      previous->hash_next = oldp->hash_next;

   strcpy(name, oldp->lower_name);
   strncpy(oldp->name, firspace, MAX_NAME - 3);
   lower_case(firspace);
   strncpy(oldp->lower_name, firspace, MAX_NAME - 3);

   /* now place oldp back into named hashed lists */

   hash = ((int) (oldp->lower_name[0]) - (int) 'a' + 1);
   oldp->hash_next = hashlist[hash];
   hashlist[hash] = oldp;
   oldp->hash_top = hash;

   /*This section ONLY if they are a resident*/
   if (oldp->residency & BASE)
     {
       /*Change the lower_name in the saved player area*/
       strcpy(oldp->saved->lower_name,oldp->lower_name);
       
       /*Find all the rooms, and then transfer them*/

       /*Get the rooms info from the OLD playerfile*/
       /*And reset the owner of them*/
       if (oldsp)
	 for (rscan=oldsp->rooms;rscan;rscan=rscan->next)
	   rscan->owner=oldp->saved;
     }

   if (oldp->saved)
     save_player(oldp);
   stack = oldstack;
   if (verbose)
   {
      sprintf(stack, " %s dissolves in front of your eyes, and "
                     "rematerialises as %s ...\n", name, oldp->name);
      stack = end_string(stack);

      /* tell room */
      scan = oldp->location->players_top;
      while (scan)
      {
        if (scan != oldp && scan != p)
           tell_player(scan, oldstack);
        scan = scan->room_next;
      }
      stack = oldstack;
      sprintf(stack, "\n -=*> %s %s just changed your name to be '%s' ...\n\n",
         p->name, havehas(p), oldp->name);
      stack = end_string(stack);
      tell_player(oldp, oldstack);
   }
   tell_player(p, " Tis done ...\n");
   stack = oldstack;

   /* log it */
   sprintf(stack, "Rename by %s - %s to %s", p->name, name, oldp->name);
   stack = end_string(stack);
   log("rename", oldstack);
   stack = oldstack;
   if (p->gender==PLURAL)
     sprintf(stack, " -=*> %s rename %s to %s.\n", p->name, name, 
oldp->name);
   else
     sprintf(stack, " -=*> %s renames %s to %s.\n", p->name, name, 
oldp->name);
   stack = end_string(stack);
   su_wall(oldstack);
   stack = oldstack;
}


/* User interface to renaming a newbie */

void rename_player(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"rename_player (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   do_rename(p, str, 1);
}

void quiet_rename(player * p, char *str)
{
#ifdef TRACK
   sprintf(functionin,"quiet_rename (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   do_rename(p, str, 0);
}


/* reset email of a player */
/* this version even manages to check if they are logged in at the time :-/ */
/* leave the old one in a little while until we are sure this works */

void blank_email(player * p, char *str)
{
   player dummy;
   player *p2;
   char *space, *oldstack;

#ifdef TRACK
   sprintf(functionin,"blank_email (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   /* we need the stack for printing some stuff */
   oldstack = stack;

   /* spot incorrect syntax */
   FORMAT(str, " Format: blank_email <player> [<email>]\n");

   /* spot lack of sensible email address */
   space = 0;
   space = strchr(str, ' ');
   if (space != NULL)
   {
      *space++ = 0;
      if (strlen(space) < 7)
      {
         tell_player(p, " Try a reasonable email address.\n");
         return;
      }
   }

   /* look for them on the prog */
   lower_case(str);
   p2 = find_player_absolute_quiet(str);

   /* if player logged in */
   if (p2)
   {
       /* no blanking the emails of superiors... */
   if (!check_privs(p->residency, p2->residency))
	   /* naughty, naughty, so tell the person, the target, and the
	      su channel */
       {
	   tell_player(p, " You cannot blank that person's email address.\n");
	   sprintf(stack, " -=*> %s tried to blank your email address, but "
		   "failed.\n", p->name);
	   stack = end_string(stack);
	   tell_player(p2, oldstack);
	   stack = oldstack;
	   sprintf(stack, " -=*> %s failed in an attempt to blank the email "
		   "address of %s.\n", p->name, p2->name);
	   stack = end_string(stack);
	   su_wall_but(p, oldstack);
	   stack = oldstack;
	   return;
       }
       else
	   /* p is allowed to do things to p2 */
       {
	   /* tell the target and the SUs all about it */
	   if (space == NULL)
	       sprintf(stack, " -=*> Your email address has been blanked "
		       "by %s.\n", p->name);
	   else
	       sprintf(stack, " -=*> Your email address has been changed "
		       "by %s.\n", p->name);	       
	   stack = end_string(stack);
	   tell_player(p2, oldstack);
	   stack = oldstack;
	   if (space == NULL)
	       sprintf(stack, " -=*> %s %s their email blanked by %s.\n",
		       p2->name, havehas(p2), p->name);
	   else
	       sprintf(stack, " -=*> %s %s their email changed by %s.\n",
		       p2->name, havehas(p2), p->name);
	   stack = end_string(stack);
	   su_wall_but(p, oldstack);
	   stack = oldstack;

	   /* actually blank it, and flag the player for update */
	   /* and avoid strcpy from NULL since it's very dodgy */
	   if (space != NULL)
	       strcpy(p2->email, space);
	   else
	       p2->email[0] = 0;
	   set_update(*str);

	   /* report success to the player */
	   if (space == NULL)
	       sprintf(stack, " -=*> You successfully blank %s's email.\n", 
		       p2->name);
	   else
	       sprintf(stack, " -=*> You successfully change %s's email.\n", 
		       p2->name);
	   stack = end_string(stack);
	   tell_player(p, oldstack);
	   stack = oldstack;
	   /* log the change */
	   if (space == NULL)
	       sprintf(stack, "%s blanked %s's email address (logged in)",
		       p->name, p2->name);
	   else
	       sprintf(stack, "%s changed %s's email address (logged in)",
		       p->name, p2->name);
	   stack = end_string(stack);
	   log("blanks", oldstack);
	   return;
       }
   }
   else
       /* they are not logged in, so load them */
       /* set up the name and port first */
   {
       strcpy(dummy.lower_name, str);
       dummy.fd = p->fd;
       if (load_player(&dummy))
       {
	   /* might as well point this out if it is so */
 	   if (dummy.residency & BANISHD)
	   {
	       tell_player(p, " By the way, this player is currently BANISHED.");
	       if (dummy.residency == BANISHD)
	       {
		   tell_player(p, " (Name Only)\n");
	       } else
	       {
		   tell_player(p, "\n");
	       }
	   }
	   /* announce to the SU channel */
	   if (space == NULL)
	       sprintf(stack, " -=*> %s blanks the email of %s, who is "
		       "logged out at the moment.\n", p->name, dummy.name);
	   else
	       sprintf(stack, " -=*> %s changes the email of %s, who is "
		       "logged out at the moment.\n", p->name, dummy.name);
	   stack = end_string(stack);
	   su_wall_but(p, oldstack);
	   stack = oldstack;
	   /* change or blank the email address */
	   if (space == NULL)
	       dummy.email[0] = 0;
	   else
	       strcpy(dummy.email, space);

	   /* report success to player */
	   if (space == NULL)
	       sprintf(stack, " -=*> Successfully blanked the email of %s, "
		       "not logged in atm.\n", dummy.name);
	   else
	       sprintf(stack, " -=*> Successfully changed the email of %s, "
		       "not logged in atm.\n", dummy.name);
	   stack = end_string(stack);
	   tell_player(p, oldstack);
	   stack = oldstack;

	   /* and log it */
	   if (space == NULL)
	       sprintf(stack, "%s blanked %s's email address (logged out)",
		       p->name, dummy.name);
	   else
	       sprintf(stack, "%s changed %s's email address (logged out)",
		       p->name, dummy.name);
	   stack = end_string(stack);
	   log("blanks", oldstack);
	   stack = oldstack;

	   /* save char LAST thing so maybe we won't blancmange the files */
	   dummy.script = 0;
	   dummy.script_file[0] = 0;
	   dummy.flags &= ~SCRIPTING;
	   dummy.location = (room *) -1;
	   save_player(&dummy);
	   return;
       }
       else
	   /* name does not exist, tell the person so and return */
       {
	   sprintf(stack, " -=*> The name '%s' was not found in saved files.\n",
		   dummy.name);
	   stack = end_string(stack);
	   tell_player(p, oldstack);
	   stack = oldstack;
	   return;
       }
   }
}

/* The almighty dumb command!!!! */

void dumb(player *p, char *str)
{
   player *d;

#ifdef TRACK
   sprintf(functionin,"dumb (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   FORMAT(str, " Format: dumb <player>\n");
   if (!strcasecmp(str, "me"))
   {
      tell_player(p, " Do you REALLY want to tweedle yourself?\n");
      return;
   }
   d = find_player_global(str);
   if (d)
   {
      if (d == p)
      {
         tell_player(p, " Do you REALLY want to tweedle yourself?\n");
         return;
      }
      if (d->flags & FROGGED)
      {
         tell_player(p, " That player is ALREADY a tweedle\n");
         return;
      }
   if (!check_privs(p->residency, d->residency))
      {
         tell_player(p, " You can't do that!\n");
         TELLPLAYER(d, " -=*> %s tried to tweedle you!\n", p->name);
         return;
      }
      d->flags |= FROGGED;
      d->system_flags |= SAVEDFROGGED;
      TELLPLAYER(p, " You turn %s into a tweedle!\n", d->name);
      TELLPLAYER(d, " -=*> %s turn%s you into a tweedle!\n", 
		p->name,single_s(p));
      SW_BUT(p, " -=*> %s turn%s %s into a tweedle!\n", p->name,
	      single_s(p), d->name);
      LOGF("dumb", " -=*> %s turn%s %s into a tweedle!", p->name,
	      single_s(p), d->name);
   }
}

/* Well, I s'pose we'd better have this too */

void undumb(player *p, char *str)
{
   player *d;
   saved_player *sp;

#ifdef TRACK
   sprintf(functionin,"undumb (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   FORMAT(str, " Format: undumb <player>\n");
   d = find_player_global(str);
   if (d)
   {
      if (d == p)
      {
         if (p->flags & FROGGED)
         {
            tell_player(p, " You can't, you spoon!\n");
	    if (p->gender==PLURAL)
	      SW_BUT(p, " -=*> %s try to untweedle %s...\n", p->name,
		      self_string(p));
	    else
	      SW_BUT(p, " -=*> %s tries to untweedle %s...\n", p->name,
		      self_string(p));
         } else
            tell_player(p, " But you're not a tweedle...\n");
         return;
      }
      if (!(d->flags & FROGGED))
      {
          tell_player(p, " That person isn't a tweedle...\n");
          return;
      }
      d->flags &= ~FROGGED;
      d->system_flags &= ~SAVEDFROGGED;
      if (p->gender==PLURAL)
	TELLPLAYER(d, " -=*> The %s all zap you and you are no longer a "
		"tweedle.\n",p->name);
      else
	TELLPLAYER(d, " -=*> %s zaps you and you are no longer a tweedle.\n",
		p->name);
      TELLPLAYER(p, " You zap %s and %s %s no longer a tweedle.\n", d->name,
              gstring(d),isare(d));
      if (p->gender==PLURAL){
	SW_BUT(p,  " -=*> The %s all zap %s, and %s %s no longer a "
		"tweedle.\n",p->name, d->name, gstring(d), isare(d));
      }else{
	SW_BUT(p, " -=*> %s zaps %s, and %s %s no longer a tweedle.\n",
		p->name, d->name, gstring(d), isare(d));
      }
	LOGF("dumb", " -=*> %s untweedles %s", p->name, d->name);
   } else
   {
      tell_player(p, " Checking saved files...\n");
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " Not found.\n");
         return;
      }
      if (!(sp->system_flags & SAVEDFROGGED))
      {
         tell_player(p, " But that person isn't a tweedle...\n");
         return;
      }
      sp->system_flags &= ~SAVEDFROGGED;
      TELLPLAYER(p, " Ok, %s is no longer a tweedle.\n", sp->lower_name);
      SW_BUT(p, " -=*> %s untweedle%s %s.\n", p->name, single_s(p), sp->lower_name);
      LOGF("dumb", " %s untweedle%s %s.", p->name, single_s(p), sp->lower_name);
   }
}


/* continuous scripting of a connection */

void script(player *p, char *str)
{
   char *oldstack;
   time_t t;
   char time_string[16];

#ifdef TRACK
   sprintf(functionin,"script (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   oldstack = stack;
   if (p->flags & SCRIPTING)
   {
      if (!*str)
      {
         tell_player(p, " You are ALREADY scripting! ('script off' to turn"
                        " current scripting off)\n");
      }
      if (!strcasecmp(str, "off"))
      {
         p->flags &= ~SCRIPTING;
         sprintf(stack, " -=*> Scripting stopped at %s\n",
                 convert_time(time(0)));
         stack = end_string(stack);
         tell_player(p, oldstack);
         *(p->script_file)=0;
         stack = oldstack;
         sprintf(stack, " -=*> %s has stopped continuous scripting.\n", 
p->name);
         stack = end_string(stack);
         su_wall(oldstack);
      }
      stack = oldstack;
      return;
   }

   if (!*str)
   {
      tell_player(p, " You must give a reason for starting scripting.\n");
      return;
   }
   p->flags |= SCRIPTING;
   sprintf(stack, " -=*> Scripting started at %s, for reason \'%s\'\n",
           convert_time(time(0)), str);
   stack = end_string(stack);
   tell_player(p, oldstack);
   stack = oldstack;
   sprintf(stack, " -=*> %s has started continuous scripting with reason "
                  "\'%s\'\n"
           , p->name, str);
   stack = end_string(stack);
   su_wall(oldstack);
   stack = oldstack;
   t = time(0);
   strftime(stack, 16, "%d%m%y%H%M%S", localtime(&t));
   stack = end_string(stack);
   sprintf(p->script_file, "%s%s", p->name, oldstack);
   stack = oldstack;
   sprintf(stack, "logs/scripts/%s.log", p->script_file);
   stack = end_string(stack);
   unlink(oldstack);
   stack = oldstack;
}

/* the yoyo commannd changed to the WHOMP messages (astyanax 5/2/95) */

void yoyo(player *p, char *str)
{
   player *p2;
   int tmp_nrd = 0;

#ifdef TRACK
   sprintf(functionin,"yoyo (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   FORMAT(str, " Format: yoyo <player>\n");

   p2 = find_player_global(str);
   if (!p2)
   {
      TELLPLAYER(p, " No-one of the name '%s' on at the moment.\n", str);
      return;
   }
   if (!check_privs(p->residency, p2->residency))
   {
      TELLPLAYER(p, " You fail to kick %s's ass anywhere - uh oh...\n", p2->name);
      TELLPLAYER(p2, " -=*> %s tried to kick your ass,  Sick 'em!\n", p->name);
      return;
   }
   if (p->gender==PLURAL)
   SUWALL(" -=*> %s kick the crap out of %s.\n", p->name, p2->name);
   else
   SUWALL(" -=*> %s kicks the crap out of %s.\n", p->name, p2->name);
   LOGF("yoyo", "%s kicked %s.'s ass", p->name, p2->name);
   /* can't let them avoid the yoyo just by blocking room desc... */
   if (p2->tag_flags & BLOCK_ROOM_DESC) {
		tmp_nrd = 1;
		p2->tag_flags &= ~BLOCK_ROOM_DESC;
   }
   command_type |= ADMIN_BARGE;
   TELLROOM(p2->location, " %s %s in need of a medic after an encounter with"
                  " some superuser!\n", p2->name, isare(p2));
   trans_to(p2, "main.boot");
   TELLROOM(p2->location, " %s staggers in reeling from a blow from some SU and then vanishes!\n",
	     p2->name);
   trans_to(p2, "main.room");
   TELLROOM(p2->location, " %s falls back on earth bruised and battered *plop*\n",p2->name);
   command_type |= HIGHLIGHT;
   tell_player(p2, "  You musta been bad cause some SU just used you as a punching bag!\n");
   tell_player(p2, " If you'd rather not have it happen again, take a look"
                   " at the rules and consider following them...\n");
   tell_player(p2, " Have a nice day *smirk*\n");
   command_type &= ~HIGHLIGHT;
   if (tmp_nrd) {
	p2->tag_flags |= BLOCK_ROOM_DESC;
   }
}

void sban(player * p, char *str)
{
  char *oldstack, *site_alpha;
  

  CHECK_DUTY(p);

  FORMAT(str, " Format: site_ban <site_num> <reason>\n");
  if (!isdigit(*str))
   {	tell_player(p, " You can only site_ban numeric (IP) addresses. try again.\n");
	return; }

  site_alpha = next_space(str);
  FORMAT(site_alpha, " Format: site_ban <site_num> <reason>\n");
  *site_alpha++ = 0;
  tell_player(p, " Site has been completely banned ...\n");
  oldstack = stack;
  sprintf(stack, "%s  C     # %s (%s)\n", str, site_alpha, p->name);
  stack = end_string(stack);
  banlog("banish", oldstack);
  stack = oldstack;
  if (banish_file.where)
    free(banish_file.where);
  banish_file = load_file("files/banish");
  tell_player(p, " New banish file uploaded.\n");
}

void nban(player * p, char *str)
{
  char *oldstack, *site_alpha;
  

  CHECK_DUTY(p);

  FORMAT(str, " Format: newbie_ban <site_num> <reason>\n");
  if (!isdigit(*str))
   {	tell_player(p, " You can only newbie_ban numeric (IP) addresses. try again.\n");
	return; }
  site_alpha = next_space(str);
  FORMAT(site_alpha, " Format: newbie_ban <site_num> <reason>\n");
  *site_alpha++ = 0;
  tell_player(p, " Site has been newbie banned ...\n");
  oldstack = stack;
  sprintf(stack, "%s  N         # %s (%s)\n", str, site_alpha, p->name);
  stack = end_string(stack);
  banlog("banish", oldstack);
  stack = oldstack;
  if (banish_file.where)
    free(banish_file.where);
  banish_file = load_file("files/banish");
  tell_player(p, " New banish file uploaded.\n");
}


/* For an SU to go back on duty */

void on_duty(player * p, char *str)
{
   player *p2;

#ifdef TRACK
   sprintf(functionin,"on_duty (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

if ((*str) && (p->residency & ADMIN))
 {
      p2 = find_player_global(str);
      if (!p2)
      {
	 return;
      }
      else if (!(p2->residency & SU))
      {
      tell_player(p," -=*> What?! That person isn't even an SU! *boggle*\n");
      return;
      }
      else if ((p2->flags & BLOCK_SU) == 0)
      {
	 tell_player(p, " Ummmm... that player is ALREADY on_duty...\n");
	 return;
      }
      else
      /* ok - so p2 is an off_duty su. Lets get the slacker back on duty */
      {   
	 p2->flags &= ~BLOCK_SU;
	 TELLPLAYER(p2, " -=*> %s forces your ass back to work...\n", p->name);
	 TELLPLAYER(p, " -=*> You force %s back on_duty... \n", p2->name);
	 LOGF("duty", " -=*> %s forces %s to return to duty...", p->name, p2->name);
	 SW_BUT(p2, " -=*> %s forces %s to return to duty...\n", p->name, p2->name);
      }
 }
else
 {
 p2 = p; 

 if ((p->flags & BLOCK_SU) != 0)
   {
      p->flags &= ~BLOCK_SU;
      if ((p->flags & OFF_LSU) != 0)
	p->flags &= ~OFF_LSU;
      tell_player(p, " You return to duty.\n");
      p->residency = p->saved_residency;
	LOGF("duty", "%s --> onduty", p->name);
	SW_BUT(p2, " -=*> %s return%s to duty.\n", p->name, single_s(p));
   } 
 else if ((p->flags & OFF_LSU) != 0)
   {
      p->flags &= ~OFF_LSU;
      tell_player(p, " You return to visible duty.\n");
      p->residency = p->saved_residency;
   } 
 else
   {
      tell_player(p, " Are you asleep or something? You are ALREADY On Duty!"
		     " <smirk>\n");
   }
 }
}


/* For an SU to go off duty */

void block_su(player * p, char *str)
{
   player *p2;

#ifdef TRACK
   sprintf(functionin,"block_su (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

if ((*str) && (p->residency & ADMIN))
 {
      p2 = find_player_global(str);
      if (!p2)
      {
	 return;
      }
      else if (!(p2->residency & SU))
      {
	 tell_player(p," -=*> What?! That person isn't even an SU! *boggle*\n");
	 return;
      }
      else if ((p2->flags & BLOCK_SU) != 0)
      {
	 tell_player(p, " Ummmm... that player is ALREADY off_duty...\n");
	 return;
      }
      else
      /* ok - so p2 is an on_duty su. Lets give the git a break */
      {   
	 p2->flags |= BLOCK_SU;
	 TELLPLAYER(p2, " -=*> %s forces you to take a break...\n", p->name);
	 TELLPLAYER(p, " -=*> You force %s to take a break... \n", p2->name);
	 p2->residency = p2->saved_residency;
	 LOGF("duty"," -=*> %s forces %s to take a break...", p->name, p2->name);
	 SW_BUT(p2," -=*> %s forces %s to take a break...\n", p->name, p2->name);
      }
 }
else
 {
 p2 = p; 
 if ((p->flags & BLOCK_SU) == 0)
   {
      p->flags |= BLOCK_SU;
      tell_player(p, " You're now off duty ... "
		  "\n");

      p->saved_residency = p->residency;
      if (p->gender==PLURAL)
	SW_BUT(p2, " -=*> %s go off duty.\n", p->name);
      else
	SW_BUT(p2, " -=*> %s goes off duty.\n", p->name);
       LOGF("duty", "%s --> offduty", p->name);
   } else
   {
      tell_player(p, " But you are ALREADY Off Duty! <boggle>\n");
   }
 }
}

/* For an admin to go back on visible duty */

void on_lsu(player * p, char *str)
{
   player *p2;

#ifdef TRACK
   sprintf(functionin,"on_lsu (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

if ((*str) && (p->residency & ADMIN))
 {
      p2 = find_player_global(str);
      if (!p2)
      {
	 return;
      }
      else if (!(p2->residency & SU))
      {
	 tell_player(p," -=*> What?! That person isn't even an SU! *boggle*\n");
	 return;
      }
      else if ((p2->flags & OFF_LSU) == 0)
      {
	 tell_player(p, " Ummmm... that player is ALREADY on_lsu...\n");
	 return;
      }
      else
      /* ok - so p2 is an off_lsu su. Lets get the slacker back on duty */
      {   
	 p2->flags &= ~OFF_LSU;
	 TELLPLAYER(p2, " -=*> %s forces you back onto ressie lsu...\n", p->name);
	 TELLPLAYER(p, " -=*> You force %s back onto lsu... \n", p2->name);
	 p2->residency = p2->saved_residency;
      }
 }else
 {
 p2 = p;
 if ((p->flags & OFF_LSU) != 0)
   {
      p->flags &= ~OFF_LSU;
      tell_player(p, " You return to visible duty.\n");
      p->residency = p->saved_residency;
   } else
   {
      tell_player(p, " Are you asleep or something? You are ALREADY On lsu!"
		     " <smirk>\n");
   }
 }
}

/* For an admin to go off visible duty */

void off_lsu(player * p, char *str)
{
   player *p2;

#ifdef TRACK
   sprintf(functionin,"off_lsu (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

if ((*str) && (p->residency & ADMIN))
 {
      p2 = find_player_global(str);
      if (!p2)
      {
	 return;
      }
      else if (!(p2->residency & SU))
      {
	 tell_player(p," -=*> What?! That person isn't even an SU! *boggle*\n");
	 return;
      }
      else if ((p2->flags & OFF_LSU) != 0)
      {
	 tell_player(p, " Ummmm... that player is ALREADY off_duty...\n");
	 return;
      }
      else
      /* ok - so p2 is an on_duty su. Lets give the git a break */
      {   
	 p2->flags |= OFF_LSU;
	 TELLPLAYER(p2," -=*> %s removes you from ressie lsu...\n", p->name);
	 TELLPLAYER(p," -=*> You force %s off of lsu... \n", p2->name);
	 p2->residency = p2->saved_residency;
      }
 }else
 {
   p2 = p;
   if ((p->flags & OFF_LSU) == 0)
   {
      p->flags |= OFF_LSU;
      tell_player(p, " You're now off visible duty ... "
		  "no more ressies buggin ya ..."
		  "\n");

      p->saved_residency = p->residency;
   } else
   {
      tell_player(p, " But you are ALREADY Off Duty! <boggle>\n");
   }
 }
}

/* Marriage - not again *sigh* - astyanax & traP */
/* hey beavis, *I* didn't invent the damn thing ;)  */
void marry(player * p, char *str)
{
   player *p2;
   saved_player *sp;
   player *p3;
   saved_player *sp3;
   int change;
   char *temp;
   temp = next_space(str);

   if (!(p->system_flags & MINISTER)) {
	tell_player(p, " You must be a minister to use this command.\n");
	return; }
   FORMAT(str, " Format: marry <player> <player>\n");
   change = MARRIED;
   *temp++ = 0;
   p2 = find_player_global(str);
   p3 = find_player_global(temp);


      if ((!p2))
      {
         tell_player(p, " Don't you think that it'd be nice to have the BRIDE AND GROOM HERE? *boggle*\n");
         return;
      }
      if ((!p3))
      {
         tell_player(p, " Well... marry to WHO? it takes 2 to tango... \n");
         return;
      }
      if (p2==p3) {
         tell_player(p, " Wouldn't that be just a bit odd?\n");
         return;
      }
      if ((p2->system_flags & MARRIED) || (p3->system_flags & MARRIED))
      {
         tell_player(p, " But at least one of them is ALREADY married.\n");
         return;
      }
      if ((p2->residency == NON_RESIDENT) || (p3->residency == NON_RESIDENT))
      {
         tell_player(p, " That player is non-resident!\n");
         return;
      }
      if ((p2->residency == BANISHD || p2->residency == SYSTEM_ROOM)
          || (p3->residency == BANISHD || p3->residency == SYSTEM_ROOM))

      {
         tell_player(p, " That is a banished NAME, or System Room.\n");
         return;
      }
      if (!(p2->system_flags & ENGAGED) || !(p3->system_flags & ENGAGED) ||
	(strcmp(p2->married_to, p3->name)) || (strcmp(p3->name, p2->married_to)))
      {
	tell_player(p, " They aren't engaged to each other !!\n");
	return;
      }
      TELLPLAYER(p2,"\n\n -=*> You are now officially net.married.\n\n");
      TELLPLAYER(p3,"\n\n -=*> You are now officially net.married.\n\n");
      p2->system_flags |= MARRIED;
      p3->system_flags |= MARRIED;
      LOGF("marry","%s married %s and %s", p->name, p2->name, p3->name);
      strncpy(p2->married_to, p3->name, MAX_NAME - 3);
      strncpy(p3->married_to, p2->name, MAX_NAME - 3);
      save_player(p2);
      save_player(p3);
      tell_player(p, " And they lived happily ever after...\n");

}

/* And they lived happily ever after .......until!! */

void divorce(player * p, char *str)
{
   player *p2, *p3;
   saved_player *sp;
   int change;
   char *tmp;

   if (!(p->system_flags & MINISTER)) {
	tell_player(p, " You must be a minister to use this command.\n");
	return; }
   tmp = next_space(str);
   if (!*str || *tmp)
   {
      tell_player(p, " Format: divorce <whoever>\n");
        return;
   }
   change = MARRIED;
   change |= ENGAGED; /* just in case */
   p2 = find_player_global(str);
   if (!p2)
   {
   tell_player(p, " Erm... that's not nice... The person requesting divorce"
                " must be logged on... \n");
   return;
   }
   p3 = find_player_global(p2->married_to);
   if (!p3)
   {
	tell_player(p, "The mate is not logged on - ignoring.\n");
/* need to load the saved player as a dummy *sigh* 
      sp = find_saved_player(str);
      if (!sp)
      {
       tell_player(p, " Couldn't find a mate - eek!...\n");
      }
      if (sp && !(sp->system_flags & MARRIED))
      {
       tell_player(p, " Interesting -- the mate seems to be unmarried...\n");
      }
      else if (sp)
      {
      sp->system_flags &= ~change;
      set_update(*str);
      tell_player(p, " Permissions changed in save files.\n");
      stack = oldstack;
      }
*/ }
   {
      p2->system_flags &= ~change;
      strncpy (p2->married_to, "", MAX_NAME -3);
      if (p3)
      {
      p3->system_flags &= ~change;
      strncpy (p3->married_to, "", MAX_NAME -3);
      }
      tell_player(p2, "\n\n You are now divorced. \n");
      if (p3)
      {
        tell_player(p3, "\n\n You are now divorced. \n");
    }
    if (p3)
       LOGF("marry"," %s divorced %s & %s", p->name, p2->name, p3->name);
    else
       LOGF("marry"," %s granted an anulment to %s", p->name, p2->name);
      if (p2->residency != NON_RESIDENT)
         save_player(p2);
      else
         remove_player_file(p2->lower_name);
      if (p3)
      {
      if (p3->residency != NON_RESIDENT)
         save_player(p3);
      else
         remove_player_file(p3->lower_name);
      }
      tell_player(p, " A wise being once said ..All good things must come to an end.\n");
   }
}

/* decapitate =) change what users type into all caps all the time */
void decap_player(player *p, char *str)
{
player *p2;

 	/* yes, this IS intended as a little joke :-P */
	FORMAT(str, " FORMAT: DECAP <ALL_CAPS_LOSER>\n");
  	CHECK_DUTY(p);
	p2 = find_player_global(str);
	if (p2)
	{
   if (!check_privs(p->residency, p2->residency))
	    {
		tell_player(p, " No way beavis!!\n");
		TELLPLAYER(p2, "%s tried to decap you...\n", p->name);
		return;
	    }
	    if (p2->system_flags & DECAPPED)
	    {
		SUWALL(" -=*> %s regrants caps to %s.\n", p->name, p2->name);
		p2->system_flags &= ~DECAPPED;
		tell_player(p2, " -=> Your shift key is returned to you.\n");
	    }
	    else
	    {
		SUWALL(" -=*> %s decides to break %s's caps lock key.\n", p->name, p2->name);
		LOGF("decap", " %s removes caps from %s.", p->name, p2->name);
		tell_player(p2, " -=*> Someone nukes your shift key!! oi!\n");
		p2->system_flags |= DECAPPED;
	    }
	}
}	

/* some more stuff for marrying -- net.propose */

void net_propose(player * p, char *str) {

	player *q;
	list_ent *l;

	FORMAT(str, " Format: propose <player>\n");
	if (p->system_flags & MARRIED) {
		tell_player(p, " But you are already married !!\n");
		return;
		}
	if (p->system_flags & ENGAGED) {
		tell_player(p, " But you are already engaged !!\n");
		return;
		}
	if (p->flags & WAITING_ENGAGE) {
		tell_player(p, " You already have an offer pending.\n");
		return;
		}
	if (p->tag_flags & NO_PROPOSE) {
		tell_player(p, " not when you yourself are blocking proposals.\n");
		return;
		}
	q = find_player_global(str);
	if (!q) 
		return;
	if (q==p) {
		tell_player(p, " That'd be SOME feat...\n");
		return;
		}
	if (q->system_flags & MARRIED) {
		tell_player(p, " Sorry, that person is already married.\n");
		return;
		}
	if (q->system_flags & ENGAGED) {
		tell_player(p, " Sorry, that person is already engaged.\n");
		return;
		}
	if (q->flags & WAITING_ENGAGE) {
		tell_player(p, " That person already has a standing offer...\n");
		return;
		}
	if (!(q->residency) || q->residency & NO_SYNC) {
		tell_player(p, " Sorry, you cannot be engaged to a non-resident.\n");
		return;
		}
	if (q->flags & NO_PROPOSE) {
		tell_player(p, " That person is blocking proposals...\n");
		return;
		}
	l = fle_from_save(q->saved, p->lower_name);
	/* since this is a ressie command, check that p isn't ignored by q */
	if (l && (l->flags & IGNORE || l->flags & BLOCK)) {
		tell_player(p, " I don't think that person likes you...\n");
		return;
		}
	/* OK, we have two legal people.. lets do it! */
	TELLPLAYER(q," -=*> %s gets down on one knee, takes your hand, \n and asks \"Will you marry me?\"\n (type ACCEPT %s to say yes, or REJECT %s to say no.)\n", p->name, p->lower_name, p->lower_name); 		
	tell_player(p, " You get on your knees and propose...\n");
	strncpy(p->married_to, q->name, MAX_NAME - 3);
	q->flags |= WAITING_ENGAGE;
	p->flags |= WAITING_ENGAGE;
}

/* to agree to be engaged to a person */
void acc_engage(player * p, char *str) {

	player *q;

	FORMAT(str, " Format: accept <player>\n");
	if (!(p->flags & WAITING_ENGAGE)) {
		tell_player(p, " No offer has been made.\n");
		return;
		}
	q = find_player_global(str);
	if (!q) {
		p->flags &= ~WAITING_ENGAGE;
		return;
		}
	if (strcasecmp(q->married_to, p->name)) {
		tell_player(p, " Whoops, wrong person! Typo I assume.. try again\n");
		return;
	} else {
		if (!(q->flags & WAITING_ENGAGE)) {
			tell_player(p, " Odd.. very odd...\n");
			return;
		}
		/* otherwise */
		p->flags &= ~WAITING_ENGAGE;
		q->flags &= ~WAITING_ENGAGE;
		p->system_flags |= ENGAGED;
		q->system_flags |= ENGAGED;
		strncpy(p->married_to, q->name, MAX_NAME - 3);
		TELLPLAYER(q, " %s whispers 'yes, I will marry you!'\n You are now net.engaged to %s.\n", p->name, p->name);
		TELLPLAYER(p, " You are now net.engaged to %s.\n", q->name);
	}
}
		

/* to crush a person's heart */
void reject(player * p, char *str) {

	player *q;

	FORMAT(str, " Format: reject <player>\n");
	if (!(p->flags & WAITING_ENGAGE)) {
		tell_player(p, " No offer has been made.\n");
		return;
		}
	q = find_player_global(str);
	if (!q) {
		p->flags &= ~WAITING_ENGAGE;
		return;
		}
	if (strcasecmp(q->married_to, p->name)) {
		tell_player(p, " Whoops, wrong person! Typo I assume.. try again\n");
		return;
	} else {
		if (!(q->flags & WAITING_ENGAGE)) {
			tell_player(p, " Odd.. very odd...\n");
			return;
		}
		/* otherwise */
		p->flags &= ~WAITING_ENGAGE;
		q->flags &= ~WAITING_ENGAGE;
		strncpy(p->married_to, "", MAX_NAME - 3);
		strncpy(q->married_to, "", MAX_NAME - 3);
		TELLPLAYER(q," %s stares -- \"No, I can't marry you.\"\n", p->name);
		TELLPLAYER(p, " You crush %s's heart.\n", q->name);
	}
}
		
/* and a command to cancel the engagement */
void cancel_engage(player * p, char *str) {

	player *z; /* yes, just to be different */
	
	if (p->system_flags & MARRIED) {
		TELLPLAYER(p, " Isn't a little late to call off the engagement"
			      " since you're already married? Ask a minister"
			      " to grant a divorce, instead.\n");
		return;
		}
	if (!p->system_flags & ENGAGED) {
		TELLPLAYER(p, " You have to be engaged to cancel you're engagement :-P\n");
		if (p->flags & WAITING_ENGAGE)
			TELLPLAYER(p, " Try \"reject\" instead...\n");
		return;
		}
	/* what other checks do I need here? *sigh* */
	z = find_player_global(p->married_to);
	if (z) {
		TELLPLAYER(z, " -=*> %s has just broken off the engagement.\n",
				p->name);
		z->system_flags &= ~(MARRIED|ENGAGED);
		strncpy(z->married_to, "", MAX_NAME - 3);
		save_player(z);
		}
	TELLPLAYER(p, " -=*> You cancel the engagement...\n");
	p->system_flags &= ~(MARRIED|ENGAGED);
	strncpy(p->married_to, "", MAX_NAME - 3);
	save_player(p);
}
		
/* and an admin command to clear fargles... */

void net_anul_all(player *p, char *str) {

	player *f;
	
	FORMAT(str, " Format: anul <player>\n Desc  : Clear all marital status for a player\n"); 

	f = find_player_global(str);
	if (!f)
		return;
	f->system_flags &= ~MARRIED;
	f->system_flags &= ~ENGAGED;
	f->flags &= ~WAITING_ENGAGE;
	strncpy(f->married_to, "", MAX_NAME - 3);
}	



void no_msgs(player *p, char *str)
{
   player *d;
   saved_player *sp;

#ifdef TRACK
   sprintf(functionin,"undumb (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif

   CHECK_DUTY(p);

   FORMAT(str, " Format: nomsg <player>\n");
   d = find_player_global(str);
   if (d)
   {
      if (d == p)
      {
	tell_player(p, " Yeah, that person is a big dork..whoops, thats YOU!\n");
	return;
      }
      if (d->system_flags & NO_MSGS) {
      	d->system_flags &= ~NO_MSGS;
	TELLPLAYER(d, " -=*> %s restores your messages.\n", p->name);
      	TELLPLAYER(p, " You restore messages to %s.\n", d->name);
      	LOGF("msg", " -=*> %s restores messages to %s", p->name, d->name);
	SW_BUT(p, " -=*> %s restores messages to %s\n", p->name, d->name);
	}
      else {
      	d->system_flags |= NO_MSGS;
	TELLPLAYER(d, " -=*> %s removes your messages.\n", p->name);
      	TELLPLAYER(p, " You removes messages from %s.\n", d->name);
      	LOGF("msg", " -=*> %s removes messages from %s", p->name, d->name);
	SW_BUT(p, " -=*> %s removes messages from %s\n", p->name, d->name);
	}
   } else
   {
      tell_player(p, " Checking saved files...\n");
      sp = find_saved_player(str);
      if (!sp)
      {
         tell_player(p, " Not found.\n");
         return;
      }
      if (!(sp->system_flags & NO_MSGS))
      {
      	sp->system_flags |= NO_MSGS;
      	TELLPLAYER(p, " You remove messages from %s.\n", sp->lower_name);
      	LOGF("dumb", " %s removes messages from %s.", p->name, sp->lower_name);
	SW_BUT(p, " -=*> %s removes messages from %s\n", p->name, d->name);
      } else {
      sp->system_flags &= ~NO_MSGS;
      TELLPLAYER(p, " You restores messages to %s.\n", sp->lower_name);
      LOGF("dumb", " %s restores messages to %s.", p->name, sp->lower_name);
      SW_BUT(p, " -=*> %s restores messages to %s\n", p->name, d->name);
      }
   }
}

/* remove sanity completely from the player (and the coders too) */

void fake_nuke_player(player * p, char *str) {
   player *p2;
   char nuked[MAX_NAME] = "";
   char nukee[MAX_NAME] = "";
   char naddr[MAX_INET_ADDR] = "";

#ifdef TRACK
   sprintf(functionin,"nuke_player (%s , SOMETHING)",p->name);
   addfunction(functionin);
#endif


   CHECK_DUTY(p);
   FORMAT(str, " Format: scare <player>\n");
   p2 = find_player_absolute_quiet(str);
   if (!p2) {
      tell_player(p, "No such person on the program.\n");
      return;
	}
   if (!check_privs(p->residency, p2->residency))
      {
         tell_player(p, " You can't scare them !\n");
         TELLPLAYER(p2, " -=*> %s tried to scare you.  Yipes!!.\n", p->name);
         return;
      }
      strcpy(nuked, p2->name);
      strcpy(naddr, p2->inet_addr);
      /* time for a new nuke screen */
      TELLPLAYER(p2, NUKE_SCREEN);
      p->num_ejected++;
      p2->eject_count++;
      quit(p2, 0);
      if (p->gender==PLURAL)
	SUWALL(" -=*> %s scares the shit out of %s, TOASTY!!\n -=*> %s "
		"was from %s\n",p->name, nuked, nuked, naddr);
      else
	SUWALL(" -=*> %s scares the shit out of %s, TOASTY!!\n -=*> %s "
		"was from %s\n",p->name, nuked, nuked, naddr);
}
