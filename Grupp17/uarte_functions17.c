#include "grupp17.h"
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
#include <stdio.h>


//definierar vilka pinnar som ska användas för att skicka och ta emot data:
#define PIN_TXD 20
#define PIN_RXD 22
#define LED1 (28)
#define LED2 (29)
#define LED3 (30)
#define LED4 (31)

uint8_t* uarte_buffer;
uint8_t* input_string;
uint8_t* output_string;


#define INPUT_BUFFER_SIZE 128

typedef struct node{
	int value;
    struct node *prev;
    struct node *next;
} node;

void read_string(uint8_t* input_string) {
	int i = 0;
	do {
		// Poll until a byte is available
		while (nrfx_uarte_rx(&instance, (uint8_t*)&uarte_buffer, 1) != NRFX_SUCCESS);
		input_string[i] = *uarte_buffer;
		i++;
	} while (input_string[i - 1] != '\r' && i < INPUT_BUFFER_SIZE - 1);
	input_string[i - 1] = '\0';
}

void write_string(const char* str)
{
	tx_done = false;

	nrfx_uarte_tx(
		&instance,
		(const uint8_t*)str,
		strlen(str),
		0
	);

	while (!tx_done)
	{
	}
}

void write_int(int input_integer){
	char data[13];
	sprintf(data, "%d", input_integer);
	int length=strlen(data);
	nrfx_uarte_tx(&instance, data, length, 0);
}

int read_int(uint8_t *input_string) {
    read_string(input_string);
    return atoi((const char *)input_string);
}

// ─── LED ───────────────────────────────────────────────────────────────────

void init_LED(void){
	nrf_gpio_cfg_output(LED1);
	nrf_gpio_cfg_output(LED2);
	nrf_gpio_cfg_output(LED3);
	nrf_gpio_cfg_output(LED4);

	/*Vi kontrollerar inte LEDs direkt utan vi kontrollerar GPIO pinnen som styr LEDen, så logiken blir lite omvänd p.g.a. saker ni kommer lära i elektroniksystem. 1 blir OFF. */

	nrf_gpio_pin_write(LED1,1);
	nrf_gpio_pin_write(LED2,1);
	nrf_gpio_pin_write(LED3,1);
	nrf_gpio_pin_write(LED4,1);
}

void LED_on(int adress){

	switch(adress){
		case 1:
		nrf_gpio_pin_write(LED1,0);
		break;
		
		case 2:
		nrf_gpio_pin_write(LED2,0);
		break;

		case 3:
		nrf_gpio_pin_write(LED3,0);
		break;

		case 4:
		nrf_gpio_pin_write(LED4,0);
		break;
	}
}

void LED_off(int adress){
	switch(adress){
		case 1:
		nrf_gpio_pin_write(LED1,1);
		break;

		case 2:
		nrf_gpio_pin_write(LED2,1);
		break;

		case 3:
		nrf_gpio_pin_write(LED3,1);
		break;

		case 4:
		nrf_gpio_pin_write(LED4,1);
		break;
	}
}

// ─── TIMING ───────────────────────────────────────────────────────────────────

void delay(int time){
	nrfx_systick_delay_ms(time);
	return;
}

// ─── UTILITY ───────────────────────────────────────────────────────────────────

void swap(int* a, int* b) {
	int save = *a;
	*a = *b;
	*b = save;
	//save a
	//overwrite a with b
	//load a
}

// ─── SORTING - ARRAYS ──────────────────────────────────────────────────────────

//Bubble Sort for array
void bubbleSort(int arr[], int n) {
	int i, j;
	int swapped;
	for (i = 0; i < n - 1; i++) {
		swapped = 0;
		for (j = 0; j < n - i - 1; j++) {
			if (arr[j] > arr[j + 1]) {
				swap(&arr[j], &arr[j + 1]);
				swapped = 1;
			}
		}

		// If no two elements were swapped by inner loop, the array is sorted, break
		if (swapped == 0)
			break;
	}
}

//Quicksort

// Partitions arr[low..high] around a pivot, returns the pivot's final index.
static int partition(int arr[], int low, int high) {
	int pivot = arr[high]; // Choose the last element as pivot
	int i = low - 1;       // i tracks the boundary of elements less than pivot

	for (int t = low; t <= high - 1; t++) { // Walk from low up to (but not including) pivot
		if (arr[t] < pivot) {
			i++;
			swap(&arr[i], &arr[t]); // Move element less than pivot to the left side
		}
	}
	swap(&arr[i + 1], &arr[high]); // Place pivot in its correct sorted position
	return i + 1;
}

