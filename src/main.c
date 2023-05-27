#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>

#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "gfx_ball.h"
#include "gfx_draw.h"
#include "gfx_font.h"
#include "gfx_event.h"
#include "gfx_sound.h"
#include "gfx_utils.h"
#include "gfx_FreeRTOS_utils.h"
#include "gfx_print.h"

#ifdef TRACE_FUNCTIONS
#include "tracer.h"
#endif

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)
#include <math.h>
#define Gray   (unsigned int)(0x808080)
#define Blue   (unsigned int)(0x0000FF)
#define Green   (unsigned int)(0x00FF00)
static TaskHandle_t DemoTask = NULL;
static TaskHandle_t BufferSwap = NULL;

SemaphoreHandle_t DrawSignal = NULL;


typedef struct buttons_buffer 
{
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;

} buttons_buffer_t;

static buttons_buffer_t buttons = { 0 };
//buttons is global

// typedef struct Mouse
// {
//     SemaphoreHandle_t lock;
//     signed char left_button;
//     signed char right_button;
//     signed char middle_button;
//     signed short x;
//     signed short y;
// }mouse_t;

void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) 
    // lock is the "mutex" in this structure, only after taking it, you can update this structure, after updating, you need to give it back
    {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

void vSwapBuffers(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t frameratePeriod = 20;

    while (1) {
        gfxDrawUpdateScreen();
        gfxEventFetchEvents(FETCH_EVENT_BLOCK);
        xSemaphoreGive(DrawSignal);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(frameratePeriod));
    }
}

