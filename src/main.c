#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>

#include <SDL2/SDL_scancode.h>
#include "portmacro.h"
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
#include "draw.h"
#include <math.h>

#ifdef TRACE_FUNCTIONS
#include "tracer.h"
#endif

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)
#define STACK_SIZE 10
#define STATE_QUEUE_LENGTH 1
#define PRESS_QUEUE_LENGTH 1

#define Gray   (unsigned int)(0x808080)
#define Blue   (unsigned int)(0x0000FF)
#define Green   (unsigned int)(0x00FF00)
#define StateOne 0
#define StateTwo 1
#define StateThree 2
#define StateFour 3
#define StateFive 4
#define StartingState StateOne
#define NEXT_TASK 1
#define PREV_TASK 0
#define STATE_COUNT 5
#define TASK_STOP 0

static StaticTask_t TaskControlBlock; 
// The structure to hold the task control block of the statically created task
static StackType_t  Stack_Buffer[STACK_SIZE];//The statically created task use this buffer as its stack

static TaskHandle_t BufferSwap = NULL;
static TaskHandle_t StateMachine = NULL;
static TaskHandle_t Exercise_two = NULL;
static TaskHandle_t Exercise_three = NULL;
static TaskHandle_t Second_toggle_cir = NULL;
static TaskHandle_t Display_G = NULL;
static TaskHandle_t Display_H = NULL;
static TaskHandle_t Semaphore_Trigger = NULL;
static TaskHandle_t TaskNotify_Trigger = NULL;
static TaskHandle_t Reset_Number = NULL;
static TaskHandle_t Increase_Variable = NULL;
static TaskHandle_t Resume_Increase_Variable = NULL;
static TaskHandle_t Task_one, Task_two, Task_three, Task_four, Output_Task;

static SemaphoreHandle_t Sema_Wake_Three;
static SemaphoreHandle_t Binary_Sema;




static QueueHandle_t StateQueue = NULL;
static QueueHandle_t Press_Queue = NULL;
SemaphoreHandle_t DrawSignal = NULL;



typedef struct buttons_buffer 
{
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;

} buttons_buffer_t;

static buttons_buffer_t buttons = { 0 };

//buttons is global

