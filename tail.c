#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include "lab.h"

#define BUFFSIZE 4096

#define ll long long

/* try to parse the argument */
void tryParse(char *arg);

/* write output from stdin */
void write_from_stdin();

/* write output from file */
void write_from_file(int fd);

/* flags for options */
bool cFlag = false;
bool nFlag = false;

ll line_number = 10;
ll char_number = 0;
unsigned char buf[BUFFSIZE];

int main(int argc, char **argv)
{
    struct stat status;
    int fd;
    int option;
    char *arg, *err;          /* arg for the argument of -n option and err for the error string */
    bool first_print = false; /* check if current file is first printed in order to write or not a newline char before header */
    bool all_wrong = true;    /* if all files generate errors then exit with EXIT_FAILURE status */

    while( (option=getopt(argc, argv, "n:c:")) != -1)
        switch(option)
        {
            case 'n':
                nFlag = true;
                cFlag = false;
                arg = optarg;
                tryParse(arg);
                break;
            case 'c':
                cFlag = true;
                nFlag = false;
                arg = optarg;
                tryParse(arg);
                break;
            case '?':
                err = "Try 'tail --help' for more information.\n";

                if(write(STDERR_FILENO,err,strlen(err)) < 0)
                    err_sys("write error");

                exit(EXIT_FAILURE);
                break;
        }

    /* exit with status 0 if number of line equal 0 */
    if((line_number == 0 && nFlag) || (char_number == 0 && cFlag))
        exit(0);

    /* if no file is given enter to standard input */
    if(argc - optind == 0)
    {
        write_from_stdin();
        exit(0);
    }

    /* if arguments are given then iterate over all the arguments */
    if(argc - optind > 0)
        for(int files = optind ; files < argc ; files ++)
        {
            /* if "-" argument is given then check if header is needed and enter to standard input */
            if(strcmp(argv[files],"-") == 0)
            {
                if(argc - optind > 1)
                {
                    char *std = "==> standard input <==\n";
                    if(first_print == true)
                        if(write(STDOUT_FILENO,"\n",1) < 0)
                            err_sys("write error");

                    if(write(STDOUT_FILENO,std,strlen(std)) < 0)
                        err_sys("write error");
                }

                write_from_stdin();
                first_print = true;
            }
            else
            {
                char *header = malloc(strlen(argv[files])*5);
                snprintf(header,strlen(argv[files])*5,"==> %s <==\n",argv[files]);

                if(lstat(argv[files],&status) < 0)
                    status.st_mode = 0;

                if((fd = open(argv[files], O_RDONLY)) < 0)
                    err_ret("cannot open '%s' for reading",argv[files]);
                else
                    if(read(fd,buf,0) < 0)
                    {
                        if(argc - optind > 1)
                        {
                            if(first_print == true)
                                if(write(STDOUT_FILENO,"\n",1) < 0)
                                    err_sys("write error");
                            
                            if(write(STDOUT_FILENO,header,strlen(header)) < 0)
                                err_sys("write error");

                            first_print = true;
                        }
                        
                        err_ret("error reading '%s'",argv[files]);
                    }

                if(fd > 0 && !S_ISDIR(status.st_mode))
                {
                    if(argc - optind > 1)
                    {
                        if(first_print == true)
                            if(write(STDOUT_FILENO,"\n",1) < 0)
                                err_sys("write error");

                        if(write(STDOUT_FILENO,header,strlen(header)) < 0)
                            err_sys("write error");
                    }

                    write_from_file(fd);
                    close(fd);
                    all_wrong = false;
                    first_print = true;
                }

                free(header);
            }
        }

    if(all_wrong == false)
        exit(0);
    else
        exit(EXIT_FAILURE);
}

void tryParse(char *arg)
{
    char *errstr;
    ll number = strtol(arg,&errstr,10);
    
    if(errstr[0] == '\0')
    {

        if(number < 0)
            number = -number;

        if(nFlag)
            line_number = number;

        if(cFlag)
            char_number = number;
    }
    else
    {
        if(arg[0] == '-')
            arg++;

        if(nFlag)
            err_msg("invalid number of lines: ‘%s’",arg);

        if(cFlag)
            err_msg("invalid number of bytes: ‘%s’",arg);

        exit(EXIT_FAILURE);
    }
}

