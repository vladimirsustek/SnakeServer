/*
 * snake_function.c
 *
 *  Created on: Feb 27, 2022
 *      Author: Vladimir Sustek
 *      sustek.vladimir.ml@gmail.com
 *      No license
 *
 *
 * Snake game implementation for embedded - originally STM32F767ZI Nucleo-144 board
 * and TFT 3.5'' 320x480 MCU-Friend display.
 *
 * Game may be ported to any (MCU) platform and originally was developed as a console app.
 *
 * All the hardware dependencies are implemented in snake_port.c/snake_port.h. Note that
 * especially display functions, dimensions and ADC for randomizer must be ported if other
 * than original platform is used.
 *
 * How to use/play a game ? See following implementation.
 *
 *
 *  #ifdef UART_ISR_RX_CONTROL
 *  MY_UART_ISR_RX()
 *  {
 *      extKeyBoardButton = WHATEVER_UART_RX_REGISTER;
 *  }
 *  #else
 *  uint32_t myFuncCheckingReceivedCharSomewhere(uint32_t whatever_arg)
 *  {
 *      if(weReceivedAnything)
 *      {
 *          extKeyBoardButton = WHATEVER_VARIABLE_HAVING_THIS_VALUE;
 *      }
 *  }
 *  #endif
 *
 *  snake_hw_init();
 *
 *   while(1)
 *   {
 *     snake_t snake = { 0 };
 *	   food_t food = { 0 };
 *
 *     while(1)
 *     {
 *
 *       // control function must have updated extKeyBoardButton
 *	     snake_control(&snake);
 *	     snake_move(&snake);
 *
 *	     if (snake.state != PLAYING) break;
 *
 *	     snake_haseaten(&snake, &food);
 *
 *	     snake_display(&snake);
 *	     snake_place_food(&snake, &food);
 *
 *	     //snake_delay is necessary otherwise will be snake too fast
 *	     //the delay may be used to call other routines, or nothing = NULL
 *   #ifdef UART_ISR_RX_CONTROL
 *	     snake_delay(150, NULL);
 *	 #else
 *	     snake_delay(150, myFuncCheckingReceivedCharSomewhere);
 *	 #endif
 *      }
 *    }
 *
 *
 */

#include "snake_function.h"

/* Needed for printf*/
#include <stdio.h>

/* Needed for uints*/
#include <stdint.h>

/* Counter incremented each game cycle (to know, when to generate a snake's food) */
static uint32_t gPrgCycle = 0;


/**
  * @brief  Function to set snake's direction.
  *
  * @note   wrapper of th platform_get_control()
  *
  * @param snake - pointer to a snake structure
  * @retval None
  */
void snake_control(snake_t* snake)
{
	platform_get_control(snake);
}


/**
  * @brief  Complete initialization of needed dependencies.
  *
  * @note   wrapper of the platform_init()
  *
  * @param None
  * @retval None
  */
void snake_hw_init(void)
{
	platform_init();
}


/**
  * @brief  Initialization of new snake
  *
  * @note   Function fills the structure members to set the snake into initial
  *         condition with initial direction, length, coords ... and also calls
  *         the platform_refresh_hw() function is called to "clean screen".
  *
  * @param snake - pointer to a snake structure
  * @retval None
  */
void snake_init(snake_t* snake)
{
	snake->length = SNAKE_INIT_LNG;
	snake->direction = PAUSE;
	snake->state = PLAYING;
	snake->ghost.x = INVALID_COORDS;
	snake->ghost.y = INVALID_COORDS;
	snake->printWholeSnake = 1;
	memset(&snake->body[0], 0, SNAKE_MAX_LNG*sizeof(coord_t));

	gPrgCycle = 0;

	for (int idx = 0; idx < SNAKE_INIT_LNG; idx++)
	{
		snake->body[idx].x = SNAKE_INIT_X_CORD + idx;
		snake->body[idx].y = SNAKE_INIT_Y_CORD;
	}

	platform_refresh_hw();
	snake_diplay_borders();

}

/**
  * @brief  Display/Draw a snake
  *
  * @note   Function either draws a new snake, or draws a new head (when moves)
  *         and erases the tail's ghost (optimized to do not draw the whole snake)
  *
  * @param snake - pointer to a snake structure
  * @retval None
  */
void snake_display(snake_t*  snake)
{
	if (NULL == snake || PAUSE == snake->direction)
	{
		return;
	}
	/* In case of move, erase snake's ghost tail  */
	if (INVALID_COORDS != snake->ghost.x && INVALID_COORDS != snake->ghost.y)
	{
		platform_eraseCell(snake->ghost.x, snake->ghost.y);
	}

	/* In case of a brand new snake draw complete snake */
	if(snake->printWholeSnake)
	{
		snake ->printWholeSnake = 0;
		for (int idx = 0; idx < snake->length; idx++)
		{
			platform_drawCell(snake->body[idx].x, snake->body[idx].y);
		}
	}
	else
	{
		/* In case of move, draw snake's new head */
		platform_drawCell(snake->body[snake->length - 1].x, snake->body[snake->length - 1].y);
	}

}