void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) 
    // lock is the "mutex" in this structure, only after taking it, you can update this structure, after updating, you need to give it back
    {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

int CheckStateInput(void)
{
    unsigned char NextStateSignal = NEXT_TASK;
    
    if(xSemaphoreTake(buttons.lock,0)==pdTRUE)
    {
    
        if(buttons.buttons[KEYCODE(E)])
        { buttons.buttons[KEYCODE(E)] = 0;
            
            if(StateQueue)
                {
                    xSemaphoreGive(buttons.lock);
                    xQueueSend(StateQueue,&NextStateSignal,0);
                    return 0;
                }
        }
        
        //}

        xSemaphoreGive(buttons.lock); 
    }
    return -1; 
}

void changeState(unsigned char* state, unsigned char change)
{
    switch (change)
    {
    case NEXT_TASK:
        if(*state == STATE_COUNT-1)
        {
            *state = 0;
        }
        else
        {
            (*state)++;
        }
        break;
    case PREV_TASK:
        if(*state == 0)
        {
            *state = STATE_COUNT-1;
        }
        else
        {
            (*state)--;
        }
        break;
    default:
        break;
    }
}

static char Output_Buffer1[20]= {0};
static char Output_Buffer2[20]= {0};
static char Output_Buffer3[20]= {0};
static char Output_Buffer4[20]= {0};

void TaskOne(void *pvParameters)
{
    TickType_t LastWake = xTaskGetTickCount();
    TickType_t Initial = xTaskGetTickCount();
    TickType_t Output_Period = 1;
    int counter = 0;
    while(1)
    {   
        printf("Task one %d\n",xTaskGetTickCount());
        sprintf(&Output_Buffer1[counter],"1");
        counter++;
        if(xTaskGetTickCount()-Initial>=15)
        {
            vTaskDelete(Task_one);
            break;
        }
        vTaskDelayUntil(&LastWake,Output_Period);
        LastWake = xTaskGetTickCount();
        
    }
}

void TaskTwo(void *pvParameters)
{
    TickType_t LastWake = xTaskGetTickCount();
    TickType_t Initial = xTaskGetTickCount();
    TickType_t Output_Period = 2;
    int counter = 1;
    while(1)
    {
        printf("Task two %d\n",xTaskGetTickCount());
        vTaskDelayUntil(&LastWake,Output_Period);
        sprintf(&Output_Buffer2[counter],"2");
        counter = counter + 2;
         if(xTaskGetTickCount()-Initial>=15)
        {
            vTaskDelete(Task_two);
            break;
        }
        xSemaphoreGive(Sema_Wake_Three);
        LastWake = xTaskGetTickCount();
    }
}

void TaskThree(void *pvParameters)
{
    TickType_t Initial = xTaskGetTickCount();
    TickType_t Output_Period = 2;
    int counter = 3;
    while(1)
    {
        printf("Task three %d\n",xTaskGetTickCount());
        xSemaphoreTake(Sema_Wake_Three,portMAX_DELAY);
        sprintf(&Output_Buffer3[counter],"3");
        counter = counter + Output_Period;
        if(xTaskGetTickCount()-Initial>=15)
        {
            vTaskDelete(Task_three);
            break;
        }
    }
}

void TaskFour(void *pvParameters)
{
    TickType_t LastWake = xTaskGetTickCount();
    TickType_t Initial = xTaskGetTickCount();
    TickType_t Output_Period = 4;
    int counter = 3;

        while(1)
    {
        printf("Task four %d\n",xTaskGetTickCount());
        vTaskDelayUntil(&LastWake,Output_Period);
        sprintf(&Output_Buffer4[counter],"4");
        counter = counter + Output_Period;
        if(xTaskGetTickCount()-Initial>=15)
        {
            vTaskDelete(Task_four);
            break;
        }
        LastWake = xTaskGetTickCount();
    }
}

void OutputTask(void *pvParameters)
{
    TickType_t LastWake = xTaskGetTickCount();

    static int string_width = 20;
    static int string_height = 20;
    signed short xString = SCREEN_WIDTH/2 - 200;
    signed short YString1 = 10;
    signed short line_distance = 15;
    char Tick1[50];
    char Tick2[50];
    char Tick3[50];
    char Tick4[50];
    char Tick5[50];
    char Tick6[50];
    char Tick7[50];
    char Tick8[50];
    char Tick9[50];
    char Tick10[50];
    char Tick11[50];
    char Tick12[50];
    char Tick13[50];
    char Tick14[50];
    char Tick15[50];
    gfxDrawBindThread();
   
    while (1)
    {
        if(xSemaphoreTake(DrawSignal,portMAX_DELAY)==pdTRUE)
        {
            gfxDrawClear(White);
            if(LastWake==xTaskGetTickCount())
            {
                xTaskDelayUntil(&LastWake,16);
            }
            sprintf(Tick1,"Tick 1 :%c %c %c %c",Output_Buffer1[0],Output_Buffer2[0],Output_Buffer3[0],Output_Buffer4[0]);
            sprintf(Tick2,"Tick 2 :%c %c %c %c",Output_Buffer1[1],Output_Buffer2[1],Output_Buffer3[1],Output_Buffer4[1]);
            sprintf(Tick3,"Tick 3 :%c %c %c %c",Output_Buffer1[2],Output_Buffer2[2],Output_Buffer3[2],Output_Buffer4[2]);
            sprintf(Tick4,"Tick 4 :%c %c %c %c",Output_Buffer1[3],Output_Buffer2[3],Output_Buffer3[3],Output_Buffer4[3]);
            sprintf(Tick5,"Tick 5 :%c %c %c %c",Output_Buffer1[4],Output_Buffer2[4],Output_Buffer3[4],Output_Buffer4[4]);
            sprintf(Tick6,"Tick 6 :%c %c %c %c",Output_Buffer1[5],Output_Buffer2[5],Output_Buffer3[5],Output_Buffer4[5]);
            sprintf(Tick7,"Tick 7 :%c %c %c %c",Output_Buffer1[6],Output_Buffer2[6],Output_Buffer3[6],Output_Buffer4[6]);
            sprintf(Tick8,"Tick 8 :%c %c %c %c",Output_Buffer1[7],Output_Buffer2[7],Output_Buffer3[7],Output_Buffer4[7]);
            sprintf(Tick9,"Tick 9 :%c %c %c %c",Output_Buffer1[8],Output_Buffer2[8],Output_Buffer3[8],Output_Buffer4[8]);
            sprintf(Tick10,"Tick 10 :%c %c %c %c",Output_Buffer1[9],Output_Buffer2[9],Output_Buffer3[9],Output_Buffer4[9]);
            sprintf(Tick11,"Tick 11 :%c %c %c %c",Output_Buffer1[10],Output_Buffer2[10],Output_Buffer3[10],Output_Buffer4[10]);
            sprintf(Tick12,"Tick 12 :%c %c %c %c",Output_Buffer1[11],Output_Buffer2[11],Output_Buffer3[11],Output_Buffer4[11]);
            sprintf(Tick13,"Tick 13 :%c %c %c %c",Output_Buffer1[12],Output_Buffer2[12],Output_Buffer3[12],Output_Buffer4[12]);
            sprintf(Tick14,"Tick 14 :%c %c %c %c",Output_Buffer1[13],Output_Buffer2[13],Output_Buffer3[13],Output_Buffer4[13]);
            sprintf(Tick15,"Tick 15 :%c %c %c %c",Output_Buffer1[14],Output_Buffer2[14],Output_Buffer3[14],Output_Buffer4[14]);

            if(!gfxGetTextSize(Tick1,&string_width,&string_height))
            {
                gfxDrawText(Tick1,xString,YString1,Black);
            }
            if(!gfxGetTextSize(Tick2,&string_width,&string_height))
            {
                gfxDrawText(Tick2,xString,YString1+line_distance,Black);
            }
             if(!gfxGetTextSize(Tick3,&string_width,&string_height))
            {
                gfxDrawText(Tick3,xString,YString1+2*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick4,&string_width,&string_height))
            {
                gfxDrawText(Tick4,xString,YString1+3*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick5,&string_width,&string_height))
            {
                gfxDrawText(Tick5,xString,YString1+4*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick6,&string_width,&string_height))
            {
                gfxDrawText(Tick6,xString,YString1+5*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick7,&string_width,&string_height))
            {
                gfxDrawText(Tick7,xString,YString1+6*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick8,&string_width,&string_height))
            {
                gfxDrawText(Tick8,xString,YString1+7*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick9,&string_width,&string_height))
            {
                gfxDrawText(Tick9,xString,YString1+8*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick10,&string_width,&string_height))
            {
                gfxDrawText(Tick10,xString,YString1+9*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick11,&string_width,&string_height))
            {
                gfxDrawText(Tick11,xString,YString1+10*line_distance,Black);
            } 
            if(!gfxGetTextSize(Tick12,&string_width,&string_height))
            {
                gfxDrawText(Tick12,xString,YString1+11*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick13,&string_width,&string_height))
            {
                gfxDrawText(Tick13,xString,YString1+12*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick14,&string_width,&string_height))
            {
                gfxDrawText(Tick14,xString,YString1+13*line_distance,Black);
            }
            if(!gfxGetTextSize(Tick15,&string_width,&string_height))
            {
                gfxDrawText(Tick15,xString,YString1+14*line_distance,Black);
            }
            gfxDrawUpdateScreen();

        }

    }
    
    
}



void vSwapBuffers(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t frameratePeriod = 15;

    while (1) {
        gfxDrawUpdateScreen();
        gfxEventFetchEvents(FETCH_EVENT_BLOCK);
        xSemaphoreGive(DrawSignal);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(frameratePeriod));
    }
}


