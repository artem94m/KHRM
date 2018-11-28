#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


void init_settings() 
{
    FILE *settings;
    int i;

    settings = fopen("settings", "w");
    if (settings == NULL) {
        printf("Can't create settings file.\n"); 
    } 

    char *algs[] = {
        "Schneier=1xFF|1x00|5xRR",
        "VSITR=1x00|1xFF|1x00|1xFF|1x00|1xFF|1xAA",
        "MyAlg=1xRR|2xFAx11x4D",
        "MyAlg2=2x22|1xRR",
        "Quick=1x00",
        "MyAlg3=2x22|1xRR"
    };

    int algs_quantity = sizeof(algs) / sizeof(char *);

    for (i = 0; i < algs_quantity; i++) {
        fprintf(settings, "%s", algs[i]);
        if (i != algs_quantity - 1) {
            fprintf(settings, "\n");
        }
    }

    fclose(settings);
}

void init_random()
{
    struct timeval timestamp;
    unsigned long microseconds;
    
    gettimeofday(&timestamp, NULL);
    microseconds = (timestamp.tv_sec * 1000) + (timestamp.tv_usec / 1000);
    srand(microseconds);
} 

unsigned char get_byte(char *hex)
{
    if (strcmp(hex, "RR") == 0) {
        return rand() % 256;
    } else {
        return strtol(hex, NULL, 16); 
    }
}

int get_substring_quantity(char *str, char sep) 
{
    int i;
    int substr_quantity = 1;

    for (i = 0; i < strlen(str); i++) {
        if (str[i] == sep)
            substr_quantity++;
    }

    return substr_quantity;
}

int split_string(char *str, char *sep, char **strlist, int n) 
{
    char *substr;
    int i;

    substr = strtok(str, sep);
    for (i = 0; i<n && substr; i++) {
        strlist[i] = substr;
        substr = strtok(NULL, sep);
    }

    return i;
}

long int file_size(FILE *file) 
{
    long int size;

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);

    return size;
}

void usage(char *program_name, char **algorithms_desc, int algorithms_quantity)
{
    int i;

    printf("Usage: %s -alg_name or \"param|param|...\" \"filename\"\n", program_name);

    printf("Available algorithms: \n");
    for (i = 0; i < algorithms_quantity; i++) {
        printf("\t%s\n",  algorithms_desc[i]);
    }
    printf("You can set algorithms in settings file.\n\n");
        
    printf("Example of param: 1x4AxRRx5D\n");
    printf("\t1 - number of repeats\n");
    printf("\tx4A and x5D - hex contsant in range (0, 255)\n");
    printf("\txRR - random number in range (0, 255)\n\n");
}

int clean_data(FILE *file, char *alg)
{
    long long i, j, k;

    int rounds_quantity = get_substring_quantity(alg, '|');
    char *rounds[rounds_quantity];
    split_string(alg, "|", rounds, rounds_quantity);

    int max_size_of_round = strlen(alg);
    char *current_round[max_size_of_round];

    long int size_of_file = file_size(file);

    for (i = 0; i < rounds_quantity; i++) {
        memset(current_round, 0, max_size_of_round);
        int params_round_quantity = get_substring_quantity(rounds[i], 'x');
        split_string(rounds[i], "x", current_round, params_round_quantity);

        long int repeat_quantity = strtol(current_round[0], NULL, 10);
        int contsants_quantity = params_round_quantity - 1;

        for (j = 0; j < repeat_quantity; j++) {
            long int left = size_of_file;

            long int buffer_size = contsants_quantity * 200;
            unsigned char buffer[buffer_size];

            while (left > 0) {
                for (k = 0; k < buffer_size; k++) {
                    buffer[k] = get_byte(current_round[(k % contsants_quantity) + 1]);
                }

                if (left > buffer_size) {
                    fwrite(buffer, 1, buffer_size, file);
                } else {
                    fwrite(buffer, 1, left, file);
                }

                left = left - buffer_size;
            }

            fflush(file);
            fseek(file, 0, SEEK_SET);
        }
    }

    fclose(file);

    return 0;
}

char get_allowed_symbol() 
{
    char *symbols = "1234567890)(-_=^$;#№@!qwertyuiop[]{}asdfghjklzxcvbnm,QWERTYUIOPASDFGHJKLZXCVBNM";
    int symbols_quantity = strlen(symbols);

    return symbols[(rand() % symbols_quantity)];
}

void set_zero_size(char *file_path)
{
    FILE *file;

    file = fopen(file_path, "wb");
    fclose(file);
}

void random_rename(char *file_path)
{
    int i;
    int last_slash_pos = -1;
    
    for (i = strlen(file_path) - 1; i >= 0; i--) {
        if (file_path[i] == '/') {
            last_slash_pos = i;
            break;
        }
    }

    int new_file_path_size = strlen(file_path);
    char new_file_path[new_file_path_size];

    strcpy(new_file_path, file_path);

    for (i = last_slash_pos + 1; i < new_file_path_size; i++) {
        new_file_path[i] = get_allowed_symbol();
    }

    rename(file_path, new_file_path);
    strcpy(file_path, new_file_path);
}





int main(int argc, char *argv[]) 
{
    FILE *settings;

    if ((settings = fopen("settings", "r")) == NULL) {
        fclose(settings);
        init_settings(); 
        settings = fopen("settings", "r"); 
    } 

    long int size_of_settings = file_size(settings);
    char imported_settings[size_of_settings + 1];

    memset(imported_settings, '\0', size_of_settings + 1);
    fread(imported_settings, 1, size_of_settings, settings);
    fclose(settings);

    int algorithms_quantity = get_substring_quantity(imported_settings, '\n');
    char *algorithms_desc[algorithms_quantity];

    split_string(imported_settings, "\n", algorithms_desc, algorithms_quantity);

    char *program_name = argv[0];

    if (argc < 3) {
        usage(program_name, algorithms_desc, algorithms_quantity);
    } else {
        char *parameters = argv[1];
        char *chosen_alg = NULL;

        if (parameters[0] == '-') {
            char *algorithms[algorithms_quantity][2];

            int i;
            for (i = 0; i < algorithms_quantity; i++) {
                split_string(algorithms_desc[i], "=", algorithms[i], 2);
            }

            for (i = 0; i < algorithms_quantity; i++) {
                if (strcmp(&parameters[1], algorithms[i][0]) == 0) {
                    chosen_alg = algorithms[i][1];
                    break;   
                }
            }

            if (chosen_alg == NULL) {
                printf("There is no %s algorithm in settings file.\n", &parameters[1]);
            }
        } else {
            chosen_alg = parameters;
        }
        
        init_random();

        FILE *file;
        char *file_path = argv[2];

        file = fopen(file_path, "rb+");
        if (file == NULL) {
           printf("Can't open %s\n", file_path);
           fclose(file);
           exit(1);
        }
 
        clean_data(file, chosen_alg);

        set_zero_size(file_path);

        random_rename(file_path);

        if (remove(file_path) == 0) {
            printf("File completely removed.\n");
        } else {
            printf("Can't remove file.\n");
        }
    }
    
    return 0;
}