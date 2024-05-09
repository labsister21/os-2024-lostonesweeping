#include "./ls.h"
#include "./user-shell.h"
#include "./util.h"




void ls()
{
    for (int i = 1; i < 63; i++)
    {
        if (state.curr_dir.table[i].user_attribute == UATTR_NOT_EMPTY)
        {
            if (state.curr_dir.table[i].name[7] != '\0')
            {
                put_chars(state.curr_dir.table[i].name);
            }
            else
            {
                put_chars(state.curr_dir.table[i].name);
            }
            if (state.curr_dir.table[i].attribute != ATTR_SUBDIRECTORY && strlen(state.curr_dir.table[i].ext) != 0)
            {
                put_chars("." );
                put_chars(state.curr_dir.table[i].ext);
            }
            
            put_char('\n');
        }
    }
    if (state.curr_dir.table[63].user_attribute == UATTR_NOT_EMPTY)
    {
        if (state.curr_dir.table[63].name[7] != '\0')
        {
            put_chars(state.curr_dir.table[63].name);
        }
        else
        {
            put_chars(state.curr_dir.table[63].name);
        }
        if (state.curr_dir.table[63].attribute != ATTR_SUBDIRECTORY && strlen(state.curr_dir.table[63].ext) != 0)
        {
            put_chars(".");
            put_chars(state.curr_dir.table[63].ext);
        }
        put_char('\n');
    }
}