void SequentialStateMachine(void *pvParameters)
{
    unsigned char current_state =  StartingState;
    unsigned char state_changed = 1;
    unsigned char input = 0;

    
    while (1)
    {   
        if(state_changed)
        {
            goto handle_state; // firstly handle current state
        }
       
        if(StateQueue)
        {
            //printf("StateQueue checked\n");
            if(xQueueReceive(StateQueue,&input,portMAX_DELAY)==pdTRUE) 
            //receive data from StateQueue and store it in input
            {
                //if(xTaskGetTickCount()-last_change>debounceDelay)
                //{
                    changeState(&current_state,input);
                    state_changed = 1;
                    //last_change = xTaskGetTickCount();
                //}
                    
            }
        }
    handle_state:
        if(state_changed)
        {
            switch (current_state)
            {
                case (StateOne): 
                    if(Exercise_two)
                    {
                        vTaskResume(Exercise_two);
                    }
                    if(Exercise_three)
                    {
                        vTaskSuspend(Exercise_three);
                    }
                    if(Second_toggle_cir)
                    {
                        vTaskSuspend(Second_toggle_cir);
                    }
                    if(Display_G)
                    {
                        vTaskSuspend(Display_G);
                    }
                    if(Semaphore_Trigger)
                    {
                        vTaskSuspend(Semaphore_Trigger);
                    }
                    if(Display_H)
                    {
                        vTaskSuspend(Display_H);
                    }
                    if(TaskNotify_Trigger)
                    {
                        vTaskSuspend(TaskNotify_Trigger);
                    }
                    if(Reset_Number)
                    {
                        vTaskSuspend(Reset_Number);
                    }
                     if(Increase_Variable)
                    {
                        vTaskSuspend(Increase_Variable);
                    }
                    if(Resume_Increase_Variable)
                    {
                        vTaskSuspend(Resume_Increase_Variable);
                    }
                    if(Task_one||Task_two||Task_three||Task_four||Output_Task)
                    {
                        vTaskSuspend(Task_one);
                        vTaskSuspend(Task_two);
                        vTaskSuspend(Task_three);
                        vTaskSuspend(Task_four);
                        vTaskSuspend(Output_Task);
                    }
                    break;
                case (StateTwo): // State2 corresponds to exercise3
                    if(Exercise_two)
                    {
                        vTaskSuspend(Exercise_two);
                    }
                    if(Exercise_three)
                    {
                        vTaskResume(Exercise_three);
                    }
                    if(Second_toggle_cir)
                    {
                        vTaskResume(Second_toggle_cir);
                    }
                    if(Display_G)
                    {
                        vTaskSuspend(Display_G);
                    }
                    if(Semaphore_Trigger)
                    {
                        vTaskSuspend(Semaphore_Trigger);
                    }
                    if(Display_H)
                    {
                        vTaskSuspend(Display_H);
                    }
                    if(TaskNotify_Trigger)
                    {
                        vTaskSuspend(TaskNotify_Trigger);
                    }
                    if(Reset_Number)
                    {
                        vTaskSuspend(Reset_Number);
                    }
                    if(Increase_Variable)
                    {
                        vTaskSuspend(Increase_Variable);
                    }
                    if(Resume_Increase_Variable)
                    {
                        vTaskSuspend(Resume_Increase_Variable);
                    }
                    if(Task_one||Task_two||Task_three||Task_four||Output_Task)
                    {
                        vTaskSuspend(Task_one);
                        vTaskSuspend(Task_two);
                        vTaskSuspend(Task_three);
                        vTaskSuspend(Task_four);
                        vTaskSuspend(Output_Task);
                    }
                    break;
                case (StateThree):
                    if(Exercise_two)
                    {
                        vTaskSuspend(Exercise_two);
                    }
                    if(Exercise_three)
                    {
                        vTaskSuspend(Exercise_three);
                    }
                    if(Second_toggle_cir)
                    {
                        vTaskSuspend(Second_toggle_cir);
                    }
                     if(Display_G)
                    {
                        vTaskResume(Display_G);
                    }
                    if(Semaphore_Trigger)
                    {
                        vTaskResume(Semaphore_Trigger);
                    }
                    if(Display_H)
                    {
                        vTaskResume(Display_H);
                    }
                    if(TaskNotify_Trigger)
                    {
                        vTaskResume(TaskNotify_Trigger);
                    }
                     if(Reset_Number)
                    {
                        vTaskResume(Reset_Number);
                    }
                    if(Increase_Variable)
                    {
                        vTaskSuspend(Increase_Variable);
                    }
                    if(Resume_Increase_Variable)
                    {
                        vTaskSuspend(Resume_Increase_Variable);
                    }
                    if(Task_one||Task_two||Task_three||Task_four||Output_Task)
                    {
                        vTaskSuspend(Task_one);
                        vTaskSuspend(Task_two);
                        vTaskSuspend(Task_three);
                        vTaskSuspend(Task_four);
                        vTaskSuspend(Output_Task);
                    }
                    break;
                case (StateFour):
                    if(Exercise_two)
                    {
                        vTaskSuspend(Exercise_two);
                    }
                    if(Exercise_three)
                    {
                        vTaskSuspend(Exercise_three);
                    }
                    if(Second_toggle_cir)
                    {
                        vTaskSuspend(Second_toggle_cir);
                    }
                    if(Display_G)
                    {
                        vTaskSuspend(Display_G);
                    }
                    if(Semaphore_Trigger)
                    {
                        vTaskSuspend(Semaphore_Trigger);
                    }
                    if(Display_H)
                    {
                        vTaskSuspend(Display_H);
                    }
                    if(TaskNotify_Trigger)
                    {
                        vTaskSuspend(TaskNotify_Trigger);
                    }
                    if(Reset_Number)
                    {
                        vTaskSuspend(Reset_Number);
                    }
                    if(Increase_Variable)
                    {
                        vTaskResume(Increase_Variable);
                    }
                    if(Resume_Increase_Variable)
                    {
                        vTaskResume(Resume_Increase_Variable);
                    }
                    if(Task_one||Task_two||Task_three||Task_four||Output_Task)
                    {
                        vTaskSuspend(Task_one);
                        vTaskSuspend(Task_two);
                        vTaskSuspend(Task_three);
                        vTaskSuspend(Task_four);
                        vTaskSuspend(Output_Task);
                    }
                    break;
                case (StateFive):
                    if(Exercise_two)
                    {
                        vTaskSuspend(Exercise_two);
                    }
                    if(Exercise_three)
                    {
                        vTaskSuspend(Exercise_three);
                    }
                    if(Second_toggle_cir)
                    {
                        vTaskSuspend(Second_toggle_cir);
                    }
                    if(Display_G)
                    {
                        vTaskSuspend(Display_G);
                    }
                    if(Semaphore_Trigger)
                    {
                        vTaskSuspend(Semaphore_Trigger);
                    }
                    if(Display_H)
                    {
                        vTaskSuspend(Display_H);
                    }
                    if(TaskNotify_Trigger)
                    {
                        vTaskSuspend(TaskNotify_Trigger);
                    }
                    if(Reset_Number)
                    {
                        vTaskSuspend(Reset_Number);
                    }
                    if(Increase_Variable)
                    {
                        vTaskSuspend(Increase_Variable);
                    }
                    if(Resume_Increase_Variable)
                    {
                        vTaskSuspend(Resume_Increase_Variable);
                    }
                    if(Task_one||Task_two||Task_three||Task_four||Output_Task)
                    {
                        vTaskResume(Task_one);
                        vTaskResume(Task_two);
                        vTaskResume(Task_three);
                        vTaskResume(Task_four);
                        vTaskResume(Output_Task);
                    }
                    break;

                default:
                    break;
            }
            state_changed = 0;
        } 
    }
}

