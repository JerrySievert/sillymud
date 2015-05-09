/*
  SillyMUD Distribution V1.1b             (c) 1993 SillyMUD Developement
 
  See license.doc for distribution terms.   SillyMUD is based on DIKUMUD
*/

#include <stdio.h>

#include "protos.h"

int choose_exit(int in_room, int tgt_room, int dvar);
struct room_data *real_roomp(int);
int remove_trap( struct char_data *ch, struct obj_data *trap);

extern char *dirs[];
extern struct char_data *character_list;
extern struct room_data *world;
extern struct dex_app_type dex_app[];

struct hunting_data {
  char	*name;
  struct char_data **victim;
};


/*************************************/
/* predicates for find_path function */

int is_target_room_p(int room, void *tgt_room);

int named_object_on_ground(int room, void *c_data);

/* predicates for find_path function */
/*************************************/


/*
**  Disarm:
*/

void do_disarm(struct char_data *ch, char *argument, int cmd)
{
  char name[30];
  int percent;
  struct char_data *victim;
  struct obj_data *w, *trap;

  if (!ch->skills) return;

  if (check_peaceful(ch,"You feel too peaceful to contemplate violence.\n\r"))
    return;

  if (!IS_PC(ch) && cmd)
    return;
  
  /*
   *   get victim
   */
  only_argument(argument, name);
    if (!(victim = get_char_room_vis(ch, name))) {
    if (ch->specials.fighting) {
      victim = ch->specials.fighting;
    } else {
      if (!ch->skills[SKILL_REMOVE_TRAP].learned) {
	send_to_char("Disarm who?\n\r", ch);
	return;
      } else {

	if (MOUNTED(ch)) {
	  send_to_char("Yeah... right... while mounted\n\r", ch);
	  return;
	}

	if (!(trap = get_obj_in_list_vis(ch, name, 
		    real_roomp(ch->in_room)->contents))) {
	  if (!(trap = get_obj_in_list_vis(ch, name, ch->carrying))) {
	    send_to_char("Disarm what?\n\r", ch);
	    return;
	  }
	}
	if (trap) {
	  remove_trap(ch, trap);
	  return;
	}
      }
    }
  }
  
  
  if (victim == ch) {
    send_to_char("Aren't we funny today...\n\r", ch);
    return;
  }

  if (victim != ch->specials.fighting) {
    send_to_char("but you aren't fighting them!\n\r", ch);
    return;
  }

  if (ch->attackers > 3) {
    send_to_char("There is no room to disarm!\n\r", ch);
    return;
  }

  if (!HasClass(ch, CLASS_WARRIOR) && !HasClass(ch, CLASS_MONK)) {
    send_to_char("You're no warrior!\n\r", ch);
    return;
  }
  
  /*
   *   make roll - modified by dex && level
   */
  percent=number(1,101); /* 101% is a complete failure */
  
  percent -= dex_app[GET_DEX(ch)].reaction*10;
  percent += dex_app[GET_DEX(victim)].reaction*10;
  if (!ch->equipment[WIELD] && !HasClass(ch, CLASS_MONK)) {
    percent += 50;
  }

  percent += GetMaxLevel(victim);
  if (HasClass(victim, CLASS_MONK))
    percent += GetMaxLevel(victim);

  if (HasClass(ch, CLASS_MONK)) {
    percent -= GetMaxLevel(ch);
  } else {
    percent -= GetMaxLevel(ch)>>1;
  }

  if (percent > ch->skills[SKILL_DISARM].learned) {
    /*
     *   failure.
     */
    act("You try to disarm $N, but fail miserably.", 
	TRUE, ch, 0, victim, TO_CHAR);
    act("$n does a nifty fighting move, but then falls on $s butt.",
	TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POSITION_SITTING;
    if ((IS_NPC(victim)) && (GET_POS(victim) > POSITION_SLEEPING) &&
        (!victim->specials.fighting)) {
      set_fighting(victim, ch);
    }
    LearnFromMistake(ch, SKILL_DISARM, 0, 95);
    WAIT_STATE(ch, PULSE_VIOLENCE*3);
  } else {
    /*
     *  success
     */
    if (victim->equipment[WIELD]) {
      w = unequip_char(victim, WIELD);
      act("$n makes an impressive fighting move.", 
	  TRUE, ch, 0, 0, TO_ROOM);
      act("You send $p flying from $N's grasp.", TRUE, ch, w, victim, 
	  TO_CHAR);
      act("$p flies from your grasp.", TRUE, ch, w, victim, TO_VICT);
/*
  send the object to a nearby room, instead
*/
      obj_to_room(w, victim->in_room);
    } else {
      act("You try to disarm $N, but $E doesn't have a weapon.", 
	  TRUE, ch, 0, victim, TO_CHAR);
      act("$n makes an impressive fighting move, but does little more.",
	  TRUE, ch, 0, 0, TO_ROOM);
    }
    if ((IS_NPC(victim)) && (GET_POS(victim) > POSITION_SLEEPING) &&
        (!victim->specials.fighting)) {
      set_fighting(victim, ch);
    }
    WAIT_STATE(victim, PULSE_VIOLENCE*2);
    WAIT_STATE(ch, PULSE_VIOLENCE*2);
  }  
}


/*
**   Track:
*/

int named_mobile_in_room(int room, struct hunting_data *c_data)
{
  struct char_data	*scan;

  for (scan = real_roomp(room)->people; scan; scan = scan->next_in_room)
    if (isname(c_data->name, scan->player.name)) {
         *(c_data->victim) = scan;
         return 1;
  }
  return 0;
}

void do_track(struct char_data *ch, char *argument, int cmd)
{
   char name[256], buf[256], found=FALSE;
   int dist, code;
  struct hunting_data	huntd;
   struct char_data *scan;
  extern struct char_data  *character_list;

#if NOTRACK
   send_to_char("Sorry, tracking is disabled. Try again after reboot.\n\r",ch);
   return;
#endif

  only_argument(argument, name);

  found = FALSE;
  for (scan = character_list; scan; scan = scan->next)
    if (isname(name, scan->player.name)) {
         found = TRUE;
    }

  if (!found) {
    send_to_char("You are unable to find traces of one.\n\r", ch);
    return;
  }

   if (!ch->skills) 
     dist = 10;
   else
     dist = ch->skills[SKILL_HUNT].learned;


   if (IS_SET(ch->player.class, CLASS_THIEF)) {
     dist *= 3;
   }

   switch(GET_RACE(ch)){
   case RACE_ELVEN:
     dist *= 2;               /* even better */
     break;
   case RACE_DEVIL:
   case RACE_DEMON:
     dist = MAX_ROOMS;   /* as good as can be */
     break;
   default:
     break;
   }

  if (GetMaxLevel(ch) >= IMMORTAL)
    dist = MAX_ROOMS;
 

  if (affected_by_spell(ch, SPELL_MINOR_TRACK)) {
    dist = GetMaxLevel(ch) * 50;
  } else if (affected_by_spell(ch, SPELL_MAJOR_TRACK)){
    dist = GetMaxLevel(ch) * 100;
  }

  if (dist == 0)
    return;
 
  ch->hunt_dist = dist;

  ch->specials.hunting = 0;
  huntd.name = name;
  huntd.victim = &ch->specials.hunting;

  if ((GetMaxLevel(ch) < MIN_GLOB_TRACK_LEV) ||
      (affected_by_spell(ch, SPELL_MINOR_TRACK)) || 
      (affected_by_spell(ch, SPELL_MAJOR_TRACK))) {
     code = find_path( ch->in_room, named_mobile_in_room, &huntd, -dist, 1);
  } else {
     code = find_path( ch->in_room, named_mobile_in_room, &huntd, -dist, 0);
  }
  
   WAIT_STATE(ch, PULSE_VIOLENCE*1);

   if (code == -1) {
    send_to_char("You are unable to find traces of one.\n\r", ch);
     return;
   } else {
     if (IS_LIGHT(ch->in_room)) {
        SET_BIT(ch->specials.act, PLR_HUNTING);
       sprintf(buf, "You see traces of your quarry to the %s\n\r", dirs[code]);
        send_to_char(buf,ch);
      } else {
      ch->specials.hunting = 0;
	send_to_char("It's too dark in here to track...\n\r",ch);
	return;
      }
   }
 }

int track( struct char_data *ch, struct char_data *vict)
{

  char buf[256];
  int code;

  if ((!ch) || (!vict))
    return(-1);

  if ((GetMaxLevel(ch) < MIN_GLOB_TRACK_LEV) || 
      (affected_by_spell(ch, SPELL_MINOR_TRACK)) || 
      (affected_by_spell(ch, SPELL_MAJOR_TRACK))) {
     code = choose_exit_in_zone(ch->in_room, vict->in_room, ch->hunt_dist);
  } else {
     code = choose_exit_global(ch->in_room, vict->in_room, ch->hunt_dist);
  }
  if ((!ch) || (!vict))
    return(-1);


  if (ch->in_room == vict->in_room) {
    send_to_char("##You have found your target!\n\r",ch);
    return(FALSE);  /* false to continue the hunt */
  }
  if (code == -1) {
    send_to_char("##You have lost the trail.\n\r",ch);
    return(FALSE);
  } else {
    sprintf(buf, "##You see a faint trail to the %s\n\r", dirs[code]);
    send_to_char(buf, ch);
    return(TRUE);
  }

}

int dir_track( struct char_data *ch, struct char_data *vict)
{

  char buf[256];
  int code;

  if ((!ch) || (!vict))
    return(-1);

  
  if ((GetMaxLevel(ch) >= MIN_GLOB_TRACK_LEV) ||
      (affected_by_spell(ch, SPELL_MINOR_TRACK)) || 
      (affected_by_spell(ch, SPELL_MAJOR_TRACK))) {
    code = choose_exit_global(ch->in_room, vict->in_room, ch->hunt_dist);
  } else {
    code = choose_exit_in_zone(ch->in_room, vict->in_room, ch->hunt_dist);
  }
  if ((!ch) || (!vict))
    return(-1);

  if (code == -1) {
    if (ch->in_room == vict->in_room) {
      send_to_char("##You have found your target!\n\r",ch);
    } else {
      send_to_char("##You have lost the trail.\n\r",ch);
    }
    return(-1);  /* false to continue the hunt */
  } else {
    sprintf(buf, "##You see a faint trail to the %s\n\r", dirs[code]);
    send_to_char(buf, ch);
    return(code);
  }

}




/** Perform breadth first search on rooms from start (in_room) **/
/** until end (tgt_room) is reached. Then return the correct   **/
/** direction to take from start to reach end.                 **/

/* thoth@manatee.cis.ufl.edu
   if dvar<0 then search THROUGH closed but not locked doors,
   for mobiles that know how to open doors.
 */

#define IS_DIR    (real_roomp(q_head->room_nr)->dir_option[i])
#define GO_OK  (!IS_SET(IS_DIR->exit_info,EX_CLOSED)\
		 && (IS_DIR->to_room != NOWHERE))