/**
  * @brief  Draw a border within a snake must live
  *
  * @note   Once the snake hits the border game is over
  *
  * @param None
  * @retval None
  */
void snake_diplay_borders(void)
{
	platform_display_border();
}

/**
  * @brief  Function to implement movement of the snake with crash/won check.
  *
  * @note   Snake does not move when is in PAUSE. Otherwise is selected any
  *         movement case and snake's head coordination are adjusted accordingly.
  *
  * @param snake - pointer to a snake structure
  * @retval None
  */
void snake_move(snake_t* snake)
{
	if (NULL == snake || PAUSE == snake->direction)
	{
		return;
	}

	/* Prepare a 'ghost tail' to be erased on display */
	snake->ghost = snake->body[0];

	/* shift the whole array right, override tail (unneeded ghost tail) and make room for a new head*/
	memcpy(&snake->body[0], &snake->body[1], sizeof(coord_t) * (snake->length - 1));


	/* According to the direction check whether snake did not hit arena borders,
	 * then in the for loop check whether snake did not bit itself and finally
	 * do a head's coordinations adjustment => head is snake->body[snake->length - 1] */
	switch (snake->direction)
	{
	case UP:
	{
		if ((snake->body[snake->length - 1].y - 1) == ARENA_MIN_Y - 1) // because 0 is still valid
		{
			snake->state = CRASHED;
			break;
		}
		for (int idx = 0; idx < snake->length; idx++)
		{
			if (((snake->body[snake->length - 1].y - 1) == snake->body[idx].y) &&
				((snake->body[snake->length - 1].x) == snake->body[idx].x))
			{
				snake->state = CRASHED;
			}
		}
		snake->body[snake->length - 1].y--;
	}
	break;
	case DOWN:
	{
		if ((snake->body[snake->length - 1].y + 1) == ARENA_MAX_Y)
		{
			snake->state = CRASHED;
			break;
		}
		for (int idx = 0; idx < snake->length; idx++)
		{
			if (((snake->body[snake->length - 1].y + 1) == snake->body[idx].y) &&
				((snake->body[snake->length - 1].x) == snake->body[idx].x))
			{
				snake->state = CRASHED;
			}
		}

		snake->body[snake->length - 1].y++;
	}
	break;
	case RIGHT:
	{
		if ((snake->body[snake->length - 1].x + 1) == ARENA_MAX_X)
		{
			snake->state = CRASHED;
			break;
		}
		for (int idx = 0; idx < snake->length; idx++)
		{
			if (((snake->body[snake->length - 1].x + 1) == snake->body[idx].x) &&
				((snake->body[snake->length - 1].y) == snake->body[idx].y))
			{
				snake->state = CRASHED;
			}
		}
		snake->body[snake->length - 1].x++;
	}
	break;
	case LEFT:
	{
		if ((snake->body[snake->length - 1].x - 1) == ARENA_MIN_X - 1) // because 0 is still valid
		{
			snake->state = CRASHED;
			break;
		}
		for (int idx = 0; idx < snake->length; idx++)
		{
			if (((snake->body[snake->length - 1].x - 1) == snake->body[idx].x) &&
				((snake->body[snake->length - 1].y) == snake->body[idx].y))
			{
				snake->state = CRASHED;
			}
		}
		snake->body[snake->length - 1].x--;
	}
	break;
	default:
	{
	}
	}

	if (snake->length == SNAKE_WON_LIMIT)
	{
		snake->state = WON;
	}
}

/**
  * @brief  Function to generate a valid foord coordination and place it
  *
  * @note   Food is generated by a algorithm to prevent food's placement on the snake
  *         or behind the allowed borders. Finally, the food is drawn on the display.
  *
  * @param snake - pointer to a snake structure
  * @param snake - pointer to a food structure
  *
  * @retval None
  */
uint16_t static generate_food(snake_t* snake, food_t *food)
{
	uint16_t isInvalid = 0;
	uint16_t iter = 0;
	coord_t coords;

	if (NULL == snake || NULL == food || PAUSE == snake->direction)
	{
		return 0;
	}
	/* Try to generate x and y coords of the food and check if it is not snake's body */
	do
	{
		isInvalid = 0;
		coords.x = (uint16_t)((platform_randomize() % (FOOD_MAX_X - FOOD_MIN_X + 1)) + FOOD_MIN_X);
		coords.y = (uint16_t)((platform_randomize() % (FOOD_MAX_Y - FOOD_MIN_Y + 1)) + FOOD_MIN_Y);

		/* Check validity - can't be part of the snake's body */
		for (int idx = 0; idx < snake->length; idx++)
		{
			if(0 == memcmp(&(snake->body[idx]), &coords, sizeof(coord_t)))
			{
				isInvalid = GENERAL_ERROR;
				break;
			}
		}

		iter++;

		/* If iteration went too many times through re-initialize randomizer */
		if (iter > FOOD_MAX_ITER)
		{
			platform_init_randomizer();
		}

	} while (isInvalid);

	if (!isInvalid)
	{
		food->coord.x = coords.x;
		food->coord.y = coords.y;
		platform_drawFood(food->coord.x, food->coord.y);
	}

	return isInvalid;

}

