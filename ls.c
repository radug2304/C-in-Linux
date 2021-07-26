#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<time.h>
#include<pwd.h>
#include<grp.h>
#include<string.h>
#include<limits.h>
#include<locale.h>
#include<langinfo.h>
#include<libgen.h>
#include<stdbool.h>
#include "lab.h"



/* function for printing long format */
void printLongFormat(char *name);

/* function for checking time style format from TIME_STYLE env variable
if variable isn't setted then program will set it on default "locale" value
otherwise if variable is set on a wrong value then exit failure */
void check_time_style();

/* function for calculating total number of blocks and max size of all files */
void takeInfo(struct dirent **namelist,int n,bool total);

/* function to determine width for printing size of a file */
int width(long long size);

/* maximum size of files used to print width */
long long max_size = 0;

/* function for setting time width */
void setFormat(int month);

/* maximum length of hard link */
long long max_link_len = 0;

/* variable for maximum lenght of username and group name for width */
int user_max_length = 0;
int group_max_length = 0;

/* variable for maximum length from all months used for strftime width */
int months_max_length = 0;

/* flags A and a declared globally because are used in myFilter function */
int flag_a = 0;
int flag_A = 0;

/* time_format and nsec used for determine format of time */
char *time_format;
bool locale_style = false;
bool nsec = false;

/* variable used to create full path of file */
char *path;
char path_buf[PATH_MAX];

/* structure for file status */
struct stat sb;

/* variable to check if is needed to print total_blocks */
bool total;

/* filter function for scandir */
int myFilter(const struct dirent *dir)
{
    if(flag_a == 0 && flag_A == 0)
            return !(dir->d_name[0] == '.');

    if(flag_A == 1)
            return !(strcmp(dir->d_name,".") == 0 || strcmp(dir->d_name,"..") == 0);

    return 1;
}

/* sort function for files array */
int sort(const void *a, const void *b)
{
    return strcoll(*(const char **)a, *(const char **)b);
}


int main(int argc,char **argv)
{
    struct dirent **namelist;
    int n;
    int i=0;
    int option , outFlag = 0, rFlag = 0;
    int flag_l = 0; // flag for -l option
    char *name;

    setlocale(LC_ALL,"");

    while((option = getopt(argc,argv,"1lraA")) !=-1)

        switch(option)
        {
            case '1':
                outFlag = 1;
                break;
            case 'r':
                rFlag = 1;
                break;
            case 'a':
                flag_a = 1;
                flag_A = 0;
                break;
            case 'A':
                flag_A = 1;
                flag_a = 0;
                break;
            case 'l':
                flag_l = 1;
                break;
            case '?':
                err_msg("Try 'ls -- help' for more information.");
                exit(EXIT_FAILURE);
        }
    /* check here if time_style is correct in case flag_l is on */
    if(flag_l == 1)
        check_time_style();
    /* here we need to separate regular files from directories
     * *files will keep name of directories and file_index will count number of directories
     * *nodir dirent list will keep name of other files and nodir_index will count number of other files */
    int file_index = 0;
    int nodir_index = 0;
    char *files[argc];
    struct dirent **nodir = (struct dirent **)malloc(argc*sizeof(struct dirent*));
    int exit_status = 0; /* variable for exit code */

    /* if no arguments are given then we put current dir "." in *files */
    if(argc - optind == 0)
    {
        files[file_index] = ".";
        file_index ++;
    }
    else
    {
        /* otherwise we iterate over all arguments 
         * take status of every file ,  if lstat fail , st_mode of file is set to 0
         * in order to print error
         * otherwise , if lstat don`t fail , put directories in *files and other files in *nodir */
        for(int index = optind; index < argc;index++)
        {
            if(lstat(argv[index],&sb) < 0)
                sb.st_mode = 0;

            if(sb.st_mode == 0 && access(argv[index],F_OK) == -1)
            {
                err_ret("cannot access '%s'",argv[index]);
                exit_status = 2;
            }
            else
            {
                if(S_ISDIR(sb.st_mode))
                {   
                    files[file_index] = argv[index];
                    file_index ++;
                }
                else
                {
                    nodir[nodir_index] = (struct dirent*)malloc(sizeof(struct dirent) - 256 + strlen(argv[index]) + 1);
                    strcpy(nodir[nodir_index]->d_name,argv[index]);
                    nodir_index ++;
                }
            }
        }
    }

    /* sort each array of files [no_dir and files] */
    qsort(files, file_index, sizeof(char*),sort);
    qsort(nodir,nodir_index,sizeof(struct dirent **),(const void*)alphasort);
    
    /* iterate over files in dirent list nodir and print informations */
    path = ".";
    if(flag_l == 1)
        takeInfo(nodir,nodir_index,false);


    while(i<nodir_index)
    {
        if(rFlag == 1)
            name = nodir[nodir_index-1-i]->d_name;
        else
            name = nodir[i]->d_name;

        if(flag_l == 1)
            printLongFormat(name);
        else
            if(outFlag == 1)
                printf("%s\n",name);
            else
                if(i<nodir_index-1)
                    printf("%s  ",name);
                else
                    printf("%s",name);

        if(rFlag == 0)
            free(nodir[i]);
        else
            free(nodir[nodir_index-1-i]);
        i++;
    }

    if(outFlag == 0 && flag_l == 0 && nodir_index > 0)
        printf("\n");

    free(nodir);
    
    i=0;

    if(file_index > 0 && nodir_index >0)
        printf("\n");

    for(int index = 0; index < file_index;index++)
    {
        n = scandir(files[index], &namelist, myFilter, alphasort);
        path=files[index];

        if(n < 0)
        {
            err_ret("cannot open directory '%s'",files[index]);
            exit_status = 2;
        }
        else
        {
            if(index > 0)
                printf("\n");

            if(argc - optind > 1)
                printf("%s:\n",files[index]);

            if(flag_l == 1)
            {
                max_size = 0;
                takeInfo(namelist,n,true);
            }
    
            while (i<n)
            {
                if (rFlag == 1)
                    name = namelist[n-1-i]->d_name;
                else
                    name = namelist[i]->d_name;

                if(flag_l == 1)
                    printLongFormat(name);
                else
                    if(outFlag == 1)
                        printf("%s\n",name);
                    else
                        if(i<n-1)
                            printf("%s  ",name);
                        else
                            printf("%s",name);

                if(rFlag == 0)
                    free(namelist[i]);
                else
                    free(namelist[n-i-1]);

                i++;
            }

            if(outFlag == 0 && flag_l == 0)
                printf("\n");

            free(namelist);
            
        }

        i = 0;
    }

    if(locale_style == true)
        free(time_format);
    
    exit(exit_status);
}