#define GO_OK_SMARTER  (!IS_SET(IS_DIR->exit_info,EX_LOCKED)\
		 && (IS_DIR->to_room != NOWHERE))

void donothing()
{
  return;
}

int find_path(int in_room, int (*predicate)(), void *c_data, 
	      int depth, int in_zone)
{
   struct room_q *tmp_q, *q_head, *q_tail;
#if 1
  struct hash_header	x_room;
/*  static struct hash_header	x_room; */
#else
  struct nodes x_room[MAX_ROOMS];
#endif
   int i, tmp_room, count=0, thru_doors;
  struct room_data	*herep, *therep;
  struct room_data      *startp;
  struct room_direction_data	*exitp;

	/* If start = destination we are done */
   if ((predicate)(in_room, c_data))
     return -1;

#if 0
   if (top_of_world > MAX_ROOMS) {
     log("TRACK Is disabled, too many rooms.\n\rContact Loki soon.\n\r");
    return -1;
   }
#endif

   if (depth<0) {
     thru_doors = TRUE;
     depth = - depth;
   } else {
     thru_doors = FALSE;
   }

  startp = real_roomp(in_room);

  init_hash_table(&x_room, sizeof(int), 2048);
  hash_enter(&x_room, in_room, (void*)-1);

	   /* initialize queue */
   q_head = (struct room_q *) malloc(sizeof(struct room_q));
   q_tail = q_head;
   q_tail->room_nr = in_room;
   q_tail->next_q = 0;

  while(q_head) {
    herep = real_roomp(q_head->room_nr);
		/* for each room test all directions */
    if (herep->zone == startp->zone || !in_zone) {  
                                           /* only look in this zone.. 
					      saves cpu time.  makes world
					      safer for players
					      */
      for(i = 0; i <= 5; i++) {
        exitp = herep->dir_option[i];
        if (exit_ok(exitp, &therep) && (thru_doors ? GO_OK_SMARTER : GO_OK)) {
	  /* next room */
	  tmp_room = herep->dir_option[i]->to_room;
	  if(!((predicate)(tmp_room, c_data))) {
	    /* shall we add room to queue ? */
	    /* count determines total breadth and depth */
	    if(!hash_find(&x_room,tmp_room) && (count < depth)
	       && !IS_SET(RM_FLAGS(tmp_room),DEATH)) {
	      count++;
	      /* mark room as visted and put on queue */
	      
	      tmp_q = (struct room_q *) malloc(sizeof(struct room_q));
	      tmp_q->room_nr = tmp_room;
	      tmp_q->next_q = 0;
	      q_tail->next_q = tmp_q;
	      q_tail = tmp_q;
	      
	      /* ancestor for first layer is the direction */
	      hash_enter(&x_room, tmp_room,
			 ((int)hash_find(&x_room,q_head->room_nr) == -1) ?
			 (void*)(i+1) : hash_find(&x_room,q_head->room_nr));
	    }
	  } else {
	    /* have reached our goal so free queue */
	    tmp_room = q_head->room_nr;
	    for(;q_head;q_head = tmp_q)   {
	      tmp_q = q_head->next_q;
	      free(q_head);
	    }
	    /* return direction if first layer */
	    if ((int)hash_find(&x_room,tmp_room)==-1) {
              if (x_room.buckets) { /* junk left over from a previous track */
		destroy_hash_table(&x_room, donothing);
              }
	      return(i);
	    } else {  /* else return the ancestor */
	      int i;
	      
              i = (int)hash_find(&x_room,tmp_room);
              if (x_room.buckets) { /* junk left over from a previous track */
		destroy_hash_table(&x_room, donothing);
              }
	      return( -1+i);
	    }
	  }
	}
      }
    }
  
      /* free queue head and point to next entry */
      tmp_q = q_head->next_q;
      free(q_head);
      q_head = tmp_q;
   }
   /* couldn't find path */
   if (x_room.buckets) { /* junk left over from a previous track */
      destroy_hash_table(&x_room, donothing);
   } 
   return(-1);

}


