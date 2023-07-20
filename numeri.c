#include<stdio.h>


int main(){
    int pippo[100000];
    FILE *file = fopen("numeri.txt", "r+");
    for(int i = 0; i < 100000; i++){
        fprintf(file, "%d\n", i);
    }    
    fclose(file);
    return 0;
}