void vDemoTask(void *pvParameters)
{
    // structure to store time retrieved from Linux kernel
    // static struct timespec the_time;
    // static char our_timebuttons_buffer _string[100];
    // static int our_time_strings_width = 0;
    // Needed such that Gfx library knows which thread controlls drawing
    // Only one thread can call gfxDrawUpdateScreen while and thread can call
    // the drawing functions to draw objects. This is a limitation of the SDL
    // backend.
    // static struct timespec the_time;
   
    // static char our_time_string[100];
  
    static int string_width = 20;
    static int string_height = 20;

    signed short Xcoord_Circle_Centre = 200;  
    signed short Ycoord_Circle_Centre = 200;
    signed short radius = 50;
    signed short Xcoord_Square_LP = 350;
    signed short Ycoord_Square_LP = 150;
    signed short Square_Width = 100;
    signed short Square_Height = 100;
    

    coord_t Triangle_Points[3];
    Triangle_Points[0].x = 250;
    Triangle_Points[1].x = 300;
    Triangle_Points[2].x = 350;
    Triangle_Points[0].y = 150;
    Triangle_Points[1].y = 250;
    Triangle_Points[2].y = 150;
    double theta = 0.01;

    signed short Xcoord_rotateC = 300;
    signed short Ycoord_rotateC = 200;
    signed short Xcoord_CCD = Xcoord_Circle_Centre-Xcoord_rotateC; // X Circle Center distance
    signed short Ycoord_CCD = Ycoord_Circle_Centre-Ycoord_rotateC; // Y Circle Center distance
    signed short Xcoord_SCD = Xcoord_Square_LP-Ycoord_rotateC;
    signed short Ycoord_SCD = Ycoord_Square_LP-Ycoord_rotateC;

    short MoveDirection = 0; // 0 refers to the string move to right
    unsigned short XString_Left = 0; // the left end of string
    int speed = 50; // moving speed of string

    char str1[12] = "Hello World";
    char str2[11] = "Hello ESPL";
    char mouse_Xcoord[20];
    char mouse_Ycoord[20];
    char mouse_coord[20];
    signed short Mouse_Xcoord;
    signed short Mouse_Ycoord;
    

    int PressTimesA = 0;
    int PressTimesB = 0;
    int PressTimesC = 0;
    int PressTimesD = 0;
    // static mouse_t mouse;

   
    static  const TickType_t debounceDelay = 5;
    TickType_t last_change = xTaskGetTickCount();

    gfxDrawBindThread();

    while (1) {
        if (DrawSignal) // To verify if there is a semaphore handle
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
            // Note: The Drawsignal is a different handle to lock
            // Drawhandle is a queue handle, meaning if you want to access the contents of the queue
            // you should first take the semaphore
            {
                gfxEventFetchEvents(FETCH_EVENT_NONBLOCK); 
                //retrieve all outstanding SDL Events. (eg. Pressing Buttons) 
                //this function is invoked only after taking the semaphore of the queue
             
                
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) 
                {
                    
                    if(xQueueReceive(buttonInputQueue, &buttons.buttons, 0) == pdTRUE)
                    {
                        if(xTaskGetTickCount()-last_change > debounceDelay)
                            {
                                if (buttons.buttons[KEYCODE(Q)]) //because the macro #define KEYCODE(CHAR) SDL_SCANCODE_##CHAR
                                     { // Equiv to SDL_SCANCODE_Q
                                        exit(EXIT_SUCCESS);
                                     }
                                        
                                else if (buttons.buttons[KEYCODE(A)]) 
                                    { // Equiv to SDL_SCANCODE_A
                                         PressTimesA++;
                                         printf("A is pressed for %d times\n",PressTimesA);
                                    }
            
                                else if (buttons.buttons[KEYCODE(B)]) 
                                    { // Equiv to SDL_SCANCODE_A
                                        PressTimesB++;
                                        printf("B is pressed for %d times\n",PressTimesB);
                                    }
                
                                else if (buttons.buttons[KEYCODE(C)]) 
                                    { // Equiv to SDL_SCANCODE_A
                                        PressTimesC++;
                                        printf("C is pressed for %d times\n",PressTimesC); 
                                    }
                                    else if (buttons.buttons[KEYCODE(D)]) 
                                    { // Equiv to SDL_SCANCODE_A
                                        PressTimesD++;
                                        printf("D is pressed for %d times\n",PressTimesD); 
                                    }
                                last_change = xTaskGetTickCount();

                            }
                    
                     }
                    xSemaphoreGive(buttons.lock);
                 }
                 
                 if(xTaskGetTickCount()-last_change > debounceDelay)
                {
                // if (xSemaphoreTake(mouse.lock,0)==pdTRUE)
                // {
                        /*mouse.*/signed char left_button = gfxEventGetMouseLeft();
                        /*mouse.*/signed char middle_button = gfxEventGetMouseMiddle();
                        /*mouse.*/signed char right_button = gfxEventGetMouseRight();
                        if (/*mouse.*/left_button  || /*mouse.*/middle_button || /*mouse.*/right_button)
                        {
                            PressTimesA = 0;
                            PressTimesB = 0;
                            PressTimesC = 0;
                            PressTimesD = 0;
                            // mouse.x = gfxEventGetMouseX();
                            // mouse.y = gfxEventGetMouseY();
                            // sprintf(mouse_Xcoord,"%hd",mouse.x);
                            // sprintf(mouse_Ycoord,"%hd",mouse.y);
                            // strcpy(mouse_coord,mouse_Xcoord);
                            // strcpy(mouse_coord,mouse_Ycoord);
                            last_change = xTaskGetTickCount();
                        }
                       
                //         xSemaphoreGive(mouse.lock);
                // }
                }
                        Mouse_Xcoord = gfxEventGetMouseX();
                        Mouse_Ycoord = gfxEventGetMouseY();
                        Xcoord_rotateC = Mouse_Xcoord;
                        Ycoord_rotateC = Mouse_Ycoord;
                        Triangle_Points[0].x = Xcoord_rotateC - 50;
                        Triangle_Points[1].x = Xcoord_rotateC;
                        Triangle_Points[2].x = Xcoord_rotateC + 50;

                        Triangle_Points[0].y = Ycoord_rotateC - 50;
                        Triangle_Points[2].y = Ycoord_rotateC - 50;
                        Triangle_Points[1].y = Ycoord_rotateC + 50;
                        sprintf(mouse_coord,"%hd,%hd",Mouse_Xcoord,Mouse_Ycoord);
                        //sprintf(mouse_Ycoord,"%hd",);
                        //strcpy(mouse_coord,mouse_Xcoord);
                        //strcpy(mouse_coord,mouse_Ycoord);


       
                gfxDrawClear(White); // Clear screen
                theta = theta + 0.2; // rotation angle theta
                double c = cos(theta);
                double s = sin(theta);
                
                Xcoord_Circle_Centre = (c * Xcoord_CCD - s * Ycoord_CCD) + Xcoord_rotateC;
                Ycoord_Circle_Centre = (s * Xcoord_CCD + c * Ycoord_CCD) + Ycoord_rotateC;

                Xcoord_Square_LP = (c * Xcoord_SCD - s * Ycoord_SCD) + Xcoord_rotateC;
                Ycoord_Square_LP = (s * Xcoord_SCD + c * Ycoord_SCD) + Ycoord_rotateC;

                
                // // Get the width of the string on the screen so we can center it
                // // Returns 0 if width was successfully obtained

                if (!gfxGetTextSize((char *)str1, &string_width, &string_height))
                {
                    gfxDrawText(str1, Xcoord_Circle_Centre-radius, Ycoord_Circle_Centre+radius,Black);
                    gfxDrawText(str1, Xcoord_Square_LP , Ycoord_Square_LP+Square_Height,Black);
                    
                }
                
                if(MoveDirection == 0)
                {
                    XString_Left = XString_Left + speed;
                    if(XString_Left + string_width >= SCREEN_WIDTH)
                    {
                        MoveDirection = 1;                       strcpy(mouse_coord,mouse_Xcoord);
                        strcpy(mouse_coord,mouse_Ycoord);
                    }
                }
                else
                {
                   XString_Left = XString_Left - speed; 
                   if(XString_Left == 0)
                    {
                        MoveDirection = 0;
                    }
                }

                if (!gfxGetTextSize((char *)str2, &string_width, &string_height))
                {
                    gfxDrawText(str2,XString_Left ,Ycoord_rotateC-2*Square_Height,Black);

                }

                if (!gfxGetTextSize((char*)mouse_coord, &string_width, &string_height))
                { 
                        gfxDrawText(mouse_coord,Mouse_Xcoord, Mouse_Ycoord+50,Black);
                        
                    
                }
                // if (!gfxGetTextSize((char*)mouse_coord, &string_width, &string_height))
                // {   
                //     if(xSemaphoreTake(mouse.lock,0)==pdTRUE)
                //     {
                //         gfxDrawText(mouse_coord,mouse.x,mouse.y,Black);
                //         xSemaphoreGive(mouse.lock);
                //     }
                // }
                 gfxDrawCircle(Xcoord_Circle_Centre,Ycoord_Circle_Centre,radius,Gray);
                gfxDrawFilledBox(Xcoord_Square_LP,Ycoord_Square_LP,Square_Width,Square_Height,Blue);
                gfxDrawTriangle(Triangle_Points, Green);
                gfxDrawUpdateScreen(); // Refresh the screen to draw string
            }
    }
}