int choose_exit_global(int in_room, int tgt_room, int depth)
{
  return find_path(in_room, is_target_room_p, (void*)tgt_room, depth, 0);
}

int choose_exit_in_zone(int in_room, int tgt_room, int depth)
{
  return find_path(in_room, is_target_room_p, (void*)tgt_room, depth, 1);
}

int go_direction(struct char_data *ch, int dir)     
{
  if (ch->specials.fighting)
    return 0;
  
  if (!IS_SET(EXIT(ch,dir)->exit_info, EX_CLOSED)) {
    do_move(ch, "", dir+1);
  } else if ( IsHumanoid(ch) && !IS_SET(EXIT(ch,dir)->exit_info, EX_LOCKED) ) {
    open_door(ch, dir);
    return 0;
  }
}


void slam_into_wall( struct char_data *ch, struct room_direction_data *exitp)
{
  char doorname[128];
  char buf[256];
  
  if (exitp->keyword && *exitp->keyword) {
    if ((strcmp(fname(exitp->keyword), "secret")==0) ||
	(IS_SET(exitp->exit_info, EX_SECRET))) {
      strcpy(doorname, "wall");
    } else {
      strcpy(doorname, fname(exitp->keyword));
    }
  } else {
    strcpy(doorname, "barrier");
  }
  sprintf(buf, "You slam against the %s with no effect\n\r", doorname);
  send_to_char(buf, ch);
  send_to_char("OUCH!  That REALLY Hurt!\n\r", ch);
  sprintf(buf, "$n crashes against the %s with no effect\n\r", doorname);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  GET_HIT(ch) -= number(1, 10)*2;
  if (GET_HIT(ch) < 0)
    GET_HIT(ch) = 0;
  GET_POS(ch) = POSITION_STUNNED;
  return;
}


