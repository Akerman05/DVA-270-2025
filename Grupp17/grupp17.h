/*#ifndef custom_uarte
#define custom_uarte

void init_uarte(void);

void read_string(uint8_t *input_string);

void write_string(const uint8_t *output_string);

void write_int(int input_integer);

int read_int(uint8_t *input_string);

void LED_init(void);

void LED_on(int adress);

void LED_off(int adress);

void delay(int time);

void bubbleSort(int arr[]);

void bubbleSortList(struct list);

static int partition(int arr[], int low, int high); // internal, can be omitted from header
void quickSort(int arr[], int low, int high);

static void merge(int arr[], int low, int mid, int high); // internal, can be omitted
void mergeSort(int arr[], int low, int high);

void insertionSort(int arr[]);

void swapDoubleList(struct node *node1, struct node *node2);

#endif*/
#ifndef CUSTOM_UARTE_H
#define CUSTOM_UARTE_H

#include <nrfx.h>
#include <nrf5340_application.h>
#include <nrfx_config.h>
#include <nrf.h>
#include <nrfx_uarte.h>
#include <nrf_gpio.h>
#include <nrfx_systick.h>
#include <nrfx_rtc.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Forward declaration — full definition is in uarte_functions.c.
// Any file that only uses pointers to struct node does not need the full definition.
struct node;
extern nrfx_uarte_t instance;
extern volatile bool tx_done;				//volatile tx_done flag and rx_ready signal flag to set up interrupts
extern volatile bool rx_ready;
extern volatile uint8_t rx_buffer;
extern volatile uint32_t uart_error_count;

#define INPUT_BUFFER_SIZE 128

// UARTE
void init_uarte(void);
void uarte_handler(nrfx_uarte_event_t const* p_event, void* p_context);
void read_string(uint8_t* input_string);
void write_string(const char* output_string);
void write_int(int input_integer);
int  read_int(uint8_t* input_string);

// LED
void init_LED(void);
void LED_on(int address);
void LED_off(int address);

// Timing
void delay(int time);

// Utility
void swap(int* a, int* b);

// Sorting — arrays
void bubbleSort(int arr[], int n);
void quickSort(int arr[], int low, int high);
void mergeSort(int arr[], int low, int high);
void insertionSort(int arr[], int n);

// Sorting — doubly linked list
void bubbleSortList(struct node** headptr);

// Linked list utilities
void swapDoubleList(struct node* node1, struct node* node2);

#endif

