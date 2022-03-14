/*
 * Porting layer for a snake game (snake_functions)
 *
 * port.c
 *
 *  Created on: Feb 27, 2022
 *      Author: Vladimir Sustek
 *      https://github.com/vladimirsustek
 */

#include "snake_port.h"


#define SNAKE_SERVER_PORT	(uint16_t)(8000u)

/* Platform dependent handles */
ADC_HandleTypeDef hadc1;

/* Randomizer seed */
static uint16_t gRandSeed;

/* variable for controlling snake's direction */
static char gKeyBoardButton;


/* wrapper around actual control implementation - start */
static void platform_control_init(void)
{
	  /* Start TCP server on the address 192.168.100.1:8000 */
	  tcp_server_init(SNAKE_SERVER_PORT);
}


/* wrapper around actual tft display init */
static void platform_display_init(void)
{
    tft_init(readID());
}


/**
  * @brief  Function to set gKeyBoardButton
  *
  * @note This function must be called in controlling callback or ISR
  *       function, because it directly stores into gKeyBoardButton
  *       which is further used by platform_get_control()
  *
  * @param  None
  * @retval None
  */
void platform_snake_set_control(char c)
{
	gKeyBoardButton = c;
}


/**
  * @brief  Initializes the randomizer's necessary blocks (platform dependent).
  *
  * @note   Implementation of this function must cover all necessary steps to
  *         run randomizing function. For embedded, the standard rand() function
  *         is not compilable, therefore LSFR mechanism must be used. In this
  *         case, the LFSR base seed is acquired as a noise ADC channel sample.
  *
  * @param  None
  * @retval None
  */
void platform_init_randomizer(void)
{
	/* This randomizer is based on ADC noise
	 * as a LFSR seed number */
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	gRandSeed = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	while(gRandSeed < 0x8000) gRandSeed += gRandSeed;
}


/**
  * @brief  Prepare hardware (especially screen) for a new game.
  *
  * @note   Implementation of this function cleans and prepares all necessary
  *         platform dependent steps for a new game - e.g. clean screen.
  *
  * @param  None
  * @retval None
  */
void platform_refresh_hw(void)
{
    fillScreen(BLACK);
}


/**
  * @brief  Draw a rectangle 'cell' into position x, y (white border, magenta filling).
  *
  * @note   Function implementation fits into display ARENA_MAX_X, ARENA_MAX_X.
  *         Adjustment may be for a platform needed. See Snake port.h MACROs.
  *
  * @param x - coordination limited by ARENA_MAX_X.
  * @param y - coordination limited by ARENA_MAX_Y.
  * @retval None
  */
void platform_drawCell(uint16_t x, uint16_t y)
{

	drawRect(ARENA_OFFSET_X + CELL_SIZE*x,
			ARENA_OFFSET_Y + CELL_SIZE*y,
			CELL_SIZE,
			CELL_SIZE,
			WHITE);

	fillRect(ARENA_OFFSET_X + CELL_SIZE*x + 1,
			ARENA_OFFSET_Y + CELL_SIZE*y + 1,
			CELL_SIZE - 2,
			CELL_SIZE - 2,
			MAGENTA);
}


/**
  * @brief  Erase a rectangle 'cell' of position x, y (black border and filling).
  *
  * @note   Function implementation fits into display ARENA_MAX_X, ARENA_MAX_X.
  *         Adjustment may be for a platform needed. See Snake port.h MACROs.
  *
  * @param x - coordination limited by ARENA_MAX_X.
  * @param y - coordination limited by ARENA_MAX_Y.
  * @retval None
  */
void platform_eraseCell(uint16_t x, uint16_t y)
{
	fillRect(ARENA_OFFSET_X + CELL_SIZE*x,
			ARENA_OFFSET_Y + CELL_SIZE*y,
			CELL_SIZE,
			CELL_SIZE,
			BLACK);
}


/**
  * @brief  Draw a circle 'food' into position x, y (green border and filling).
  *
  * @note   Function implementation fits into display ARENA_MAX_X, ARENA_MAX_X.
  *         Adjustment may be for a platform needed. See Snake port.h MACROs.
  *
  * @param x - coordination limited by FOOD_MAX_X.
  * @param y - coordination limited by FOOD_MAX_Y.
  * @retval None
  */
void platform_drawFood(uint16_t x, uint16_t y)
{
	fillCircle(ARENA_OFFSET_X + CELL_SIZE*x + CELL_SIZE/2,
			   ARENA_OFFSET_Y + CELL_SIZE*y + CELL_SIZE/2,
			   CELL_SIZE/3,
			   GREEN);
}


/**
  * @brief  Erase a circle 'food' of position x, y (black border and filling).
  *
  * @note   Function implementation fits into display ARENA_MAX_X, ARENA_MAX_X.
  *         Adjustment may be for a platform needed. See Snake port.h MACROs.
  *
  * @param x - coordination limited by FOOD_MAX_X.
  * @param y - coordination limited by FOOD_MAX_Y.
  * @retval None
  */