/*
  skill to allow fighters to break down doors
*/
void do_doorbash( struct char_data *ch, char *arg, int cmd)
{
  extern char *dirs[];
  int dir;
  int ok;
  struct room_direction_data *exitp;
  int was_in, roll;

  char buf[256], type[128], direction[128];

  if (GET_MOVE(ch) < 10) {
    send_to_char("You're too tired to do that\n\r", ch);
    return;
  }

  if (MOUNTED(ch)) {
    send_to_char("Yeah... right... while mounted\n\r", ch);
    return;
  }

  /*
    make sure that the argument is a direction, or a keyword.
  */

  for (;*arg == ' '; arg++);

  argument_interpreter(arg, type, direction);

  if ((dir = find_door(ch, type, direction)) >= 0) {
    ok = TRUE;
  } else {
    act("$n looks around, bewildered.", FALSE, ch, 0, 0, TO_ROOM);
    return;
  }

  if (!ok) {
    send_to_char("Hmm, you shouldn't have gotten this far\n\r", ch);
    return;
  }

  exitp = EXIT(ch, dir);
  if (!exitp) {
    send_to_char("you shouldn't have gotten here.\n\r", ch);
    return;
  }

  if (dir == UP) {
    if (real_roomp(exitp->to_room)->sector_type == SECT_AIR &&
	!IS_AFFECTED(ch, AFF_FLYING)) {
      send_to_char("You have no way of getting there!\n\r", ch);
      return;
    }
  }
  
  sprintf(buf, "$n charges %swards", dirs[dir]);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  sprintf(buf, "You charge %swards\n\r", dirs[dir]);
  send_to_char(buf, ch);

  if (!IS_SET(exitp->exit_info, EX_CLOSED)) {
    was_in = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, exitp->to_room);
    do_look(ch, "", 0);

    DisplayMove(ch, dir, was_in, 1);
    if (!check_falling(ch)) {
      if (IS_SET(RM_FLAGS(ch->in_room), DEATH) && 
	  GetMaxLevel(ch) < LOW_IMMORTAL) {
	NailThisSucker(ch);
	return;
      } else {
	WAIT_STATE(ch, PULSE_VIOLENCE*3);
	GET_MOVE(ch) -= 10;
      }
    } else {
      return;
    }
    WAIT_STATE(ch, PULSE_VIOLENCE*3);
    GET_MOVE(ch) -= 10;
    return;
  }

  GET_MOVE(ch) -= 10;

  if (IS_SET(exitp->exit_info, EX_LOCKED) &&
      IS_SET(exitp->exit_info, EX_PICKPROOF)) {
    slam_into_wall(ch, exitp);
    return;
  }

  /*
    now we've checked for failures, time to check for success;
    */
  if (ch->skills) {
    if (ch->skills[SKILL_DOORBASH].learned) {
      roll = number(1, 100);
      if (roll > ch->skills[SKILL_DOORBASH].learned) {
	slam_into_wall(ch, exitp);
	LearnFromMistake(ch, SKILL_DOORBASH, 0, 95);
      } else {
	/*
	  unlock and open the door
	  */
	sprintf(buf, "$n slams into the %s, and it bursts open!", 
		fname(exitp->keyword));
	act(buf, FALSE, ch, 0, 0, TO_ROOM);
	sprintf(buf, "You slam into the %s, and it bursts open!\n\r", 
		fname(exitp->keyword));
	send_to_char(buf, ch);
	raw_unlock_door(ch, exitp, dir);
	raw_open_door(ch, dir);
	GET_HIT(ch) -= number(1,5);
	/*
	  Now a dex check to keep from flying into the next room
	  */
	roll = number(1, 20);
	if (roll > GET_DEX(ch)) {
	  was_in = ch->in_room;

	  char_from_room(ch);
	  char_to_room(ch, exitp->to_room);
	  do_look(ch, "", 0);
	  DisplayMove(ch, dir, was_in, 1);
	  if (!check_falling(ch)) {
	    if (IS_SET(RM_FLAGS(ch->in_room), DEATH) && 
		GetMaxLevel(ch) < LOW_IMMORTAL) {
	      NailThisSucker(ch);
	      return;
	    }
	  } else {
	    return;
	  }
	  WAIT_STATE(ch, PULSE_VIOLENCE*3);
	  GET_MOVE(ch) -= 10;
	  return;	  
	} else {
	  WAIT_STATE(ch, PULSE_VIOLENCE*1);
	  GET_MOVE(ch) -= 5;
	  return;
	}
      }
    } else {
      send_to_char("You just don't know the nuances of door-bashing.\n\r", ch);
      slam_into_wall(ch, exitp);
      return;
    }
  } else {
    send_to_char("You're just a goofy mob.\n\r", ch);
    return;
  }
}