void write_from_stdin()
{
    /* -c option needs other implementation than -n option OR standard because it is possible 
     * that in 10 lines which is standard to be less than `char_number` characters */
    if(cFlag)
    {
        int bytes = 0;
        int index = 0;

        char *line = (char*) malloc(char_number*sizeof(char));

        while((bytes = read(STDIN_FILENO,buf,BUFFSIZE)) > 0)
        {
            for(int i = 0; i < bytes ;i++)
            {
                line[index % char_number] = buf[i];
                index++;
            }

            for(int k=0 ; k < BUFFSIZE; k++)
                buf[k] = '\0';
        }

        int number = (index > char_number) ? char_number : index;

        char *write_line = malloc(number * sizeof(char));

        for(int ind = 0; ind < number; ind++)
        {
            write_line[ind] = line[index % number];
            index ++;
        }
        if(write(STDOUT_FILENO,write_line,number) < 0)
            err_sys("write  error");
    
        free(line);
        free(write_line);
    }
    
    if(nFlag || !cFlag)
    {
        char *lines[line_number];
        int i;
        ll lines_size[line_number];
        ll size[line_number];

        for(i = 0; i < line_number; i++)
        {
            lines[i] =(char*) malloc(BUFFSIZE*sizeof(char*));
            size[i] = BUFFSIZE*sizeof(char*);
            lines_size[i] = 0;
        }

        i = 0;

        bool if_reset = false; /* used to check if counter is reseted */
        bool same_line = true; /* used to check if we are on same line or a different one */
        int line_index = 0;    /* used to access every char from a line */
        ll bytes_readed = 0;
    

        while((bytes_readed = read(STDIN_FILENO,buf,BUFFSIZE)) > 0)
        {
            for(int index = 0; index < bytes_readed; index++)
            {
                if(if_reset  && !same_line)
                {
                    free(lines[i]);
                    lines[i] = (char*) malloc(BUFFSIZE*sizeof(char*));
                    size[i] = BUFFSIZE*sizeof(char*);
                    lines_size[i] = 0;
                    same_line = true;
                }

                if(lines_size[i] == size[i])
                {
                    char *tmp = (char*) realloc(lines[i],size[i]+BUFFSIZE);
                    if(tmp)
                        lines[i] = tmp;
                    size[i] = size[i]+BUFFSIZE;
                }

                /* append each char from buffer to current line and increase line_index */
                lines[i][line_index] = buf[index];
                lines_size[i] ++;
                line_index ++;

                /* if newline char is read then increase line number , set same_line on false ,
                * reset counter to 1 , reset line_index to 0 and append null char to the 
                * current line */
                if(buf[index] == '\n')
                {
                    line_index = 0;
                    i++;
                    same_line = false;
                    if(i ==line_number)
                    {
                        i=0;
                        if_reset = true;
                    }
                }   
            }
            /* after itterating over all char from buf , in order to be sure that no chars from previous reading
            * will remain in buffer , we need to reset buf */
            for(int index = 0; index < BUFFSIZE; index++)
                buf[index] = '\0';
        }
        
        /* print the last "line_number" lines by keep a counter "nr" for the number of lines to be printed
        * and print each line in cyclic mode and after a line is printed , free the memory from line */
        for(int nr = 0; nr < line_number; nr++)
        {
            if(write(STDOUT_FILENO,lines[i % line_number],lines_size[i % line_number]) < 0)
                err_sys("write error");

            free(lines[i % line_number]);
            i++;
        }

    }

}

void write_from_file(int fd)
{
    /* Some explanations about what i am doing here
     * Firstly move cursor to the end of file
     * While current position of cursor is bigger than 0
     * Save current position of cursor in "val" and check if val is bigger or lower than BUFFSIZE
     * If "val" is lower save "val" in "counts" and move cursor with -val offset
     * Otherwise save "BUFFSIZE" in "counts" and move cursor with -BUFFSIZE offset */

    
    size_t current = lseek(fd,0,SEEK_END); /* current position of cursor (now is at end of file) */
    int counter = 0;                       /* count the number of lines */
    int val = 0;                           /* keep how much characters remain to read in file */
    int counts;                            /* keep offset for moving cursor */
    int byte_counter = 0;                  /* count bytes that are readed , in this way i know where to position the cursor to write te output */
    bool breaked = false;                  /* used to check if counter == line_number */
    ll bytes_readed = 0;

    /* check if last character is newline
     * if is not the decrease line_number */
    lseek(fd,-1,SEEK_CUR);
    if(read(fd,buf,1) < 0)
        err_sys("read");

    if(buf[0] != '\n')
        line_number --;


    while(current > 0)
    {
        val = lseek(fd,0,SEEK_CUR);
        
        /* if current position from the beging of file is less than BUFFSIZE counts take "val" value and move cursor 
         * with -val offset */
        if(val < BUFFSIZE)
        {
            counts = val;
            lseek(fd,-val,SEEK_CUR);
        }
        /* else counts take "BUFFSIZE" value and move cursor with -BUFFSIZE offset */
        else
        {
            counts = BUFFSIZE;
            lseek(fd,-BUFFSIZE,SEEK_CUR);
        }
        
        /* now with the cursor moved from the above statements we can read "counts" characters from file */
        if((bytes_readed = read(fd,buf,counts)) < 0)
            err_sys("read error");

        /* and now move back the cursor where he was before the read function */
        current = lseek(fd,-bytes_readed,SEEK_CUR);
        
        /* iterate through all characters in buffer "buf" from last character to first one
         * and count the newline ('\n') characters */
        for(int index = bytes_readed - 1 ; index >= 0; index --)
        {
            byte_counter ++;

            if(buf[index] == '\n')
                counter ++;
            
            if(byte_counter == char_number && cFlag)
            {
                breaked = true;
                break;
            }

            if(counter == line_number +1 && (nFlag || !cFlag))
            {
                /* if counter is equal to line_number 
                 * than decrease with one byte_counter because in if statement above we need to check line_numnber+1
                 * newline characters to get line_number lines 
                 * set breaked on true to break in while statement and break for statement */
                byte_counter --;
                breaked = true;
                break;
            }
        }
        /* after each read we need to iniatilize buffer "buf" with null chars '\0' in order to avoid situation
         * when lines are with different lengths */
        for(int index = 0;index < BUFFSIZE;index ++)
            buf[index] = '\0';

    
    if(breaked == true)
        break;
    }
    /* here we move cursor from end of file with -byte_counter that we have from above */
    lseek(fd,-byte_counter,SEEK_END);

    /* now finally read from current cursor position and write the Output */
    while((bytes_readed = read(fd,buf,BUFFSIZE)) > 0)
    {
        if(write(STDOUT_FILENO,buf,bytes_readed) != bytes_readed)
           err_sys("write error");

        for(int index = 0;index < BUFFSIZE; index++)
            buf[index] = '\0';
    }
    

}