void ExerciseThree(void *pvParameters)
{
    signed short Xcoord_Circle_Centre = SCREEN_WIDTH/2;  
    signed short Ycoord_Circle_Centre = SCREEN_HEIGHT/2;
    signed short radius = 25;
    int flag = 1;
    TickType_t Delay = 500;
    TickType_t last_change = xTaskGetTickCount();
     
    gfxDrawBindThread();
      while (1) 
    {
        
        if (DrawSignal) // To verify if there is a semaphore handle
        {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
            {
                gfxDrawClear(White);
                vDrawFPS();
                if(flag==1)
                {
                    gfxDrawCircle(Xcoord_Circle_Centre+4*radius,Ycoord_Circle_Centre,radius,Blue);
                }
                
                if(xTaskGetTickCount()-last_change>=Delay)
                {
                    flag = !flag;
                    last_change = xTaskGetTickCount();
                }
                gfxDrawUpdateScreen();
                gfxEventFetchEvents(FETCH_EVENT_NONBLOCK); 
                xGetButtonInput();
            }
            CheckStateInput();  
        }
        
    }
}

void SecondToggleCircle(void *pvParameters)
{
    signed short Xcoord_Circle_Centre = SCREEN_WIDTH/2;  
    signed short Ycoord_Circle_Centre = SCREEN_HEIGHT/2;
    signed short radius = 25;
    int flag = 1;
    TickType_t Delay = 250;
    TickType_t last_change = xTaskGetTickCount();
     
    gfxDrawBindThread();
      while (1) 
    {
        if (DrawSignal) // To verify if there is a semaphore handle
        {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
            {
                
                if(flag==1)
                {
                    gfxDrawClear(White);
                    gfxDrawCircle(Xcoord_Circle_Centre-4*radius,Ycoord_Circle_Centre,radius,Black);
                }
                vDrawFPS();
                if(xTaskGetTickCount()-last_change>=Delay)
                {
                    flag = !flag;
                    last_change = xTaskGetTickCount();
                }
                gfxDrawUpdateScreen();
                gfxEventFetchEvents(FETCH_EVENT_NONBLOCK); 
                xGetButtonInput();
            }
            CheckStateInput();  
        }
        
    }

}