/*
  skill to allow anyone to move through rivers and underwater
*/

void do_swim( struct char_data *ch, char *arg, int cmd)
{

  struct affected_type af;
  byte percent;
  

  send_to_char("Ok, you'll try to swim for a while.\n\r", ch);

  if (IS_AFFECTED(ch, AFF_WATERBREATH)) {
    /* kinda pointless if they don't need to...*/
    return;
  }
  
  if (affected_by_spell(ch, SKILL_SWIM)) {
    send_to_char("You're too exhausted to swim right now\n", ch);
    return;
  }

  percent=number(1,101); /* 101% is a complete failure */

  if (!ch->skills)
    return;
  
  if (percent > ch->skills[SKILL_SWIM].learned) {
    send_to_char("You're too afraid to enter the water\n\r",ch);
    if (ch->skills[SKILL_SWIM].learned < 95 &&
	ch->skills[SKILL_SWIM].learned > 0) {
      if (number(1,101) > ch->skills[SKILL_SWIM].learned) {
	send_to_char("You feel a bit braver, though\n\r", ch);
	ch->skills[SKILL_SWIM].learned++;
      }
    }
    return;
  }
 
  af.type = SKILL_SWIM;
  af.duration = (ch->skills[SKILL_SWIM].learned/10)+1;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_WATERBREATH;
  affect_to_char(ch, &af);

  af.type = SKILL_SWIM;
  af.duration = 13;
  af.modifier = -10;
  af.location = APPLY_MOVE;
  af.bitvector = 0;
  affect_to_char(ch, &af);

}