int main(int argc, char *argv[])
{
    char *bin_folder_path = gfxUtilGetBinFolderPath(argv[0]);

    prints("Initializing: ");

    //  Note PRINT_ERROR is not thread safe and is only used before the
    //  scheduler is started. There are thread safe print functions in
    //  gfx_Print.h, `prints` and `fprints` that work exactly the same as
    //  `printf` and `fprintf`. So you can read the documentation on these
    //  functions to understand the functionality.

    if (gfxDrawInit(bin_folder_path)) {
        PRINT_ERROR("Failed to intialize drawing");
        goto err_init_drawing;
    }
    else {
        prints("drawing");
    }

    if (gfxEventInit()) {
        PRINT_ERROR("Failed to initialize events");
        goto err_init_events;
    }
    else {
        prints(", events");
    }

    if (gfxSoundInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize audio");
        goto err_init_audio;
    }
    else {
        prints(", and audio\n");

        buttons.lock = xSemaphoreCreateMutex(); // Locking mechanism
        if (!buttons.lock) {
            PRINT_ERROR("Failed to create buttons lock");
            goto err_buttons_lock;
        }
    }

    if (gfxSafePrintInit()) {
        PRINT_ERROR("Failed to init safe print");
        goto err_init_safe_print;
    }

    DrawSignal = xSemaphoreCreateBinary(); // Screen buffer locking
    if (!DrawSignal) {
        PRINT_ERROR("Failed to create draw signal");
        goto err_draw_signal;
    }

    if (xTaskCreate(vDemoTask, "DemoTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    mainGENERIC_PRIORITY, &DemoTask) != pdPASS) {
        goto err_demotask;
    }

    if (xTaskCreate(vSwapBuffers, "BufferSwapTask",
                    mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES,
                    &BufferSwap) != pdPASS) {
        PRINT_TASK_ERROR("BufferSwapTask");
        goto err_bufferswap;
    }

    vTaskStartScheduler();

    return EXIT_SUCCESS;

err_bufferswap:
    vTaskDelete(DemoTask);
err_demotask:
    vSemaphoreDelete(DrawSignal);
err_draw_signal:
    gfxSafePrintExit();
err_init_safe_print:
    vSemaphoreDelete(buttons.lock);
err_buttons_lock:
    gfxSoundExit();
err_init_audio:
    gfxEventExit();
err_init_events:
    gfxDrawExit();
err_init_drawing:
    return EXIT_FAILURE;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
