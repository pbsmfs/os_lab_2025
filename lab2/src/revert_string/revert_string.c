#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
    int length = strlen(str);
    
    char *start = str;
    char *end = str + length - 1;
    
    while (start < end)
    {
        char temp = *start;
        *start = *end;
        *end = temp;
        
        start++;
        end--;
    }
}