int SpyCheck(struct char_data *ch)
{

  if (!ch->skills) return(FALSE);

  if (number(1,101) > ch->skills[SKILL_SPY].learned)
    return(FALSE);

  return(TRUE);

}

void do_spy( struct char_data *ch, char *arg, int cmd)
{

  struct affected_type af;
  byte percent;
  

  send_to_char("Ok, you'll try to act like a secret agent\n\r", ch);

  if (IS_AFFECTED(ch, AFF_SCRYING)) {
    /* kinda pointless if they don't need to...*/
    return;
  }
  
  if (affected_by_spell(ch, SKILL_SPY)) {
    send_to_char("You're already acting like a secret agent\n", ch);
    return;
  }

  percent=number(1,101); /* 101% is a complete failure */

  if (!ch->skills)
    return;
  
  if (percent > ch->skills[SKILL_SPY].learned) {
    if (ch->skills[SKILL_SPY].learned < 95 &&
	ch->skills[SKILL_SPY].learned > 0) {
      if (number(1,101) > ch->skills[SKILL_SPY].learned) {
	ch->skills[SKILL_SPY].learned++;
      }
    }
    af.type = SKILL_SPY;
    af.duration = (ch->skills[SKILL_SPY].learned/10)+1;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);    return;
  }
 
  af.type = SKILL_SPY;
  af.duration = (ch->skills[SKILL_SPY].learned/10)+1;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = AFF_SCRYING;
  affect_to_char(ch, &af);    return;
}

int remove_trap( struct char_data *ch, struct obj_data *trap)
{
  int num;

  if (ITEM_TYPE(trap) != ITEM_TRAP) {
    send_to_char("That's no trap!\n\r", ch);
    return(FALSE);
  }
  if (GET_TRAP_CHARGES(trap) <= 0) {
    send_to_char("That trap is already sprung!\n\r", ch);
    return(FALSE);
  }
  num = number(1,101);
  if (num < ch->skills[SKILL_REMOVE_TRAP].learned) {
    send_to_char("<Click>\n\r", ch);
    act("$n disarms $p", FALSE, ch, trap, 0, TO_ROOM);
    GET_TRAP_CHARGES(trap) = 0;
    return(TRUE);
  } else {
    send_to_char("<Click>\n\r(uh oh)\n\r", ch);
    act("$n attempts to disarm $p", FALSE, ch, trap, 0, TO_ROOM);
    TriggerTrap(ch, trap);
    return(TRUE);
  }
}

