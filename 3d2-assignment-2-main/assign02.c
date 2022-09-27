#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Libraries for LED functionality 
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"


// Define variables for the LED

#define IS_RGBW true        // Will use RGBW format
#define NUM_PIXELS 1        // There is 1 WS2812 device in the chain
#define WS2812_PIN 28       // The GPIO pin that the WS2812 connected to


/* ---GLOBAL VARIABLES--- */

char buffer[5];
int j = 0;
int level_selected = 0;
int print_letter_done = 0;
int a;
int end;
int life = 3;
int ans_check = 0;
int restart = 0;
int right_answers = 0;
int winning_flag =1;
char check[5];
char morse_check[5];
int input_flag = 0;
int space_flag = 0;

int count_of_lives_used = 0;
int count_of_correct_answers = 0;
int count_of_wrong_answers = 0;


char char_array[] = {
    // Digits 0 - 9
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
    // Letters A - Z
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
char morse_table[36][5] = { // Must declare as a char pointer array to get an array of strings since this is in C not C++
    // Digits 0 - 9
    "-----", ".----", "..---", "...--", "....-", ".....",
    "-....", "--...", "---..", "----.", 
    // Letters A - Z
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....",
    "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.",
    "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-",
    "-.--", "--..",
}; 

/* ---FUNCTIONS--- */

// Must declare the main assembly entry point before use.
void main_asm();


// Initialise a GPIO pin – see SDK for detail on gpio_init()
void asm_gpio_init(uint pin) {
    gpio_init(pin);
}

// Set direction of a GPIO pin – see SDK for detail on gpio_set_dir()
void asm_gpio_set_dir(uint pin, bool out) {
    gpio_set_dir(pin, out);
}

// Get the value of a GPIO pin – see SDK for detail on gpio_get()
bool asm_gpio_get(uint pin) {
    return gpio_get(pin);
}

// Set the value of a GPIO pin – see SDK for detail on gpio_put()
void asm_gpio_put(uint pin, bool value) {
    gpio_put(pin, value);
}


// Enable edge interrupts – see SDK for detail on gpio_set_irq_enabled()
void asm_gpio_set_irq(uint pin) {
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_FALL, true);
}

// Declare watchdog functions
void watchdog_update();

void watchdog_enable(uint32_t delay_ms, bool pause_on_debug);


// Functions used for intialising and updating the LED
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}


static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}


void stats(){
    watchdog_update();
    printf("\nNumber of lives used: %d\n",count_of_lives_used);
    printf("Number of correct answers: %d\n",count_of_correct_answers);
    printf("Number of wrong answers: %d\n",count_of_wrong_answers);
}


// Initialise the LEDs and change the colour depending the number of lives
static inline void update_led() {

    if (game_start == 0) {
        // Set LED to BLUE once the game opens but hasnt started
        put_pixel(urgb_u32(0x00, 0x00, 0xFF));
    }
    else {
        if (life >= 3)
        // Set LED to GREEN at half intensity
        put_pixel(urgb_u32(0x00, 0x7F, 0x00));

        if (life == 2)
        // Set LED to YELLOW at half intensity
        put_pixel(urgb_u32(0xF7, 0xFF, 0x00));

        if (life == 1)
        // Set LED to ORANGE at half intensity
        put_pixel(urgb_u32(0xFF, 0x78, 0x00));

        if (life == 0) {
            put_pixel(urgb_u32(0xFF, 0x08, 0x00));
            
        }
        // Set LED to RED at half intensity
        

      //  printf("Lives remaining: %d\n", lives);
    }
}