// Recursively sorts arr[low..high] using quicksort.
void quickSort(int arr[], int low, int high) {
	if (low < high) {
		int pivot_index = partition(arr, low, high);
		quickSort(arr, low, pivot_index - 1);  // Sort left of pivot
		quickSort(arr, pivot_index + 1, high); // Sort right of pivot
	}
}

// Mergesort (divide & conquer)

// Merges two sorted subarrays: arr[low..mid] and arr[mid+1..high].
static void merge(int arr[], int low, int mid, int high) {
	int left_size = mid - low + 1;
	int right_size = high - mid;

	// Temporary arrays to hold copies of each half
	int left[left_size];
	int right[right_size];

	for (int i = 0; i < left_size; i++)
		left[i] = arr[low + i];
	for (int i = 0; i < right_size; i++)
		right[i] = arr[mid + 1 + i];

	// Merge the two halves back into arr
	int i = 0, t = 0, k = low;
	while (i < left_size && t < right_size) {
		if (left[i] <= right[t]) {
			arr[k++] = left[i++];
		}
		else {
			arr[k++] = right[t++];
		}
	}
	// Copy any remaining elements from whichever half isn't exhausted
	while (i < left_size)  arr[k++] = left[i++];
	while (t < right_size) arr[k++] = right[t++];
}

// Divides arr[low..high] in half, sorts each half, then merges.
void mergeSort(int arr[], int low, int high) {
	if (low < high) {
		int mid = low + (high - low) / 2; // Avoids overflow vs. (low+high)/2
		mergeSort(arr, low, mid);          // Divide and sort left half
		mergeSort(arr, mid + 1, high);     // Divide and sort right half
		merge(arr, low, mid, high);        // Conquer: merge the sorted halves
	}
}

//Insertion Sort
void insertionSort(int arr[], int n){
	for(int i=1; i<n; i++){
		int key = arr[i];	//key looks ahead
		int t=i-1;		//trail follows, ignoring everything that is lesser than itself
		while(t>=0 && arr[t]>key){	//when key moves, trail follows through the rest of the array backwards
			arr[t+1]=arr[t];	//trail swaps element with already sorted elements
			t--;		//trail decrements
		}
		arr[t+1]=key;	//reset trail and allow key to step forward
	}
	return;
}

// ─── SORTING - LISTS ──────────────────────────────────────────────────────────

void bubbleSortList(struct node** headptr) {
	if (headptr == NULL || *headptr == NULL) {
		write_string("List empty or null\n");
		return;
	}

	int swapped;
	struct node* current;
	struct node* last = NULL;

	do {
		swapped = 0;
		current = *headptr;

		while (current->next != last) {
			if (current->value > current->next->value) {
				struct node* next = current->next;

				swapDoubleList(current, next);

				// After the swap, next is now in current's old position
				// Update head if needed
				if (next->prev == NULL) {
					*headptr = next;
				}

				swapped = 1;
				current = next; // next is "behind" after the swap; advance through it
			}
			else {
				current = current->next;
			}
		}
		last = current;
	} while (swapped);
}

void swapDoubleList(struct node* node1, struct node* node2) {
	if (node1 == NULL || node2 == NULL) {
		write_string("Cannot swap: null node\n");
		return;
	}
	if (node1 == node2) {
		write_string("No need to swap: same node\n");
		return;
	}

	if (node1->next != node2 && node1->prev != node2) {
		// Non-adjacent nodes
		struct node* prev1 = node1->prev;
		struct node* prev2 = node2->prev;
		struct node* next1 = node1->next;
		struct node* next2 = node2->next;

		if (prev1 != NULL) prev1->next = node2;
		if (prev2 != NULL) prev2->next = node1;
		if (next1 != NULL) next1->prev = node2;
		if (next2 != NULL) next2->prev = node1;

		node1->next = next2;
		node2->next = next1;
		node1->prev = prev2;
		node2->prev = prev1;

	}
	else if (node1->next == node2) {
		// node1 is directly before node2
		struct node* prev1 = node1->prev;
		struct node* next2 = node2->next;

		if (prev1 != NULL) prev1->next = node2;
		if (next2 != NULL) next2->prev = node1;

		node2->next = node1;
		node1->next = next2;
		node2->prev = prev1;
		node1->prev = node2;

	}
	else {
		// node1 is directly after node2 (node2->next == node1)
		struct node* prev2 = node2->prev;
		struct node* next1 = node1->next;

		if (prev2 != NULL) prev2->next = node1;
		if (next1 != NULL) next1->prev = node2;

		node1->next = node2;
		node2->next = next1;
		node1->prev = prev2;
		node2->prev = node1;
	}
}