void platform_eraseFood(uint16_t x, uint16_t y)
{
	fillCircle(ARENA_OFFSET_X + CELL_SIZE*x + CELL_SIZE/2,
			   ARENA_OFFSET_Y + CELL_SIZE*y + CELL_SIZE/2,
			   CELL_SIZE/3,
			   BLACK);
}


/**
  * @brief  Initialize all needed peripherals for a snake game
  *
  * @note   For a embedded implementation with a display randomizer,
  *         control and display initialization is needed.
  *
  * @param None
  * @retval None
  */
void platform_init(void)
{
	platform_init_randomizer();
	platform_control_init();
	platform_display_init();
}


/**
  * @brief  Optional print for a information
  *
  * @note   For a embedded implementation with a display randomizer,
  *         control and display initialization is needed.
  *
  * @param str - string to be printed on the display
  * @param length - length of the string
  * @retval None
  */
void platform_showInformal(char* str, uint16_t length)
{
	/* Implementation to be done */
}


/**
  * @brief  Function to randomize a number - LFSR implemenation
  *
  * @note   This function is intended to be used in embedded MCU
  *         application, however for a PC implementation the rand()
  *         may work better (no seed handling needed).
  *
  *         e.g. min + rand() / (RAND_MAX / (max - min + 1) + 1);
  *
  * @param str - string to be printed on the display
  * @param length - length of the string
  * @retval randomized number
  */
uint16_t platform_randomize(void)
{
  uint16_t lsb;

  lsb = gRandSeed & 1;
  gRandSeed >>= 1;
  if (lsb == 1)
  {
	  gRandSeed ^= 0xB400u;
  }

  return gRandSeed;
}

/**
  * @brief  Function to get 1ms tick - used for blocking delay
  *
  * @note   For some platform it is native, for others is needed
  *         to created and start free running 1ms timer
  *  *
  * @param ms - tick in milliseconds
  * @retval None
  */
uint16_t platform_msTickGet(void)
{
	return (uint32_t)HAL_GetTick();
}


/**
  * @brief  Function to be used as a fatal error signaling
  *
  * @note   Not implemented yet
  *
  * @param None
  * @retval None
  */
void platform_fatal(void)
{
	platform_showInformal("FatalError\n", strlen("FatalError\n"));
	while (1);
}

/**
  * @brief  Function to set snake's direction
  *
  * @note   Function relies on the extKeyBoardButton and then casts
  *         this value into any direction/pause/quit of the snake.
  *         Function natively prevents change of 180° (LEFT-RIGHT)
  *
  * @param snake - pointer to a snake structure
  * @retval None
  */
void platform_get_control(snake_t * snake)
{
	snake_dir_e direction = 0;
	static snake_dir_e prev_direction = RIGHT;

	/* this value should be set by platform_snake_set_control */
	direction = (snake_dir_e)gKeyBoardButton;

	if (direction == 0)
	{
		return;
	}

	/* reset the value as it has been already parsed */
	platform_snake_set_control(0);

	/* If received character is not a known function, do pause and save previous state */
	if ((direction != LEFT) && (direction != RIGHT) && (direction != UP) &&
		(direction != DOWN) && (direction != PAUSE) && (direction != QUIT))
	{
		prev_direction = snake->direction;
		snake->direction = PAUSE;
	}
	else
	{
		/* If characters is a pause*/
		if (direction == PAUSE)
		{
			if (snake->direction != PAUSE)
			{
				/* Save snake's direction and set pause*/
				prev_direction = snake->direction;
				snake->direction = PAUSE;
			}
			else
			{
				/* Retrieve former direction and run the snake */
				snake->direction = prev_direction;
			}
		}
		/* Finally if characters is a valid direction - change direction (not allowed 180° changes)*/
		else
		{
			if ((snake->direction != PAUSE) &&
				!(snake->direction == LEFT && direction == RIGHT) &&
				!(snake->direction == RIGHT && direction == LEFT) &&
				!(snake->direction == UP && direction == DOWN) &&
				!(snake->direction == DOWN && direction == UP))
			{
				snake->direction = direction;
			}
		}
	}
}


/**
  * @brief  Function to draw snake border
  *
  * @note   Function draw 5 pixel thick border
  *
  * @param None
  * @retval None
  */
void platform_display_border(void)
{
	for(int idx = 0; idx < 6; idx++)
	{
		/* Draw a white rectangle 'frame' around display of size 320x480 */
		drawRect(idx, idx, 319 - 2*idx, 479 - 2*idx, WHITE);
	}
}


/**
  * @brief  Print text of size 18x7 into upper-center,
  *
  * @note   Prints text with
  *
  *
  * @param str - string to print
  * @param length -
  * @param color -
  *
  * @retval None
  */
void platform_print_text(char *str, uint16_t length, uint16_t color)
{
/*
  * mono18x7bold maximally 13 on line without fix
  * mono12x7bold maximally 19 on line without fix
*/
	fillRect(7, 135, 290, 20, BLACK);
	printnewtstr(150, color, &mono12x7bold, 1, str);

}