void DisplayG(void *pvParameters)
{
    static int string_width = 20;
    static int string_height = 20;
    char Counter[50];
    int counter = 0;
    signed short xString = SCREEN_WIDTH/2 - 200;
    signed short yString = SCREEN_HEIGHT/2 + 100;
    TickType_t last_change = xTaskGetTickCount();
    TickType_t debounce_delay = 5;
    TickType_t last_reset = xTaskGetTickCount();
    TickType_t reset_period = 15000;
    gfxDrawBindThread();
    xSemaphoreTake(Binary_Sema,portMAX_DELAY);
    //printf("111\n"); 
    while(1)
    {
            if(xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
        {
            gfxDrawClear(White);
            gfxEventFetchEvents(FETCH_EVENT_NONBLOCK);
            //printf("111\n");
            if(xSemaphoreTake(buttons.lock,0)==pdTRUE)
            {     
                if(xQueueReceive(buttonInputQueue, &buttons.buttons, 0) == pdTRUE)
                {
                    if(xTaskGetTickCount()-last_change>debounce_delay)
                    {
                        if(buttons.buttons[KEYCODE(G)])
                        {
                            counter = counter+1;
                            last_change = xTaskGetTickCount();

                        }
                        
                    }
                }    
                xSemaphoreGive(buttons.lock);
            }

            if(xTaskGetTickCount()-last_reset>reset_period)
                {
                    xQueueReceive(Press_Queue,&counter,0);
                    last_reset = xTaskGetTickCount();
                }

            sprintf(Counter,"Times that G is pressed: %d",counter);
            if (!gfxGetTextSize(Counter, &string_width, &string_height))
            {
                gfxDrawText(Counter,xString ,yString,Black);

            }
            vDrawFPS();
            gfxDrawUpdateScreen();
        CheckStateInput();
        }
        
    }   
        
}


void SemaphoreTrigger(void *pvParameters)
{
    gfxDrawBindThread();
    while(1)
    {
        // printf("the trigger is activated\n");
        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
        {   
            gfxDrawClear(White);
            vDrawFPS();
            gfxEventFetchEvents(FETCH_EVENT_NONBLOCK);   
            xSemaphoreTake(Binary_Sema,0);
            xGetButtonInput();
                if(xSemaphoreTake(buttons.lock,0)==pdTRUE)
                {
                    //printf("222\n");
                    if(buttons.buttons[KEYCODE(G)])
                    {
                        //printf("111\n");
                        xSemaphoreGive(Binary_Sema);
                        xSemaphoreGive(buttons.lock);
                        vTaskSuspend(Semaphore_Trigger);
                    }
                    xSemaphoreGive(buttons.lock);
                }
            gfxDrawUpdateScreen();
            CheckStateInput();
        }
    }
}

void TaskNotifyTrigger(void *pvParameters)
{
    gfxDrawBindThread();
    while (1)
    {
        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
        {
            
            gfxEventFetchEvents(FETCH_EVENT_NONBLOCK);   
            xGetButtonInput();
            if(xSemaphoreTake(buttons.lock,0)==pdTRUE)
                {
                    if(buttons.buttons[KEYCODE(H)])
                    {
                        buttons.buttons[KEYCODE(H)]=0;
                        xTaskNotifyGive(Display_H);
                        xSemaphoreGive(buttons.lock);
                    }
                    xSemaphoreGive(buttons.lock);
                }
            CheckStateInput();
        }
    }
    
    
}

void DisplayH(void *pvParameters)
{
    static int string_width = 20;
    static int string_height = 20;
    int counter = 0;
    char Counter[50];
    signed short xString = SCREEN_WIDTH/2 + 100;
    signed short yString = SCREEN_HEIGHT/2 + 100;
    TickType_t last_change = xTaskGetTickCount();
    TickType_t debounce_delay = 5;
    TickType_t last_reset = xTaskGetTickCount();
    TickType_t reset_period = 15000;
    ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    //printf("The task is triggered\n");   
    gfxDrawBindThread();
    while(1)
    {
        if (xSemaphoreTake(DrawSignal, portMAX_DELAY) == pdTRUE)
        {
            gfxEventFetchEvents(FETCH_EVENT_NONBLOCK);
            gfxDrawClear(White);
            vDrawFPS();
                if(xSemaphoreTake(buttons.lock,0)==pdTRUE)
                {     
                    if(xQueueReceive(buttonInputQueue, &buttons.buttons, 0) == pdTRUE)
                    {
                        if(xTaskGetTickCount()-last_change>debounce_delay)
                        {
                            if(buttons.buttons[KEYCODE(H)])
                            {
                                counter = counter+1;
                                last_change = xTaskGetTickCount();
                            }
                            
                        }
                    }    
                    xSemaphoreGive(buttons.lock);
                }
                if(xTaskGetTickCount()-last_reset>reset_period)
                {
                    xQueueReceive(Press_Queue,&counter,0);
                    last_reset = xTaskGetTickCount();
                }
                vDrawFPS();
                sprintf(Counter,"Times that H is pressed: %d",counter);
                if (!gfxGetTextSize(Counter, &string_width, &string_height))
                {
                    gfxDrawText(Counter,xString ,yString,Black);

                }
                gfxDrawUpdateScreen();
            CheckStateInput();
            }
            
    }   
        
}

void ResetNumber(void *pvParameters)
{
    TickType_t last_change = xTaskGetTickCount();
    TickType_t Reset_Period = 15000;
    int reset = 0;
    while(1)
    {
        if(xTaskGetTickCount()-last_change < Reset_Period)
        {
            vTaskDelayUntil(&last_change,Reset_Period);
            last_change = xTaskGetTickCount();
            printf("The press times are reset\n");
            xQueueSend(Press_Queue,&reset,0);
        }
    }
}


void IncreaseVariable(void *pvParameters)
{
    TickType_t last_increase = xTaskGetTickCount();
    TickType_t increase_period = 1000;
    TickType_t last_press = xTaskGetTickCount();
    TickType_t Debounce_delay = 5;
    int variable = 0;
    char Variable[50];
    static int string_width = 20;
    static int string_height = 20;
    int xString = SCREEN_WIDTH/2 - 100;
    gfxDrawBindThread();
    while(1)
    {
        if(xSemaphoreTake(DrawSignal,portMAX_DELAY) == pdTRUE)
        {
            gfxDrawClear(White);
            gfxEventFetchEvents(FETCH_EVENT_NONBLOCK);
            if(xTaskGetTickCount()-last_increase >= increase_period)
            {
                variable++;
                last_increase = xTaskGetTickCount();
            }
            
            if(xSemaphoreTake(buttons.lock,0)==pdTRUE)
            {
                if(xTaskGetTickCount()-last_press>Debounce_delay)
                {
                    xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
                    if(buttons.buttons[KEYCODE(X)])
                    {
                        xSemaphoreGive(buttons.lock);
                        vTaskSuspend(Increase_Variable);
                    }
                    xSemaphoreGive(buttons.lock);
                }
            }
            vDrawFPS();
            sprintf(Variable,"The variable is: %d now.",variable);
                if (!gfxGetTextSize(Variable, &string_width, &string_height))
                {
                    gfxDrawText(Variable,xString ,SCREEN_HEIGHT/2,Black);

                }
                gfxDrawUpdateScreen();
            CheckStateInput();
        }

    }
}

void ResumeIncreaseVariable(void *pvParameters)
{
    static int string_width = 20;
    static int string_height = 20;
    int xString = SCREEN_WIDTH/2 - 100;
    char String[50] = "The increase of the variable is stopped.";
    TickType_t last_press = xTaskGetTickCount();
    TickType_t Debounce_delay = 5;
    gfxDrawBindThread();
    while(1)
    {
        if(xSemaphoreTake(DrawSignal,portMAX_DELAY) == pdTRUE)
        {   
             gfxDrawClear(White);
             gfxEventFetchEvents(FETCH_EVENT_NONBLOCK);
             if(xSemaphoreTake(buttons.lock,0)== pdTRUE)
            {
                if(xTaskGetTickCount()-last_press>Debounce_delay)
                {
                    xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
                    if(buttons.buttons[KEYCODE(X)])
                    {
                        xSemaphoreGive(buttons.lock);
                        vTaskResume(Increase_Variable);
                    }
                    xSemaphoreGive(buttons.lock);
                }
            }
            if (!gfxGetTextSize(String, &string_width, &string_height))
                {
                    gfxDrawText(String,xString ,SCREEN_HEIGHT/2,Black);

                }
            gfxDrawUpdateScreen();
            vDrawFPS();
            CheckStateInput();
        }
    }
}

void ExerciseTwo(void *pvParameters)
{
 
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
    //char mouse_Xcoord[20];
    //char mouse_Ycoord[20];
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
             
                //xGetButtonInput();
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
                
                        signed char left_button = gfxEventGetMouseLeft();
                        signed char middle_button = gfxEventGetMouseMiddle();
                        signed char right_button = gfxEventGetMouseRight();
                        if (left_button  || middle_button || right_button)
                        {
                            PressTimesA = 0;
                            PressTimesB = 0;
                            PressTimesC = 0;
                            PressTimesD = 0;
                          
                            last_change = xTaskGetTickCount();
                        }
                       
               
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
                        MoveDirection = 1;                       
                     
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
                
                gfxDrawCircle(Xcoord_Circle_Centre,Ycoord_Circle_Centre,radius,Gray);
                gfxDrawFilledBox(Xcoord_Square_LP,Ycoord_Square_LP,Square_Width,Square_Height,Blue);
                gfxDrawTriangle(Triangle_Points, Green);
                vDrawFPS();
                gfxDrawUpdateScreen(); // Refresh the screen to draw string

                CheckStateInput();
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
    //  functions to understand the functionality.if(Increase_Variable)
  
    if(gfxDrawInit(bin_folder_path)){
        PRINT_ERROR("Failed to initialize drawing");
        goto err_init_drawing;
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

    Binary_Sema = xSemaphoreCreateBinary();

    Sema_Wake_Three = xSemaphoreCreateBinary();


    StateQueue = xQueueCreate(STATE_QUEUE_LENGTH, sizeof(unsigned char));
    if (!StateQueue) {
        PRINT_ERROR("Could not open state queue");
        goto err_state_queue;
    }
    Press_Queue = xQueueCreate(PRESS_QUEUE_LENGTH,sizeof(int));

    if (xTaskCreate(ExerciseTwo, "Exercise2", mainGENERIC_STACK_SIZE * 2, NULL,
                    mainGENERIC_PRIORITY, &Exercise_two) != pdPASS) {
        goto err_init_drawing;
    }

 
    if (xTaskCreate(SequentialStateMachine, "StateMachine", mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES-1, &StateMachine) != pdPASS) {
        PRINT_TASK_ERROR("StateMachine");
        goto err_demotask;
    }

    if (xTaskCreate(vSwapBuffers, "BufferSwapTask",
                    mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES,
                    &BufferSwap) != pdPASS) {
        PRINT_TASK_ERROR("BufferSwapTask");
        goto err_bufferswap;
    }
    
    xTaskCreate(DisplayG, "Display_G",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+2,
                    &Display_G);

    xTaskCreate(SemaphoreTrigger, "Semaphore Trigger",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+2,
                    &Semaphore_Trigger);

    xTaskCreate(TaskNotifyTrigger, "TaskNotifyTrigger",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+2,
                    &TaskNotify_Trigger);
    xTaskCreate(DisplayH, "Display_H",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+3,
                    &Display_H);
    xTaskCreate(ResetNumber,"Reset",mainGENERIC_STACK_SIZE * 2,NULL,mainGENERIC_PRIORITY+4,&Reset_Number);

    xTaskCreate(IncreaseVariable, "IncreaseVariable",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+2,
                    &Increase_Variable);
    xTaskCreate(ResumeIncreaseVariable, "ResumeIncreaseVariable",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+1,
                    &Resume_Increase_Variable);

    xTaskCreate(TaskOne, "Task1",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+1,
                    &Task_one);
    xTaskCreate(TaskTwo, "Task2",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+2,
                    &Task_two);
    xTaskCreate(TaskThree, "Task3",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+3,
                    &Task_three);
    xTaskCreate(TaskFour, "Task4",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY+4,
                    &Task_four);
    xTaskCreate(OutputTask, "Output Task",
                    mainGENERIC_STACK_SIZE * 2, NULL, mainGENERIC_PRIORITY,
                    &Output_Task);
    
    if (xTaskCreate(ExerciseThree, "Exercise3", mainGENERIC_STACK_SIZE * 2, NULL,
                    mainGENERIC_PRIORITY+3,&Exercise_three) != pdPASS) {
        goto err_demotask;
    }
    Second_toggle_cir = xTaskCreateStatic(SecondToggleCircle, "SecondToggleCircle",STACK_SIZE, NULL,
                    mainGENERIC_PRIORITY+1,Stack_Buffer,&TaskControlBlock);
    
    



    vTaskSuspend(Exercise_two);
    vTaskSuspend(Exercise_three); 
    vTaskSuspend(Second_toggle_cir);
    vTaskSuspend(Display_G);
    vTaskSuspend(Display_H);
    vTaskSuspend(Semaphore_Trigger);
    vTaskSuspend(TaskNotify_Trigger);
    vTaskSuspend(Increase_Variable);
    vTaskSuspend(Resume_Increase_Variable);
    vTaskSuspend(Task_one);
    vTaskSuspend(Task_two);
    vTaskSuspend(Task_three);
    vTaskSuspend(Task_four);
    vTaskSuspend(Output_Task);
    
    

    vTaskStartScheduler();

    return EXIT_SUCCESS;


// err_statemachine:
err_bufferswap:
    vTaskDelete(Exercise_two);
err_demotask: err_state_queue:
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
        xSemaphoreGive(buttons.lock); 
    xTimeToSleep.tv_nsec = 0;state_changed
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}
