#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void replace_tabs(char *str)
{
    size_t length = strlen(str);

    for(int i = 0; i < length; i++)
        if(str[i] == '\t')
            str[i] = ' ';
}

void replace_multi_spaces(char *str)
{
    char *dest = str;

    while(*str != '\0'){
        while(*str == ' ' && *(str + 1) == ' ')
            str++;
    
        *dest++ = *str++;
    }
    *dest = '\0';
}

char **split_str(char *str, const char delimiter)
{
    int count = 0;
    char *temp = str;
    char **result = 0;
    char delim[2];
    delim[0] = delimiter;
    delim[1] = '0';

    while(*temp != '\0'){

        if(*temp == delimiter)
            count++;


        temp++;
    }
    
    if(str[strlen(str)-1] != delimiter || str[0] != delimiter)
        count++;
    
    count++;

    result = malloc(sizeof(char*)*count);

    char *token = strtok(str, delim);
    size_t index = 0; 
    while(token != NULL){
        result[index++] = strdup(token);
        token = strtok(NULL, delim);
    }

    result[index] = NULL;
    return result;
}
