#include "../include/vshcfp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLEN 1024

static void init();
void        analize(HcfOpts, char *);
void        execute(HcfOpts, char *);
char       *extend_get(HcfOpts, char *, char *);

/* Globals */
char *current_field;


char *
extend_get(HcfOpts opts, char *default_field, char *extend_key)
{
    char *c;

    c = strchr(extend_key, '.');

    if (!c)
        return hcf_get(opts, default_field, extend_key);

    *c = '\0';

    return hcf_get(opts, extend_key, c + 1);
}

void
execute(HcfOpts opts, char *seq)
{
    char  buf[MAXLEN] = { 0 };
    char *key_intro;
    char *key_sep;
    char *value;

    if (!seq)
    {
        printf("Invalid sequence!\n");
        return;
    }

    key_sep = seq;
    while (key_sep && (key_intro = strchr(seq, '$')))
    {
        /* Remove the $ and append all the text before it to buf */
        *key_intro = '\0';
        strcat(buf, seq);

        /* Using $(...) varname matching */
        if (*++key_intro == '(')
        {
            /* Find the next ")" (end of varname) */
            key_sep = strchr(key_intro, ')');
            if (key_sep)
                *key_sep = '\0';
            /* If it's null, the key is the last thing in the string */
        }

        else
        {
            /* Find the next " " (end of varname) */
            key_sep = strchr(key_intro, ' ');
            if (key_sep)
                *key_sep = '\0';
            /* If it's null, the key is the last thing in the string */
        }

        /* Getting value from current field */
        if ((value = extend_get(opts, current_field, key_intro)))
            strcat(buf, value);

        /* Getting from default field */
        else if ((value = extend_get(opts, "default", key_intro)))
            strcat(buf, value);

        else
            printf("Can not found %s\n", key_intro);

        /* Appending a " " just to avoid overlapping if using " " sep */
        if (*key_intro == '(')
            strcat(buf, " ");

        /* Move seq to the next character after the next ' ' */
        seq = key_sep;
    }

    /* If there are text remaining that is not in the buffer */
    if (key_sep && *seq)
        strcat(buf, seq);

    /* Execute the commands in buffer */
    printf("> exec: %s\n", buf);
    system(buf);
}

void
analize(HcfOpts opts, char *exec_seq)
{
    char *value_start;
    char *cur_seq;
    char *temp;
    char *c;

    /* Initialize the value start pointer */
    value_start = exec_seq;

    while (value_start && *value_start)
    {
        /* Cut the string at the next ' ' */
        c = strchr(value_start, ' ');
        if (c)
            *c = '\0';

        /* Find for the current entry in the exec sequence */
        cur_seq = extend_get(opts, current_field, value_start);

        if (cur_seq)
            /* If it is found, execute the sequence in this function */
            execute(opts, cur_seq);

        /* If it is a field name, execute its exec */
        else if (hcf_get_field(opts, current_field))
        {
            /* Change the entry field to make it feel as
             * the default entry */
            temp          = current_field;
            current_field = value_start;

            /* Call analize recursively from the value exec entry */
            analize(opts, extend_get(opts, value_start, "exec"));

            /* Restore entry field */
            current_field = temp;
        }

        else
            printf("Invalid entry %s\n", value_start);


        /* Remove remaining spaces */
        if (c)
            do
            {
                ++c;
            } while (*c == ' ' || *c == '\t');

        /* Move the value_start pointer to the first char after
         * the spaces */
        value_start = c;
    }
}


static void
init()
{
    HcfOpts opts;
    char   *exec_seq;

    /* Load the default file. If it is not
     * found this function print a brief
     * error msg */
    opts = hcf_load("build.hcf");

    /* This is quite weird but I dont
     * implement error checking */
    if (opts.node_arr == NULL)
        /* Opts were not created */
        exit(1);

    /* Find the first entry point */
    if (!(exec_seq = hcf_get(opts, current_field, "exec")))
    {
        printf("Can not find entry point %s.exec!\n", current_field);
        hcf_destroy(&opts);
        exit(1);
    }

    /* This function call recursively so it finally end by executing
     * all the needed instructions */
    analize(opts, exec_seq);

    hcf_destroy(&opts);
}


int
main(int argc, char *argv[])
{
    if (argc == 2)
        current_field = argv[1];
    else
        current_field = "default";

    init();

    return 0;
}
