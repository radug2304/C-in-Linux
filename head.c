#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include "lab.h"

#define BUFFSIZE 4096

/* function for trying to parse the argument for n option */
void tryParse(char *arg);

/* function for write output */
void writeOutput(int fd);

/* options */
bool nFlag = false;
bool cFlag = false;


int file_size = 0;
int line_number = 10;
int char_number = 1;

char read_buf[2];
char write_buf[BUFFSIZE];

int main(int argc,char **argv)
{
    struct stat status;
    int fd; //file descriptor for files
    int option; // current option
    bool first_print = false;
    bool all_wrong = true;
    char *arg="", *err;

    while(( option = getopt(argc,argv,"n:c:") ) != -1)
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
                err = "Try 'head --help' for more information.\n";
                if(write(STDERR_FILENO,err,strlen(err)) == -1)
                    err_sys("stderr write error");
                exit(EXIT_FAILURE);
                break;
        }

    if((line_number == 0 && nFlag) || (char_number == 0 && cFlag))
        exit(0);


    /* if no files are given , entry on stdin */
    if(argc - optind == 0)
    {
        writeOutput(STDIN_FILENO);
        exit(0);
    }

    if(argc - optind > 0)
        for(int index = optind; index < argc; index++)
        {
            if(strcmp(argv[index],"-") == 0)
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

                writeOutput(STDIN_FILENO);
                first_print = true;
            }
            else
            {
                char *header = malloc(strlen(argv[index])*5);
                snprintf(header,strlen(argv[index])*5,"==> %s <==\n",argv[index]);
    
                if(lstat(argv[index],&status) < 0)
                    status.st_mode = 0;

            
                if((fd = open(argv[index],O_RDONLY)) < 0)
                    err_ret("cannot open '%s' for reading",argv[index]);

                else 
                    if(read(fd,read_buf,0) < 0)
                    {
                        if(argc - optind > 1)
                        {
                            if(first_print == true)
                                if(write(STDOUT_FILENO,"\n",1) < 0)
                                    err_ret("write error");

                            if(write(STDOUT_FILENO,header,strlen(header)) < 0)
                                err_ret("write error");

                            first_print = true;
                        }

                        err_ret("error reading '%s'",argv[index]);
                    }

                if(fd > 0 && !S_ISDIR(status.st_mode))
                {
                    if(argc - optind > 1)
                    {
                        if(first_print == true)
                            if(write(STDOUT_FILENO,"\n",1) < 0)
                                err_ret("write error");

                        if(write(STDOUT_FILENO,header,strlen(header)) < 0)
                            err_ret("write error");
                    }
                       
                    writeOutput(fd);
                    close(fd);
                    all_wrong = false;
                    first_print = true;
                }

                free(header);
            
            }
        }
                    

    if(all_wrong == true)
        exit(EXIT_FAILURE);
    else
        exit(0);
}

void tryParse(char *arg)
{
    char *errstr;
    long number = strtol(arg,&errstr,10);

    if(errstr[0] == '\0')
    {
        if(cFlag)
            char_number = number;

        if(nFlag)
            line_number = number;
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

void writeOutput(int fd)
{
    int input_counter = 0;
    int char_counter = 0;
    int j = 0;

    while(read(fd,read_buf,1) > 0)
    {
        if(cFlag)
            char_counter ++;

        write_buf[j] = read_buf[0];
        j++;
        
        if(read_buf[0] == '\n' || j == BUFFSIZE - 1)
        {
            if(write(STDOUT_FILENO,write_buf,strlen(write_buf)) == -1)
                err_sys("write error");

            if(read_buf[0] == '\n')
                input_counter ++;
            
            for(int ind = 0; ind <= j; ind++)
                write_buf[ind] = '\0';
            
            j = 0;
        }

        if(input_counter == line_number && (nFlag || cFlag == false))
            break;

        if(char_counter == char_number && cFlag)
        {
            if(write(STDOUT_FILENO,write_buf,strlen(write_buf)) == -1)
                err_sys("write error");
            break;
        }

    }
}