// Print the opening screen with rules explaining the game
void welcome_screen() {
  printf("\n+------------------------------------------------------------+\n");
  printf("|              ASSIGNMENT #02      Group 19                  |\n");
  printf("+------------------------------------------------------------+\n");
  printf("|  #        #  ########  ########     ######     ########    |\n");
  printf("|  # #    # #  #      #  #       #    #          #           |\n");
  printf("|  #  #  #  #  #      #  #       #    #          #           |\n");
  printf("|  #   #    #  #      #  #######      ######     ########    |\n");
  printf("|  #        #  #      #  #     #           #     #           |\n");
  printf("|  #        #  #      #  #      #          #     #           |\n");
  printf("|  #        #  ########  #       #    ######     ########    |\n");
  printf("|                                                            |\n");
  printf("|        ########    ########  ######      ########          |\n");
  printf("|        #           #      #  #     #     #                 |\n");
  printf("|        #           #      #  #      #    #                 |\n");
  printf("|        #           #      #  #      #    ########          |\n");
  printf("|        #           #      #  #      #    #                 |\n");
  printf("|        #           #      #  #     #     #                 |\n");
  printf("|        ########    ########  ######      ########          |\n");
  printf("|                                                            |\n");
  printf("|        ########        #      #       #   ########         |\n");
  printf("|        #             #   #    # #   # #   #                |\n");
  printf("|        #           #       #  #  # #  #   #                |\n");
  printf("|        #    ###    #########  #   #   #   ########         |\n");
  printf("|        #      #    #       #  #       #   #                |\n");
  printf("|        #      #    #       #  #       #   #                |\n");
  printf("|        ########    #       #  #       #   ########         |\n");
  printf("+------------------------------------------------------------+\n");
  printf("|          USE GP21 TO ENTER A SEQUENCE TO BEGIN             |\n");
  printf("|           \".----\" - LEVEL 01 - CHARS (EASY)                |\n");
  printf("|           \"..---\" - LEVEL 02 - CHARS (HARD)                |\n");
  printf("|           \"...--\" - LEVEL 03 - WORDS (EASY)                |\n");
  printf("|           \"....-\" - LEVEL 04 - WORDS (HARD)                |\n");
  printf("+------------------------------------------------------------+\n");
  printf("The rules are as follows:\n"); 
  printf("1. Enter the character displayed in morse\n"); 
  printf("2. If you get it correct you gain a life\n");
  printf("3. Otherwise you lose a life. The LED will indicate how many lives you have\n");
  printf("4. If you take longer than 9 seconds to input a character the game will reset\n");
  printf("5. If you lose all 3 lives the game will end\n");
  printf("ENTER LEVEL NOW: \n");
}