void printLongFormat(char *name)
{
    /* some changes here
     * if name is a relative path then construct the path relative to current director 
     * else if name is a absolute path put name in path_buf */
    if(name[0] != '/')
        snprintf(path_buf,PATH_MAX,"%s/%s",path,name);
    else
        snprintf(path_buf,PATH_MAX,"%s",name);

    if(lstat(path_buf,&sb) == -1)
    {
        err_sys("stat error");
    }


    /* print file type */
    switch(sb.st_mode & S_IFMT)
    {
        case S_IFBLK:  printf("b");	 break;
        case S_IFCHR:  printf("c");	 break;
        case S_IFDIR:  printf("d");	 break;
        case S_IFIFO:  printf("p");	 break;
        case S_IFLNK:  printf("l");	 break;
        case S_IFREG:  printf("-");	 break;
        case S_IFSOCK: printf("s");	 break;
        default:       printf("?");	 break;
    }

    /* print file permissions */
    mode_t perm = sb.st_mode;

    printf("%s",(perm & S_IRUSR) ? "r" : "-");
    printf("%s",(perm & S_IWUSR) ? "w" : "-");
    printf("%s",(perm & S_IXUSR) ? "x" : "-");
    printf("%s",(perm & S_IRGRP) ? "r" : "-");
    printf("%s",(perm & S_IWGRP) ? "w" : "-");
    printf("%s",(perm & S_IXGRP) ? "x" : "-");
    printf("%s",(perm & S_IROTH) ? "r" : "-");
    printf("%s",(perm & S_IWOTH) ? "w" : "-");
    printf("%s",(perm & S_IXOTH) ? "x" : "-");

    /* print hard link count */
    printf(" %*ld",width(max_link_len),sb.st_nlink);

    /* print owner name */
    struct passwd *pws;
    pws = getpwuid(sb.st_uid);

    printf(" %s",pws->pw_name);
    printf("%*s",(int)(user_max_length - strlen(pws->pw_name)),""); /* print width for user name */

    /* print group name */
    struct group *grp;
    grp = getgrgid(sb.st_gid);

    printf(" %s",grp->gr_name);
    printf("%*s",(int)(group_max_length - strlen(grp->gr_name)),"");/* print width for group name */

    /* print file size */
    printf(" %*ld",width(max_size),sb.st_size);

    /* print last modification time */
    struct tm *timer;
    char time_buffer[256];
    timer = localtime(&sb.st_mtime);

    /* if time_format is locale , then for each month we need to calculate width */
    if(locale_style == true)
        setFormat(timer->tm_mon);

    strftime(time_buffer,256,time_format,timer);
    
    if(nsec == true)
    {
        /* construct date format with nanoseconds */
        char nsec_time[256];
        snprintf(nsec_time,256,time_buffer,sb.st_mtim.tv_nsec);
        strcpy(time_buffer,nsec_time);
    }

    printf(" %s",time_buffer);

    /* if file is a symbolic link print file name and the file where it points
       otherwise print file name */
    if((sb.st_mode & S_IFMT) == S_IFLNK)
    {
        /* realpath function used to determine to which file symbolink link points to */
        char buffer[PATH_MAX];
        char *res = realpath(name,buffer);
        if(res)
            printf(" %s -> %s",name,basename(buffer));
        else
            printf(" %s -> %s",name,basename(buffer));
    }
    else
        printf(" %s",name);
    printf("\n");

}


