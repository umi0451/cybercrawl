/*
 * save and restore routines
 *
 * @(#)save.c	3.9 (Berkeley) 6/16/81
 */

#include "curses.h"
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include "rogue.h"

#define AS_IT_IS(var) { &var, sizeof(var) }
struct as_it_is_var {
	void * variable;
	int size;
} as_it_is[] = {
	AS_IT_IS(rooms), AS_IT_IS(rdes), AS_IT_IS(between), AS_IT_IS(level),
	AS_IT_IS(purse), AS_IT_IS(ntraps), AS_IT_IS(no_move), AS_IT_IS(no_command),
	AS_IT_IS(inpack), AS_IT_IS(max_hp), AS_IT_IS(lastscore), AS_IT_IS(no_food),
	AS_IT_IS(count), AS_IT_IS(fung_hit), AS_IT_IS(quiet), AS_IT_IS(max_level),
	AS_IT_IS(food_left), AS_IT_IS(group), AS_IT_IS(hungry_state),
	AS_IT_IS(whoami), AS_IT_IS(fruit), AS_IT_IS(ch_ret), AS_IT_IS(nh),
	AS_IT_IS(oldpos), AS_IT_IS(delta), AS_IT_IS(traps), AS_IT_IS(huh),
	AS_IT_IS(running), AS_IT_IS(playing), AS_IT_IS(wizard), AS_IT_IS(after),
	AS_IT_IS(notify), AS_IT_IS(fight_flush), AS_IT_IS(terse),
	AS_IT_IS(door_stop), AS_IT_IS(jump), AS_IT_IS(slow_invent),
	AS_IT_IS(firstmove), AS_IT_IS(waswizard), AS_IT_IS(askme), AS_IT_IS(amulet),
	AS_IT_IS(in_shell), AS_IT_IS(take), AS_IT_IS(runch), AS_IT_IS(s_know),
	AS_IT_IS(p_know), AS_IT_IS(r_know), AS_IT_IS(ws_know),
	AS_IT_IS(max_stats),
	{ 0, 0 }
};

write_game(FILE *savef)
{
	struct as_it_is_var * as_var = as_it_is;
	while((as_var++)->variable) {
		fwrite(as_var->variable, 1, as_var->size, savef);
	}
}

read_game(FILE *savef)
{
	struct as_it_is_var * as_var = as_it_is;
	while((as_var++)->variable) {
		fread(as_var->variable, 1, as_var->size, savef);
	}
}


#ifdef VARIABLES_TO_SAVE
// Contains d_func() pointer
delayed_action d_list[MAXDAEMONS]
// Recreate linked_list of thing
struct linked_list *mlist; // contains coord * t_dest and linked_list[object] * t_pack;
// Recreate linked_list of object: contains o_text, o_damage, o_hurldamage
struct linked_list *lvl_obj;		/* List of objects on this level */
// contains coord * t_dest and linked_list[object] * t_pack;
struct thing player;			/* The rogue */

// From player pack.
struct object *cur_weapon;		/* Which weapon he is weilding */
struct object *cur_armor;		/* What a well dresssed rogue wears */
struct object *cur_ring[2];		/* Which rings are being worn */

// Strings. Recreate using new().
char *s_names[MAXSCROLLS];		/* Names of the scrolls */
char *p_colors[MAXPOTIONS];		/* Colors of the potions */
char *r_stones[MAXRINGS];		/* Stone settings of the rings */
char *ws_made[MAXSTICKS];		/* What sticks are made of */
char *s_guess[MAXSCROLLS];		/* Players guess at what scroll is */
char *p_guess[MAXPOTIONS];		/* Players guess at what potion is */
char *r_guess[MAXRINGS];		/* Players guess at what ring is */
char *ws_guess[MAXSTICKS];		/* Players guess at what wand is */
char *ws_type[MAXSTICKS];		/* Is it a wand or a staff */

#endif

typedef struct stat STAT;

extern char version[], encstr[];
//extern char *sys_errlist[], version[], encstr[];
extern bool _endwin;
//extern int errno;

char *sbrk();

STAT sbuf;

save_game()
{
    register FILE *savef;
    register int c;
    char buf[80];

    /*
     * get file name
     */
    mpos = 0;
    if (file_name[0] != '\0')
    {
	msg("Save file (%s)? ", file_name);
	do
	{
	    c = getchar();
	} while (c != 'n' && c != 'N' && c != 'y' && c != 'Y');
	mpos = 0;
	if (c == 'y' || c == 'Y')
	{
	    msg("File name: %s", file_name);
	    goto gotfile;
	}
    }

    do
    {
	msg("File name: ");
	mpos = 0;
	buf[0] = '\0';
	if (get_str(buf, cw) == QUIT)
	{
	    msg("");
	    return FALSE;
	}
	strcpy(file_name, buf);
gotfile:
	if ((savef = fopen(file_name, "w")) == NULL)
	    msg(strerror(errno));	/* fake perror() */
    } while (savef == NULL);

    /*
     * write out encrpyted file (after a stat)
     * The fwrite is to force allocation of the buffer before the write
     */
    save_file(savef);
    return TRUE;
}

