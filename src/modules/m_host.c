/*
Child, Internet Relay Chat Services
Copyright (C) 2005-2020  David Lebrun (target0@geeknode.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include "child.h"
#include "commands.h"
#include "core.h"
#include "core_api.h"
#include "hashmap.h"
#include "logging.h"
#include "modules.h"
#include "net.h"
#include "string_utils.h"
#include "user.h"

#include <stdlib.h>
#include <string.h>

void do_host (Nick *, User *, char *);
void do_help (Nick *, User *, char *);
void host_set (Nick *, User *, char *);
void host_list (Nick *);

void child_init()
{
    addBaseCommand("host",do_host,core_get_config()->level_oper);

    addHostCommand("set",host_set,core_get_config()->level_oper);
    addHostCommand("list",host_list,core_get_config()->level_oper);
}

void child_cleanup()
{
    deleteCommand("host",CMD_BASE,0);

    delHostCommand("set");
    delHostCommand("list");
}

void do_host(Nick *nptr, User *uptr, char *all)
{
    char *arg1,*arg2;
    char blah[1024];
    strncpy(blah,all,1024);

    arg1 = blah;
    arg2 = SeperateWord(arg1);
    SeperateWord(arg2);
    
    if (!arg2 || *arg2 == '\0') {
        NoticeToUser(nptr,"Type \2/msg %s help host\2 for more informations", core_get_config()->nick);
        return;
    }
    
    if (!Strcmp(arg2,"help")) {
        do_help(nptr,uptr,all);
        return;
    }
    
    all = SeperateWord(all);
    all = SeperateWord(all);
    
    Command *cmd;
    LLIST_FOREACH_ENTRY(core_get_commands(), cmd, list_head) {
        if (!Strcmp(cmd->name,arg2) && cmd->type == CMD_HOST) {
            if ((!IsAuthed(uptr) && cmd->level == 0) || (IsAuthed(uptr) && uptr->level >= cmd->level))
                cmd->func(nptr,uptr,all);
            else
                NoticeToUser(nptr,"Access denied");
            return;
        }
    }
    
    NoticeToUser(nptr,"Unknown command");
}

void host_set (Nick *nptr, User *uptr __unused, char *all)
{
    User *uptr2;
    char *arg3,*arg4;
    arg3 = all;
    arg4 = SeperateWord(arg3);
    SeperateWord(arg4);

    if (!arg3 || *arg3 == '\0') {
        NoticeToUser(nptr,"Syntax: \2set \037nick\037 [\037vhost\037]\2");
        return;
    }

    uptr2 = find_user(arg3);
    if (!uptr2) {
        NoticeToUser(nptr,"This user does not exist");
        return;
    }
        
    if (!arg4 || *arg4 == '\0') {
        memset(uptr2->vhost,'\0',HOSTLEN);
        NoticeToUser(nptr,"Vhost for \2%s\2 deleted",arg3);
        operlog("%s removed vhost from %s",nptr->nick,arg3);
        if (get_core_api()->find_nick(arg3))
            get_core_api()->send_raw("SVSMODE %s -x+x",arg3);
    } else {
        strncpy(uptr2->vhost,arg4,HOSTLEN);
        NoticeToUser(nptr,"Vhost for \2%s\2 set to \2%s\2",arg3,arg4);
        operlog("%s set vhost for %s to %s",nptr->nick,arg3,arg4);
        if (get_core_api()->find_nick(arg3))
            get_core_api()->send_raw("CHGHOST %s %s",arg3,arg4);
    }
}

void host_list (Nick *nptr)
{
    User *uptr2;
    struct hashmap_entry *entry;

    HASHMAP_FOREACH_ENTRY_VALUE(core_get_users(), entry, uptr2) {
        if (uptr2->vhost[0] != '\0')
            NoticeToUser(nptr,"%s             %s",uptr2->nick,uptr2->vhost);
    }
    NoticeToUser(nptr,"End of list.");
}