void do_feign_death( struct char_data *ch, char *arg, int cmd)
{
  struct room_data *rp;
  struct char_data *t;

  if (!ch->skills)
    return;

  if (!ch->specials.fighting) {
    send_to_char("But you are not fighting anything...\n\r", ch);
    return;
  }
  
  if (!HasClass(ch, CLASS_MONK)) {
    send_to_char("You're no monk!\n\r", ch);
    return;
  }

  if (MOUNTED(ch)) {
    send_to_char("Yeah... right... while mounted\n\r", ch);
    return;
  }

  rp = real_roomp(ch->in_room);
  if (!rp)
    return;

  send_to_char("You try to fake your own demise\n\r", ch);

  death_cry(ch);
  act("$n is dead! R.I.P.", FALSE, ch, 0, 0, TO_ROOM);

  if (number(1,101) < ch->skills[SKILL_FEIGN_DEATH].learned) {
    stop_fighting(ch);
    for (t = rp->people;t;t=t->next_in_room) {
      if (t->specials.fighting == ch) {
	stop_fighting(t);
	if (number(1,101) < ch->skills[SKILL_FEIGN_DEATH].learned/2)
	  SET_BIT(ch->specials.affected_by, AFF_HIDE);
	GET_POS(ch) = POSITION_SLEEPING;
      }
    }
    WAIT_STATE(ch, PULSE_VIOLENCE*2);
    return;
  } else {
    GET_POS(ch) = POSITION_SLEEPING;
    WAIT_STATE(ch, PULSE_VIOLENCE*3);
    if (ch->skills[SKILL_FEIGN_DEATH].learned < 95 &&
	ch->skills[SKILL_FEIGN_DEATH].learned > 0) {
      if (number(1,101) > ch->skills[SKILL_FEIGN_DEATH].learned) {
	ch->skills[SKILL_FEIGN_DEATH].learned++;
      }
    }
  }
}


void do_first_aid( struct char_data *ch, char *arg, int cmd)
{
  struct affected_type af;
    
  send_to_char("You attempt to render first aid unto yourself\n\r", ch);

  if (affected_by_spell(ch, SKILL_FIRST_AID)) {
    send_to_char("You can only do this once per day\n\r", ch);
    return;
  }

  if (number(1,101) < ch->skills[SKILL_FIRST_AID].learned) {
    GET_HIT(ch)+= number(1,4)+GET_LEVEL(ch, MONK_LEVEL_IND);
    if(GET_HIT(ch) > GET_MAX_HIT(ch))
       GET_HIT(ch) = GET_MAX_HIT(ch);

    af.duration = 24;
  } else {
    af.duration = 6;
    if (ch->skills[SKILL_FIRST_AID].learned < 95 &&
	ch->skills[SKILL_FIRST_AID].learned > 0) {
      if (number(1,101) > ch->skills[SKILL_FIRST_AID].learned) {
	ch->skills[SKILL_FIRST_AID].learned++;
      }
    }    
  }

  af.type = SKILL_FIRST_AID;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = 0;
  affect_to_char(ch, &af);    
  return;  
}


void do_disguise(struct char_data *ch, char *argument, int cmd)
{
  struct affected_type af;
    
  send_to_char("You attempt to disguise yourself\n\r", ch);

  if (affected_by_spell(ch, SKILL_DISGUISE)) {
    send_to_char("You can only do this once per day\n\r", ch);
    return;
  }

  if (number(1,101) < ch->skills[SKILL_DISGUISE].learned) {
    struct char_data *k;

    for (k=character_list; k; k=k->next) {
      if (k->specials.hunting == ch) {
	k->specials.hunting = 0;
      }
      if (number(1,101) < ch->skills[SKILL_DISGUISE].learned) {
	if (Hates(k, ch)) {
	  ZeroHatred(k, ch);
	}
	if (Fears(k, ch)) {
	  ZeroFeared(k, ch);
	}
      }
    }
  } else {
    if (ch->skills[SKILL_DISGUISE].learned < 95 &&
	ch->skills[SKILL_DISGUISE].learned > 0) {
      if (number(1,101) > ch->skills[SKILL_DISGUISE].learned) {
	ch->skills[SKILL_DISGUISE].learned++;
      }
    }    
  }

  af.type = SKILL_DISGUISE;
  af.duration = 24;
  af.modifier = 0;
  af.location = APPLY_NONE;
  af.bitvector = 0;
  affect_to_char(ch, &af);    
  return;  
}