void takeInfo(struct dirent **namelist, int n,bool total)
{
    int i = 0;
    long long total_blocks = 0;
    struct stat status;
    struct passwd *pws;
    struct group *grp;
    int user_len, group_len;
    char *name;

    while(i<n)
    {
        char temp_path[PATH_MAX];
        name = namelist[i]->d_name;
        
        /* same change like in printLongFormat function */
        if(name[0] != '/')
            snprintf(temp_path,PATH_MAX,"%s/%s",path,name);
        else
            snprintf(temp_path,PATH_MAX,"%s",name);
        
        if(lstat(temp_path,&status) == -1)
        {
            err_sys("stat error");
        }

        /* calculate max_size and total_blocks only for visible files
           max_size and total_blocks values depends on which files are printed
           also here calculate width for user and group name */
        pws = getpwuid(status.st_uid);
        grp = getgrgid(status.st_gid);

        user_len = (int)strlen(pws->pw_name);
        group_len = (int)strlen(grp->gr_name);

        if( user_len > user_max_length )
            user_max_length = user_len;

        if( group_len > group_max_length )
            group_max_length = group_len;

        max_size = (status.st_size > max_size) ? status.st_size : max_size;
        total_blocks += status.st_blocks/2;
        max_link_len = ((int)status.st_nlink > max_link_len) ? (int)status.st_nlink : max_link_len;

        i++;
    }
    if(total == true)
        printf("total %lld\n",total_blocks);
}

int width(long long size)
{
    int count = 0;

    while(size)
    {
        count++;
        size /= 10;
    }

    return count;
}

void check_time_style()
{
    if(!getenv("TIME_STYLE"))
        putenv("TIME_STYLE=locale"); /* set TIME_STYLE on "locale" default value if it is not set */
    int length;
    char *time_style = getenv("TIME_STYLE");

    if(strcmp(time_style,"locale") == 0 || strcmp(time_style,"posix-locale") == 0)
    {
        /* get the maximum length of months abreviations for printing width */
        for(int i=0;i < 12; i++)
        {
            /* locale-aware month names can be UTF-8 strings, so calculate the
            * length from the number of actual characters printed, not the byte
            * length of the string
            * easiest way is to emulate a conversion into wide character string
            * without an output buffer, so no actual conversion will take place
            * just the number of characters are returned */
            length = (int)mbstowcs(NULL, nl_langinfo(ABMON_1+i), 0);

            if(length > months_max_length)
                months_max_length = length;

            locale_style = true;
        }
        /* allocate size in time_format for using it in snprintf function
         * which is called in setFormat function */
        time_format = malloc(256);
    }

    if(strcmp(time_style,"iso") == 0 || strcmp(time_style,"posix-iso") == 0)
        time_format = "%m-%d %H:%M";
    if(strcmp(time_style,"long-iso") == 0 || strcmp(time_style,"posix-long-iso") == 0)
        time_format = "%Y-%m-%d %H:%M";
    if(strcmp(time_style,"full-iso") == 0 || strcmp(time_style,"posix-full-iso") == 0)
    {
        time_format = "%Y-%m-%d %H:%M:%S.%%09u %z";
        nsec = 1;
    }

    if(time_style[0] == '+')
        time_format = time_style+1;

    if(!time_format && locale_style == false)
    {
        /* check for [posix-] prefix */
        char prefix[6];
        strncpy(prefix,time_style,6);
        prefix[6]='\0';

        if(strcmp(prefix,"posix-") == 0)
            time_style = time_style+6;
        /* if TIME_STYLE env var is "" then original ls command print ambiguous else print invalid */
        char *text_difference;

        if(strcmp(time_style,"") == 0)
            text_difference = "ambiguous";
        else
            text_difference = "invalid";

        err_msg("%s argument ‘%s’ for ‘time style’\nValid arguments are:\n  - [posix-]full-iso\n  - [posix-]long-iso\n  - [posix-]iso\n  - [posix-]locale\n  - +FORMAT (e.g., +%H:%M) for a 'date'-style format\nTry 'ls --help' for more information.",text_difference,time_style);
        exit(2);
    }
}

void setFormat(int month)
{
    /* calculate the number of UTF-8 characters in the months short name
     * see explanation at the months_max_length calculation */
    int month_width = (int)mbstowcs(NULL, nl_langinfo(ABMON_1 + month), 0);
    /* create the time_format string having %b %e %H:%M but a spacing
     * between b and e which aligns with the longest month name
     * so the time format is printed using a space padding before %e
     * equal to the difference between months_max_length and current month_width
     * of course the padding should also include the 2 characters of %e */
    snprintf(time_format,256,"%s %*s %s","%b", months_max_length - month_width + 2, "%e", "%H:%M");
}