void Dot_or_Dash(int state) {
    if(right_answers == 10 && winning_flag == 1) {
        printf("Congratualations, you have completed both levels one and two! You have won the game!\n");
        stats();
        winning_flag = 0;
    }

    if (life != 0) {

    char level_one[] = ".----\0";
    char level_two[] = "..---\0";


    if (buffer[5] == 5 && ans_check == 0 && right_answers != 10) {
        printf("\n");
        printf("Entering check mode...\n");

        //CODE TO PRINT USER INPUT AND POTENTIAL MATCH IN INDEX:
        printf("User Input: %c");
        for(int i =0; i<5; i++) {
            printf("%c", buffer[i]);
        }

        
        printf("\n");

        for(int i = 0; i<5; i++) {
            check[i] = buffer[i];
        }

        for(int a = 0; a<36; a++) {

            for(int i = 0; i<5; i++) {
                morse_check[i] = morse_table[a][i];
        
            }

            if (strcmp(check, morse_check) == 0) {
                printf("User input equivalent: %c\n", char_array[a]);
            }
        }

        
        
        /// END OF USER INPUT PRINTING CODE

        if(input_flag == 0) {
            printf("Wrong due to no input\n");
            count_of_wrong_answers++;
            ans_check = 1;
            life = life -1;
            count_of_lives_used++;
            print_letter_done = 0;

            j = 0;
            restart = 1;
            right_answers = 0;
            input_flag = 0;
            level_selected = 1;

            printf("Lives left: %d\n",life);
            printf("Right answers: %d\n", right_answers);
            update_led();
        }

        for(int i = 0; i < j; i++){
            
            if(ans_check == 0 && input_flag == 1) {

                if(buffer[i] != morse_table[a][i]) {

                    
                    count_of_wrong_answers++;
                    printf("Wrong mate sorry\n");
                    ans_check = 1;
                    life --;
                    count_of_lives_used++;

                    print_letter_done = 0;

                    j = 0;
                    restart = 1;
                    right_answers = 0;
                    input_flag = 0;
                    level_selected = 1;

                    printf("Lives left: %d\n",life);
                    printf("Right answers: %d\n", right_answers);
                    update_led();

                        if(life == 0 ) {
                            end = 1;
                        }
                    break;

                    for(int i =0; i<5; i++) {
                        buffer[i] = 0;
                    }
                }
            }
        }

         if(ans_check == 0 && input_flag == 1) {
                printf("Lets go bruv its right!\n");
                    count_of_correct_answers++;
                    ans_check = 1;
                    print_letter_done = 0;
                    j = 0;
                    restart = 1;
                    right_answers = right_answers + 1;
                    input_flag = 0;

                    if (life != 3) {
                        life = life +1;
                    }

                    printf("Lives left: %d\n",life);
                    printf("Right answers: %d\n", right_answers);
                    update_led();

                    if (right_answers == 5) {
                        level_selected = 2;
                        stats();
                        printf("YOU ARE NOW PROGRESSING TO LEVEL TWO! NO MORSE WILL BE PROVIDED.\n");
                    }

                    for(int i =0; i<5; i++) {
                        buffer[i] = 0;
                    } 
                }

            clear_buffer();
       }



        if(state == 0) {
            printf(".");
            buffer[j] = '.';
            j++;
            watchdog_update();
            input_flag = 1;
            
        }
        else if(state == 1) {
            printf("-");
            buffer[j] = '-';
            j++;
            watchdog_update();
            input_flag = 1;
        }

        else {
            if(level_selected != 0 && restart == 0) {
                    printf(" ");
                    buffer[5] = 5;
                    space_flag = 1;
            }
                
        }
        
        if (j > 4 && level_selected == 0){
            if (strcmp(buffer, level_one) == 0){
                watchdog_update();
                printf("You will now play level 1\n");
                level_selected = 1;
                game_start = 1;
                update_led();
                clear_buffer();
            }
            
            if (strcmp(buffer, level_two) == 0 && level_selected == 0){
                watchdog_update();
                printf("You will now play level 2\n");
                level_selected = 2;
                clear_buffer();
            }
        }
       
            //CODE FOR LEVEL ONE HERE
            if(level_selected == 1 && print_letter_done == 0) {
                watchdog_update();
                a = (rand() % (35 - 0 + 1) + 0 ); // gens random number in index to test
                
                printf("Enter the following %c in Morse code\n", char_array[a]);
                printf("Morse code equivalent:" );

                for(int i = 0; i < 5; i++) {
                    printf("%c", morse_table[a][i]);
                }

                printf("\n");

                print_letter_done = 1; // resets all varaibles so the game can start again! christ
                j = 0;
                restart = 0;
                ans_check = 0;
                buffer[5] = 4;
            }

            if(level_selected == 2 && print_letter_done == 0) {

                a = (rand() % (35 - 0 + 1)); // gens random number in index to test

                
                printf("Enter the following %c in Morse code\n", char_array[a]);
                printf("\n");

                print_letter_done = 1; // resets all varaibles so the game can start again! christ
                j = 0;
                restart = 0;
                ans_check = 0;
                buffer[5] = 4;
            }

        }

        else if (life == 0 && end == 1) {
            printf("GAME OVER XD");
            end = 0;
            stats();
        }

}
    
int main() {
    stdio_init_all();
    // // Initialise the PIO interface with the WS2812 code - just include these three lines in any file using the RGB LED.
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, IS_RGBW);
    watchdog_enable(8388607, 1); // Enable the watchdog timer

    welcome_screen(); // Open the game's welcome screen
    game_start = 0;
    update_led();
    // //update_led(); // Initialise LED before game starts
    main_asm(); // Run assembly code
   // set_level(); // Set level and start the game
  
    return(0);
}
