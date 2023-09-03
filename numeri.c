#include<stdio.h>


int main(){
    int pippo[100000];
    FILE *file = fopen("numeri.txt", "w+");
    for(int i = 1; i < 100000; i++){
        fprintf(file, "%d\n", i);
    }    
    fclose(file);
    return 0;
}