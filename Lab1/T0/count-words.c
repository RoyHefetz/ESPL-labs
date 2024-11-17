#include <stdio.h>
#include <string.h>
#include <malloc.h>

char *words(int count)
{
    static char word_buffer[] = "words";  // Make it a modifiable array instead of string literal
    if (count == 1)
        word_buffer[strlen(word_buffer)-1] = '\0';
    
    return word_buffer;
}

int print_word_count(char **argv)
{
    int count = 0;
    char **a = argv;
    while (*(a++))
        ++count;
    char *wordss = words(count);
    printf("The sentence contains %d %s.\n", count, wordss);
    
    return count;
}

int main(int argc, char **argv)
{
    print_word_count(argv + 1);
    return 0;
}