/*
 * automatically save a file.  This is used if a HUP signal is
 * recieved
 */
void auto_save()
{
    register FILE *savef;
    register int i;

    for (i = 0; i < NSIG; i++)
	signal(i, SIG_IGN);
    if (file_name[0] != '\0' && (savef = fopen(file_name, "w")) != NULL)
	save_file(savef);
    exit(1);
}

/*
 * write the saved game on the file
 */
save_file(savef)
register FILE *savef;
{
    wmove(cw, lines()-1, 0);
    draw(cw);
    fstat(fileno(savef), &sbuf);
    fwrite("junk", 1, 5, savef);
    fseek(savef, 0L, 0);
	fwrite(version, 1, strlen(version) + 1, savef);
	//encwrite(version, strlen(version) + 1, savef);
    _endwin = TRUE;
	write_game(savef);
    //encwrite(version, sbrk(0) - version, savef);
    fclose(savef);
}

restore(file, envp)
register char *file;
char **envp;
{
    register FILE* inf;
    extern char **environ;
    char buf[80];
    STAT sbuf2;

    if (strcmp(file, "-r") == 0)
	file = file_name;
    if ((inf = fopen(file, "r")) == NULL)
    {
	perror(file);
	return FALSE;
    }

    fflush(stdout);
	fread(buf, 1, strlen(version) + 1, inf);
    //encread(buf, strlen(version) + 1, inf);
    if (strcmp(buf, version) != 0)
    {
	printf("Sorry, saved game is out of date.\n");
	return FALSE;
    }

    fstat(fileno(inf), &sbuf2);
    fflush(stdout);
    //brk(version + sbuf2.st_size);
    //lseek(inf, 0L, 0);
	read_game(inf);
    //encread(version, (unsigned int) sbuf2.st_size, inf);
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

	/* TODO check inode savescumming.
    if (!wizard)
	if (sbuf2.st_ino != sbuf.st_ino || sbuf2.st_dev != sbuf.st_dev)
	{
	    printf("Sorry, saved game is not in the same file.\n");
	    return FALSE;
	}
	else if (sbuf2.st_ctime - sbuf.st_ctime > 300)
	{
	    printf("Sorry, file has been touched.\n");
	    return FALSE;
	}
	*/
    mpos = 0;
    mvwprintw(cw, 0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime));

    /*
     * defeat multiple restarting from the same place
     */
    if (!wizard)
	if (sbuf2.st_nlink != 1)
	{
	    printf("Cannot restore from a linked file\n");
	    return FALSE;
	}
	else if (unlink(file) < 0)
	{
	    printf("Cannot unlink file\n");
	    return FALSE;
	}

    environ = envp;
	/*
    if (!My_term && isatty(2))
    {
	register char	*sp;

	_tty_ch = 2;
	gettmode();
	if ((sp = getenv("TERM")) == NULL)
	    sp = Def_term;
	setterm(sp);
    }
    else
	setterm(Def_term);
	*/
    strcpy(file_name, file);
	seed = getpid();

	// Afterload.
    srand(seed);

    init_things();			/* Set up probabilities of things */
    init_names();			/* Set up names of scrolls */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */
    setup();
    /*
     * Set up windows
     */
    cw = newwin(lines(), cols(), 0, 0);
    mw = newwin(lines(), cols(), 0, 0);
    hw = newwin(lines(), cols(), 0, 0);
    waswizard = wizard;
    clearok(curscr, TRUE);
    touchwin(cw);

    playit();
    /*NOTREACHED*/
}

/*
 * perform an encrypted write
 */
encwrite(start, size, outf)
register char *start;
unsigned int size;
register FILE *outf;
{
    register char *ep;

    ep = encstr;

    while (size--)
    {
	putc(*start++ ^ *ep++, outf);
	if (*ep == '\0')
	    ep = encstr;
    }
}

/*
 * perform an encrypted read
 */
encread(start, size, inf)
register char *start;
unsigned int size;
register int inf;
{
    register char *ep;
    register int read_size;

    if ((read_size = read(inf, start, size)) == -1 || read_size == 0)
	return read_size;

    ep = encstr;

    while (size--)
    {
	*start++ ^= *ep++;
	if (*ep == '\0')
	    ep = encstr;
    }
    return read_size;
}
