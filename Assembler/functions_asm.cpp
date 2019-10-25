//
// Created by texnar on 23/10/2019.
//

#include "functions_asm.h"

void CountConstHash(commands* com) {

#define DEF_CMD(name, num, code, code_cpu) \
    com->hash_##name = MurmurHash(#name);

#include "commands.h"

#undef DEF_CMD

#define REGISTER(name, num)\
    com->hash_reg_##name = MurmurHash(#name);

#include "register.h"

#undef REGISTER
}


unsigned int MurmurHash(const char text[])
{
    const unsigned int m = 0x5bd1e995;
    const unsigned int seed = 0;
    const int r = 24;

    unsigned int len = strlen(text);

    const unsigned char * data = (const unsigned char *) text;
    unsigned int h = seed ^ len;

    unsigned int k = 0;

    while (len >= 4)
    {
        k  = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len)
    {
        case 3:
            h ^= data[2] << 16;
        case 2:
            h ^= data[1] << 8;
        case 1:
            h ^= data[0];
            h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

long ItLength(FILE* file){
    assert(file != NULL);

    fseek(file, 0, SEEK_END);
    long result = ftell(file);
    fseek(file, 0, SEEK_SET);

    return result;
}

FILE* ConvertFile(FILE* file, commands* com, int num_arg, char *poin_arg[], const char name_file[]){
    assert(file != nullptr);


    long length = ItLength(file);

    char* buffer = (char *)calloc(length, sizeof(char));
    char* data = (char *)calloc(length * 3, sizeof(char)); // should be rewrite!
    int num_re = fread(buffer, sizeof(char), length, file);

    #ifdef DEBUG
    printf("%s\n", buffer);
    #endif

//    int num_enter = 0;
    long write_point = 0;
//    char* last_symb = buffer;

    *(int *)data = 'aKiM';
    data[4] = VERSION;
    write_point += 5;

    CommandProcessing (buffer, data, length, com, &write_point);


    FILE* newfile = 0;
    if (num_arg > 2){
        strcat(poin_arg[2],".nasm");
        newfile = fopen(poin_arg[2], "w");
    } else newfile = fopen(name_file, "w");


    fwrite(data, sizeof(char), write_point, newfile);
    free(buffer);
    free(data);
    fclose(newfile);
}

void CommandProcessing (char* buffer, char* data, long length, commands* com, long* write_point) {
    assert(buffer != 0);
    assert(data != 0);
    assert(com != 0);
    assert(write_point != 0);

    jumps array_jumps[MAX_NUM_JMP] = {};

    long copy_write_point = *write_point;
    for(int j = 0; j < 2; j++){

        *write_point = copy_write_point;
        int num_enter = 1;
        char *last_symb = buffer;

        while ((last_symb - buffer) < length) {
            char *first_symb = last_symb;
            last_symb = strchr(first_symb, '\n');
            if (last_symb == nullptr)
                last_symb = buffer + (length - 1);

            *last_symb = '\0';
            if (*(last_symb - 1) == '\r') *(last_symb - 1) = '\0';
            char command[10] = {};
            int pointer_read = 0;

            char *comment = strchr(first_symb, ';');
            if (comment != nullptr) {
                *comment = '\0';
            }

            int num_read = sscanf(first_symb, "%s%n", command, &pointer_read);
            //        if (num_read == 0) {
            //        }
            unsigned int hash_read_com = MurmurHash(command);

#ifdef DEBUG
            printf("%s\n", command);
            printf("%u\n", hash_read_com);
            printf("Poiter_read %u\n", pointer_read);
#endif
            bool known_command = false;


#define DEF_CMD(name, num, code, code_cpu)\
                if (hash_read_com == com->hash_##name){ code; known_command = true;}

#include "commands.h"

            if (!known_command) {

                    #ifdef DEBUG
                    printf("sscanf %d", sscanf(command, "%*[A-Za-z:0-9]%n", &pointer_read));
                    #endif

                if (sscanf(command, "%*[A-Za-z:0-9]%n", &pointer_read) == -1) known_command = true;
                if (command[0] == ':') {
                    long num_jmp = 0;
                    if (sscanf(&(command[1]), "%ld", &num_jmp)) {
                        array_jumps[num_jmp].address_jump_to = *write_point;
                        array_jumps[num_jmp].used_jump = true;
                        known_command = true;
                    }
                }
            }

            if (!known_command) {
                printf("Don't find command \"%s\" in line (%d) \n", command, num_enter);
                abort();
            }
#undef DEF_CMD


            num_enter++;
            // it is return text (\n)
            *last_symb = '\n';
            if (comment != nullptr) {
                *comment = ';';
            }
            last_symb++;
            #ifdef DEBUG
            printf("\n");
            #endif
        }
    }
}

void FindRegister(commands* com, char str_reg[], unsigned int hash_read_reg, char* num_reg, int num_enter){
    bool known_register = false;

#define REGISTER(name, num)\
    if (com->hash_reg_##name == hash_read_reg && #name[0] == str_reg[0]){\
        *num_reg = num;\
        known_register = true;\
    }
#include "register.h"

    if (!known_register){
        printf("Unknown register in command push '%s'<%d>\n", str_reg, num_enter);
        abort();
    }

#undef REGISTER
}

FILE* OpenFile(int num_arg, char *poin_arg[]){
    if (num_arg > 1){
        return fopen(poin_arg[1], "rb");
    } else
        return fopen("../workspace_for_processor/text.txt", "rb");
}

void FunctionJMP(char** data, long* write_point, jumps array_jumps[], commands* com, int* pointer_read, char* first_symb, int num_enter){

//void FunctionJMP(char **data, long *write_point, jumps array_jumps[], commands com, int *pointer_read, char *first_symb,
//                 int num_enter) {
    long num_jmp = 0;

//    printf("poi before %d\n", pointer_read);
    if (sscanf((first_symb + *pointer_read), "%ld%n", &num_jmp, pointer_read)){

#ifdef DEBUG
        printf("num %ld\n", num_jmp);
#endif

        *(data[(*write_point)++]) = WRITE_NUM;
        *(long*)(&data[(*write_point)]) = num_jmp;
        *(write_point) += sizeof(long);
    }
//        printf("pi after %d\n", pointer_read);
//        char s[10] = {};
//        sscanf((first_symb + pointer_read), "%s%n", s, &pointer_read);
//    printf("s[%s\n", s);
    if (char* jmp_symbol = strchr(first_symb + *pointer_read, ':')){
        *pointer_read = jmp_symbol + 1 - first_symb;
        sscanf((first_symb + *pointer_read), "%ld%n", &num_jmp, pointer_read);

#ifdef DEBUG
        printf("jmp %ld\n", num_jmp);
#endif

        if (num_jmp < 0 || num_jmp >= MAX_NUM_JMP) {
            printf("Invalid jmp number<%d>\n", num_enter);
            abort();
        }

        (*data)[(*write_point)++] = WRITE_NUM;
        *(long*) (&(*data)[*write_point]) = array_jumps[num_jmp].address_jump_to;
        *(write_point) += sizeof(long);
        array_jumps[num_jmp].used_jump = true;
    } else {
        char str_reg[10] ={};
        char num_reg = 0;
        if (sscanf((first_symb + *pointer_read), "%s%n", str_reg, pointer_read) >= 1) {
//                printf("%s\n", str_reg);
//                printf("Hash reg %d\n", com->hash_reg_ax);

            unsigned int hash_read_reg = MurmurHash(str_reg);
//                printf("Hash read %d", hash_read_reg);
            FindRegister(com, str_reg, hash_read_reg, &num_reg, num_enter);

            (*data)[(*write_point)++] = WRITE_REG;

            *((unsigned char *) (*data + *write_point)) = num_reg;
            *write_point += sizeof(char);

//
        } else {
            printf("Don't find number or register of command push <%d>\n", num_enter);
            abort();
        }
    }

    //*((char*) (&(*data)[*write_point])) = num;
}

void FindLabelJMP(char** data, long* write_point, jumps array_jumps[],
        int* pointer_read, char* first_symb, int num_enter){


    long num_jmp = 0;
    if (sscanf((first_symb + *pointer_read), "%ld%n", &num_jmp, pointer_read)){

        #ifdef DEBUG
        printf("num %ld\n", num_jmp);
        #endif

        *(long*)(&(*data)[(*write_point)]) = num_jmp;
        *(write_point) += sizeof(long);
    }

    if (char* jmp_symbol = strchr(first_symb + *pointer_read, ':')){
        *pointer_read = jmp_symbol + 1 - first_symb;
        sscanf((first_symb + *pointer_read), "%ld%n", &num_jmp, pointer_read);

        #ifdef DEBUG
        printf("jmp %ld\n", num_jmp);
        #endif

        if (num_jmp < 0 || num_jmp >= MAX_NUM_JMP) {
            printf("Invalid jmp number<%d>\n", num_enter);
            abort();
        }


        *(long*) (&(*data)[*write_point]) = array_jumps[num_jmp].address_jump_to;
        *(write_point) += sizeof(long);
        array_jumps[num_jmp].used_jump = true;

    }
}

void FunctionCALL(char** data, long* write_point, jumps array_jumps[],
        int* pointer_read, char* first_symb, int num_enter){

    long num_jmp = 0;
    if (sscanf((first_symb + *pointer_read), "%ld%n", &num_jmp, pointer_read)){

        #ifdef DEBUG
        printf("num %ld\n", num_jmp);
        #endif

        *(long*)(&(*data)[(*write_point)]) = num_jmp;
        *(write_point) += sizeof(long);

        *(long*)(&(*data)[(*write_point)]) = *write_point;
        *(write_point) += sizeof(long);
    }

    if (char* jmp_symbol = strchr(first_symb + *pointer_read, ':')){
        *pointer_read = jmp_symbol + 1 - first_symb;
        sscanf((first_symb + *pointer_read), "%ld%n", &num_jmp, pointer_read);

        #ifdef DEBUG
        printf("jmp %ld\n", num_jmp);
        #endif

        if (num_jmp < 0 || num_jmp >= MAX_NUM_JMP) {
            printf("Invalid jmp number<%d>\n", num_enter);
            abort();
        }

        *(long*) (&(*data)[*write_point]) = array_jumps[num_jmp].address_jump_to;
        *(write_point) += sizeof(long);

        *(long*)(&(*data)[(*write_point)]) = (long)(*write_point + sizeof(long));
        *(write_point) += sizeof(long);

        array_jumps[num_jmp].used_jump = true;
    }

}

void FunctionPUSH(char** data, long* write_point, commands* com,
        int* pointer_read, char* first_symb, int num_enter){

    double num = 0;
    if (sscanf((first_symb + *pointer_read), "%lg%n", &num, pointer_read) == 0) {

        char str_reg[10] ={};
        char num_reg = 0;
        if (sscanf((first_symb + *pointer_read), "%s%n", str_reg, pointer_read) >= 1) {


            unsigned int hash_read_reg = MurmurHash(str_reg);
//                printf("Hash read %d", hash_read_reg);
            FindRegister(com, str_reg, hash_read_reg, &num_reg, num_enter);


            (*data)[(*write_point)++] = WRITE_REG;

            *((unsigned char *) (*data + *write_point)) = num_reg;
            *write_point += sizeof(char);


        } else {
            printf("Don't find number or register of command push <%d>\n", num_enter);
            abort();
        }
    } else {

        #ifdef DEBUG
        printf("Number %lg\n", num);
        #endif

        (*data)[(*write_point)++] = WRITE_NUM; //send number or registr
        /*if number 0xAA if registr 0xBB*/
        //            data[(*write_point)] = 0x77; // this is wall

        *((double *) (*data + *write_point)) = num;

        #ifdef DEBUG
        printf("Num FROM DATA %lg\n", *((double *) (*data + *write_point)));
        #endif

        *write_point += sizeof(double);
    }
}

void FunctionPOP(char** data, long* write_point,
                  commands* com, int* pointer_read, char* first_symb, int num_enter){
    char str_reg[10] ={};
    char num_reg = 0;
    if (sscanf((first_symb + *pointer_read), "%s%n", str_reg, pointer_read) >= 1) {

        unsigned int hash_read_reg = MurmurHash(str_reg);

        FindRegister(com, str_reg, hash_read_reg, &num_reg, num_enter);

        (*data)[(*write_point)++] = WRITE_REG; // pop upload number in register

        *((unsigned char *) (*data + *write_point)) = num_reg;
        *write_point += sizeof(char);

    } else {

        (*data)[(*write_point)++] = WRITE_NOTHING; // pop not upload number. lost the number

    }
}