/**
  * @brief  Function to place a new food in the arena
  *
  * @note   Function utilizes generate_food() when is "the time" to generate food.
  *         This means that gPrgCycle % 10 must be reached and also food must be
  *         already eaten. If there is a not eaten food and time still elapsed, just
  *         a flag time_elapsed is raised to indicate for next call, that food shall
  *         be immediately placed.
  *
  * @param snake - pointer to a snake structure
  * @retval None
  */
void snake_place_food(snake_t* snake, food_t* food)
{
	if (NULL == snake || NULL == food || PAUSE == snake->direction)
	{
		return;
	}

	if (0 == gPrgCycle % 10 || food->time_elapsed)
	{
		if (food->state != PLACED)
		{
			if (GENERAL_ERROR == generate_food(snake, food))
			{
				platform_fatal();
			}
			else
			{
				food->time_elapsed = 0;
				food->state = PLACED;
			}
		}
		else
		{
			food->time_elapsed = 1;
		}

	}
	gPrgCycle++;

}

/**
  * @brief  Function to implement snake grow if already eats a food
  *
  * @note   Function checks, whether snake's head really hits the food.
  *         If so, snake length is increased and the ghost tail becomes
  *         a part of the body.
  *
  *
  * @param snake - pointer to a snake structure
  * @retval None
  */
void snake_haseaten(snake_t* snake, food_t* food)
{

	if (NULL == snake || NULL == food || PAUSE == snake->direction)
	{
		return;
	}

	if ((snake->body[snake->length - 1].x == food->coord.x)
		&& (snake->body[snake->length - 1].y == food->coord.y))
	{
		/* Needed temporary copy for shifting the whole array into right - for an MCU embedded*/
		coord_t tempSnake[SNAKE_MAX_LNG] = {0};
		memcpy(tempSnake, &(snake->body[0]), (size_t)snake->length*sizeof(coord_t));

		/* Just append the ghost to the end, increment length and disable ghost erase this cycle*/
		memcpy(&(snake->body[1]), tempSnake, (size_t)snake->length*sizeof(coord_t));
		snake->body[0] = snake->ghost;
		snake->ghost.x = INVALID_COORDS;
		snake->ghost.y = INVALID_COORDS;
		snake->length++;

		food->state = EATEN;
	}
}


/**
  * @brief  Function to print a snake state/information
  *
  * @param snake - pointer to a snake structure
  * @retval None
  */
void snake_inform(snake_t* snake)
{
	static uint16_t pauseStringAppeared = 0;
	char printStr[20] = {0};


	if(snake->direction == PAUSE && !pauseStringAppeared)
	{
		sprintf(printStr, " Paused Score:%05d", snake->length - SNAKE_INIT_LNG);
		platform_print_text(printStr, strlen(printStr), WHITE);
		pauseStringAppeared = 1;
	}
	if(snake->direction != PAUSE && pauseStringAppeared)
	{
		sprintf(printStr, " Paused Score:%05d", snake->length - SNAKE_INIT_LNG);
		pauseStringAppeared = 0;
		platform_print_text(printStr, strlen(printStr), BLACK);
		snake->printWholeSnake = 1;
	}
	if(snake->state == CRASHED)
	{
		sprintf(printStr, " Crash! Score:%05d", snake->length - SNAKE_INIT_LNG);
		platform_print_text(printStr, strlen(printStr), WHITE);
	}
	if(snake->state == WON)
	{
		sprintf(printStr, " Win  ! Score:%05d", snake->length - SNAKE_INIT_LNG);
		platform_print_text(printStr, strlen(printStr), WHITE);
	}
}


/**
  * @brief  Function to do pseudo-blocking delay
  *
  * @note  Function does blocking delay based on platform_msTickGet. However,
  *        during the "waiting loop" MCU periodically process passed function.
  *        Be careful, what kind of function is passed. It is intended for very
  *        quick actions e.g. checking received buffers. Passed function can not
  *        be long in execution time and also must not content any blocking or
  *        "waiting for event" statement. Basically, maximal execution time of
  *        the passed function must be 2x smaller then intended Delay time.
  *        In case, there is no suitable function to be called within this delay,
  *        simply pass NULL to the pointer and function will behave as blocking delay.
  *
  * @param Delay - time in millisecond to be spent in function
  * @param func - function to be called during time
  * @retval None
  */
void snake_delay(uint32_t Delay, fn_t func)
{
  uint32_t tickstart = platform_msTickGet();
  uint32_t wait = Delay;
  uint32_t dummyArgRet = 0;

  /* Add a freq to guarantee minimum wait */
  if (wait < HAL_MAX_DELAY)
  {
    wait += (uint32_t)(uwTickFreq);
  }

  while ((platform_msTickGet() - tickstart) < wait)
  {
	  if (NULL != func)
	  {
		  /* Arguments are in original case unused,
		   * however may be useful ... once ?*/
		  dummyArgRet = func(dummyArgRet);
	  }

  }
}