/* Skill for climbing walls and the like -DM */
void do_climb( struct char_data *ch, char *arg, int cmd)
{
  extern char *dirs[];
  int dir;
  struct room_direction_data *exitp;
  int was_in, roll;
  extern char *dirs[];

  char buf[256], type[128], direction[128];

  if (GET_MOVE(ch) < 10) {
    send_to_char("You're too tired to do that\n\r", ch);
    return;
  }

  if (MOUNTED(ch)) {
    send_to_char("Yeah... right... while mounted\n\r", ch);
    return;
  }

  /*
    make sure that the argument is a direction, or a keyword.
  */

  for (;*arg == ' '; arg++);

  only_argument(arg,direction);

  if ((dir = search_block(direction, dirs, FALSE)) < 0) {
    send_to_char("You can't climb that way.\n\r", ch);
    return;
  }


  exitp = EXIT(ch, dir);
  if (!exitp) {
    send_to_char("You can't climb that way.\n\r", ch);
    return;
  }

  if(!IS_SET(exitp->exit_info, EX_CLIMB)) {
    send_to_char("You can't climb that way.\n\r", ch);
    return;
  }

  if (dir == UP) {
    if (real_roomp(exitp->to_room)->sector_type == SECT_AIR &&
	!IS_AFFECTED(ch, AFF_FLYING)) {
      send_to_char("You have no way of getting there!\n\r", ch);
      return;
    }
  }

  if (IS_SET(exitp->exit_info, EX_ISDOOR) &&
      IS_SET(exitp->exit_info, EX_CLOSED)) {
    send_to_char("You can't climb that way.\n\r", ch);
    return;
  }

  sprintf(buf, "$n attempts to climb %swards", dirs[dir]);
  act(buf, FALSE, ch, 0, 0, TO_ROOM);
  sprintf(buf, "You attempt to climb %swards\n\r", dirs[dir]);
  send_to_char(buf, ch);

  GET_MOVE(ch) -= 10;

  /*
    now we've checked for failures, time to check for success;
    */
  if (ch->skills) {
    if (ch->skills[SKILL_CLIMB].learned) {
      roll = number(1, 100);
      if (roll > ch->skills[SKILL_CLIMB].learned) {
	slip_in_climb(ch, dir, exitp->to_room);
	LearnFromMistake(ch, SKILL_CLIMB, 0, 95);
      } else {

	  was_in = ch->in_room;

	  char_from_room(ch);
	  char_to_room(ch, exitp->to_room);
	  do_look(ch, "", 0);
	  DisplayMove(ch, dir, was_in, 1);
	  if (!check_falling(ch)) {
	    if (IS_SET(RM_FLAGS(ch->in_room), DEATH) && 
		GetMaxLevel(ch) < LOW_IMMORTAL) {
	      NailThisSucker(ch);
	      return;
	    }

	  }
	  WAIT_STATE(ch, PULSE_VIOLENCE*3);
	  GET_MOVE(ch) -= 10;
	  return;	  
	}
      }
     else {
      send_to_char("You just don't know the nuances of climbing.\n\r", ch);
      slip_in_climb(ch, dir, exitp->to_room);
      return;
    }
  } else {
    send_to_char("You're just a goofy mob.\n\r", ch);
    return;
  }
}


void slip_in_climb(struct char_data *ch, int dir, int room)
{
 int i;

 i = number(1, 6);

 if(dir != DOWN) {
   act("$n falls down and goes splut.", FALSE, ch, 0, 0, TO_ROOM);
   send_to_char("You fall.\n\r", ch);
 }

 else {
   act("$n loses $s grip and falls further down.", FALSE, ch, 0, 0, TO_ROOM);
   send_to_char("You slip and start to fall.\n\r", ch);
   i += number(1, 6);
   char_from_room(ch);
   char_to_room(ch, room);
   do_look(ch, "", 0);
 }

 GET_POS(ch) = POSITION_SITTING;
 if(i > GET_HIT(ch))
   GET_HIT(ch) = 1;
 else
   GET_HIT(ch) -= i;
}
