/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mpu6050.h"
#include "oled.h"
#include "usart.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
static volatile uint32_t g_uptime_seconds = 0;
osThreadId clockTaskHandle;
osThreadId bluetoothTaskHandle;
osThreadId sensorTaskHandle;
osThreadId uiTaskHandle;

/* USER CODE END Variables */
osThreadId defaultTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartClockTask(void const * argument);
void StartBluetoothTask(void const * argument);
void StartSensorTask(void const * argument);
void StartUiTask(void const * argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  osThreadDef(clockTask, StartClockTask, osPriorityNormal, 0, 128);
  clockTaskHandle = osThreadCreate(osThread(clockTask), NULL);

  osThreadDef(bluetoothTask, StartBluetoothTask, osPriorityBelowNormal, 0, 128);
  bluetoothTaskHandle = osThreadCreate(osThread(bluetoothTask), NULL);

  osThreadDef(sensorTask, StartSensorTask, osPriorityNormal, 0, 128);
  sensorTaskHandle = osThreadCreate(osThread(sensorTask), NULL);

  osThreadDef(uiTask, StartUiTask, osPriorityAboveNormal, 0, 256);
  uiTaskHandle = osThreadCreate(osThread(uiTask), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
#define UI_PAGE_COUNT 5U
#define UI_MENU_ITEM_COUNT 4U
#define UI_MENU_VISIBLE_COUNT 4U
#define UI_MENU_CURSOR_X 0U
#define UI_MENU_TEXT_X 18U
#define UI_MENU_FIRST_PAGE 1U
#define UI_MENU_PAGE_STEP 2U
#define ENCODER_STEPS_PER_UI_MOVE 4
#define CLOCK_DAY_SECONDS 86400U
#define STEP_HIGH_THRESHOLD_SQ 420000000UL
#define STEP_LOW_THRESHOLD_SQ 330000000UL
#define STEP_COOLDOWN_SAMPLES 1U

typedef enum {
  CLOCK_FIELD_HOUR = 0,
  CLOCK_FIELD_MINUTE,
  CLOCK_FIELD_SECOND
} ClockField;

static volatile uint32_t g_stopwatch_seconds = 0;
static volatile uint8_t g_stopwatch_running = 0;
static volatile uint32_t g_clock_seconds = 12U * 3600U;
static volatile uint8_t g_mpu_ready = 0;
static volatile uint8_t g_mpu_whoami = 0;
static volatile MPU6050_Data g_mpu_data;
static volatile uint32_t g_step_count = 0;
static volatile uint32_t g_step_motion = 0;
static volatile uint8_t g_step_active = 0;

static uint8_t Key_IsPressed(GPIO_TypeDef *port, uint16_t pin)
{
  return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET;
}

static uint8_t Ui_MenuMove(uint8_t selection, int8_t encoder_step)
{
  if (encoder_step > 0) {
    return (uint8_t)((selection + 1U) % UI_MENU_ITEM_COUNT);
  }
  if (encoder_step < 0) {
    if (selection == 0U) {
      return (uint8_t)(UI_MENU_ITEM_COUNT - 1U);
    }
    return (uint8_t)(selection - 1U);
  }

  return selection;
}

static uint8_t Ui_MenuSelectionToPage(uint8_t selection)
{
  return (uint8_t)(selection + 1U);
}

static uint8_t Encoder_ReadState(void)
{
  uint8_t a = Key_IsPressed(ENC_A_GPIO_Port, ENC_A_Pin);
  uint8_t b = Key_IsPressed(ENC_B_GPIO_Port, ENC_B_Pin);

  return (uint8_t)((a << 1U) | b);
}

static int8_t Encoder_GetStep(void)
{
  static const int8_t transition_table[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
  };
  static uint8_t last_state = 0xFFU;
  static int8_t accumulator = 0;
  uint8_t state = Encoder_ReadState();
  int8_t movement;

  if (last_state == 0xFFU) {
    last_state = state;
    return 0;
  }

  movement = transition_table[(last_state << 2U) | state];
  last_state = state;

  if (movement == 0) {
    return 0;
  }

  accumulator = (int8_t)(accumulator + movement);
  if (accumulator >= ENCODER_STEPS_PER_UI_MOVE) {
    accumulator = 0;
    return 1;
  }
  if (accumulator <= -ENCODER_STEPS_PER_UI_MOVE) {
    accumulator = 0;
    return -1;
  }

  return 0;
}

static void Ui_ShowTimeLarge(uint8_t x, uint8_t page, uint32_t seconds)
{
  char text[9];
  uint8_t hour = (uint8_t)((seconds / 3600U) % 24U);
  uint8_t minute = (uint8_t)((seconds / 60U) % 60U);
  uint8_t second = (uint8_t)(seconds % 60U);

  text[0] = (char)('0' + (hour / 10U));
  text[1] = (char)('0' + (hour % 10U));
  text[2] = ':';
  text[3] = (char)('0' + (minute / 10U));
  text[4] = (char)('0' + (minute % 10U));
  text[5] = ':';
  text[6] = (char)('0' + (second / 10U));
  text[7] = (char)('0' + (second % 10U));
  text[8] = '\0';

  OLED_ShowString2x(x, page, text);
}

static void Ui_ClearLine(uint8_t page)
{
  OLED_ShowString(0, page, "                     ");
}

static void Ui_ShowStopwatchStatus(void)
{
  Ui_ClearLine(6);
  OLED_ShowString(0, 6, g_stopwatch_running ? "Running" : "Paused");
}

static void Ui_ShowI16Fixed(uint8_t x, uint8_t page, int16_t value)
{
  char text[7];
  uint8_t index = 6U;
  uint16_t magnitude;

  text[6] = '\0';
  for (uint8_t i = 0; i < 6U; i++) {
    text[i] = ' ';
  }

  if (value < 0) {
    magnitude = (uint16_t)(-value);
  } else {
    magnitude = (uint16_t)value;
  }

  do {
    text[--index] = (char)('0' + (magnitude % 10U));
    magnitude /= 10U;
  } while (magnitude > 0U && index > 1U);

  if (value < 0 && index > 0U) {
    text[--index] = '-';
  }

  OLED_ShowString(x, page, text);
}

static void Ui_ShowU32Field(uint8_t x, uint8_t page, uint32_t value)
{
  char text[11];
  uint8_t index = 10U;

  text[10] = '\0';
  for (uint8_t i = 0; i < 10U; i++) {
    text[i] = ' ';
  }

  do {
    text[--index] = (char)('0' + (value % 10U));
    value /= 10U;
  } while (value > 0U && index > 0U);

  OLED_ShowString(x, page, text);
}

static void Ui_ShowClockTitle(uint8_t edit_mode)
{
  Ui_ClearLine(0);
  OLED_ShowString(0, 0, edit_mode ? "Clock Set" : "Clock");
}

static void Clock_AddSelectedField(ClockField field)
{
  uint32_t hour = (g_clock_seconds / 3600U) % 24U;
  uint32_t minute = (g_clock_seconds / 60U) % 60U;
  uint32_t second = g_clock_seconds % 60U;

  if (field == CLOCK_FIELD_HOUR) {
    hour = (hour + 1U) % 24U;
  } else if (field == CLOCK_FIELD_MINUTE) {
    minute = (minute + 1U) % 60U;
  } else {
    second = (second + 1U) % 60U;
  }

  g_clock_seconds = (hour * 3600U) + (minute * 60U) + second;
}

static ClockField Clock_NextField(ClockField field)
{
  return (ClockField)((field + 1U) % 3U);
}

static void Ui_ShowClockStatus(uint8_t edit_mode, ClockField field)
{
  Ui_ClearLine(6);

  if (!edit_mode) {
    OLED_ShowString(0, 6, "Normal");
  } else if (field == CLOCK_FIELD_HOUR) {
    OLED_ShowString(0, 6, "Set Hour");
  } else if (field == CLOCK_FIELD_MINUTE) {
    OLED_ShowString(0, 6, "Set Min");
  } else {
    OLED_ShowString(0, 6, "Set Sec");
  }
}

static void Ui_ShowSensorValues(void)
{
  MPU6050_Data data;

  data = g_mpu_data;

  OLED_ShowString(0, 2, "AX:");
  Ui_ShowI16Fixed(18, 2, data.ax);

  OLED_ShowString(0, 4, "AY:");
  Ui_ShowI16Fixed(18, 4, data.ay);

  OLED_ShowString(0, 6, "AZ:");
  Ui_ShowI16Fixed(18, 6, data.az);
}

static uint32_t Step_SquareI16(int16_t value)
{
  int32_t wide = value;

  return (uint32_t)(wide * wide);
}

static void Step_Update(const MPU6050_Data *data)
{
  static uint8_t cooldown = 0;
  uint32_t magnitude_sq;

  magnitude_sq = Step_SquareI16(data->ax) + Step_SquareI16(data->ay) + Step_SquareI16(data->az);
  g_step_motion = magnitude_sq / 1000000UL;

  if (cooldown > 0U) {
    cooldown--;
  }

  if (g_step_active) {
    if (magnitude_sq < STEP_LOW_THRESHOLD_SQ) {
      g_step_active = 0;
    }
  } else if (cooldown == 0U && magnitude_sq > STEP_HIGH_THRESHOLD_SQ) {
    g_step_count++;
    g_step_active = 1;
    cooldown = STEP_COOLDOWN_SAMPLES;
  }
}

static void Ui_ShowStepCount(void)
{
  OLED_ShowString(0, 2, "Steps:");
  Ui_ShowU32Field(42, 2, g_step_count);
}

static void Ui_ShowStepMotion(void)
{
  OLED_ShowString(0, 4, "Move:");
  Ui_ShowU32Field(36, 4, g_step_motion);
}

static void Ui_ShowStepStatus(void)
{
  Ui_ClearLine(6);
  OLED_ShowString(0, 6, g_step_active ? "Step Active" : "Ready");
}

static void Ui_ShowStepValues(void)
{
  Ui_ShowStepCount();
  Ui_ShowStepMotion();
  Ui_ShowStepStatus();
}

static const char *Ui_GetMenuItem(uint8_t index)
{
  static const char *items[UI_MENU_ITEM_COUNT] = {
    "Stopwatch",
    "Clock",
    "MPU6050",
    "Steps"
  };

  return items[index];
}

static uint8_t Ui_GetMenuTop(uint8_t selection)
{
  uint8_t top = 0U;

  if (selection >= (UI_MENU_VISIBLE_COUNT - 1U)) {
    top = (uint8_t)(selection - (UI_MENU_VISIBLE_COUNT - 1U));
  }
  if (top > (UI_MENU_ITEM_COUNT - UI_MENU_VISIBLE_COUNT)) {
    top = (uint8_t)(UI_MENU_ITEM_COUNT - UI_MENU_VISIBLE_COUNT);
  }

  return top;
}

static uint8_t Ui_GetMenuRow(uint8_t top, uint8_t selection)
{
  return (uint8_t)(selection - top);
}

static uint8_t Ui_GetMenuPage(uint8_t row)
{
  return (uint8_t)(UI_MENU_FIRST_PAGE + (row * UI_MENU_PAGE_STEP));
}

static void Ui_DrawMenuCursor(uint8_t row, uint8_t selected)
{
  uint8_t page = Ui_GetMenuPage(row);

  OLED_ShowString(UI_MENU_CURSOR_X, page, selected ? "->" : "  ");
}

static void Ui_UpdateMenuCursor(uint8_t old_selection, uint8_t new_selection)
{
  uint8_t old_top = Ui_GetMenuTop(old_selection);
  uint8_t new_top = Ui_GetMenuTop(new_selection);

  if (old_top != new_top) {
    return;
  }

  Ui_DrawMenuCursor(Ui_GetMenuRow(old_top, old_selection), 0U);
  Ui_DrawMenuCursor(Ui_GetMenuRow(new_top, new_selection), 1U);
}

static void Ui_DrawMenu(uint8_t selection)
{
  uint8_t top = Ui_GetMenuTop(selection);

  OLED_Clear();
  OLED_ShowString(0, 0, "Smart Watch Menu");

  for (uint8_t row = 0U; row < UI_MENU_VISIBLE_COUNT; row++) {
    uint8_t item = (uint8_t)(top + row);
    uint8_t page = Ui_GetMenuPage(row);
    Ui_DrawMenuCursor(row, item == selection);
    OLED_ShowString(UI_MENU_TEXT_X, page, Ui_GetMenuItem(item));
  }
}

static void Bluetooth_SendText(const char *text)
{
  uint16_t length = 0U;

  while (text[length] != '\0') {
    length++;
  }

  if (length > 0U) {
    HAL_UART_Transmit(&huart1, (uint8_t *)text, length, 100);
  }
}

static void Bluetooth_SendU32(uint32_t value)
{
  char text[11];
  uint8_t index = 10U;

  text[index] = '\0';
  do {
    index--;
    text[index] = (char)('0' + (value % 10U));
    value /= 10U;
  } while (value > 0U && index > 0U);

  Bluetooth_SendText(&text[index]);
}

static void Bluetooth_SendI16(int16_t value)
{
  uint16_t magnitude;

  if (value < 0) {
    Bluetooth_SendText("-");
    magnitude = (uint16_t)(-value);
  } else {
    magnitude = (uint16_t)value;
  }

  Bluetooth_SendU32(magnitude);
}

static void Bluetooth_Send2Digits(uint8_t value)
{
  char text[3];

  text[0] = (char)('0' + (value / 10U));
  text[1] = (char)('0' + (value % 10U));
  text[2] = '\0';
  Bluetooth_SendText(text);
}

static void Bluetooth_SendStepLine(uint32_t step_count)
{
  char text[24];
  char digits[11];
  uint8_t text_index = 0U;
  uint8_t digit_index = 10U;

  text[text_index++] = 'S';
  text[text_index++] = 't';
  text[text_index++] = 'e';
  text[text_index++] = 'p';
  text[text_index++] = 's';
  text[text_index++] = ':';
  text[text_index++] = ' ';

  digits[digit_index] = '\0';
  do {
    digit_index--;
    digits[digit_index] = (char)('0' + (step_count % 10U));
    step_count /= 10U;
  } while (step_count > 0U && digit_index > 0U);

  while (digits[digit_index] != '\0') {
    text[text_index++] = digits[digit_index++];
  }

  text[text_index++] = '\r';
  text[text_index++] = '\n';
  text[text_index] = '\0';

  Bluetooth_SendText(text);
}

static void Bluetooth_SendStatus(void)
{
  MPU6050_Data data = g_mpu_data;
  uint32_t seconds = g_clock_seconds;

  Bluetooth_SendStepLine(g_step_count);
  Bluetooth_SendText("Time: ");
  Bluetooth_Send2Digits((uint8_t)((seconds / 3600U) % 24U));
  Bluetooth_SendText(":");
  Bluetooth_Send2Digits((uint8_t)((seconds / 60U) % 60U));
  Bluetooth_SendText(":");
  Bluetooth_Send2Digits((uint8_t)(seconds % 60U));
  Bluetooth_SendText("\r\nAX: ");
  Bluetooth_SendI16(data.ax);
  Bluetooth_SendText("  AY: ");
  Bluetooth_SendI16(data.ay);
  Bluetooth_SendText("  AZ: ");
  Bluetooth_SendI16(data.az);
  Bluetooth_SendText("\r\n\r\n");
}

static void Ui_DrawPage(uint8_t page, uint32_t uptime_seconds, uint8_t edit_mode, ClockField field)
{
  if (page == 0U) {
    return;
  }

  OLED_Clear();

  switch (page) {
    case 0:
      OLED_ShowString(0, 0, "Smart Watch");
      OLED_ShowString(0, 2, "FreeRTOS OK");
      OLED_ShowString(0, 4, "Uptime:");
      OLED_ShowU32(48, 4, uptime_seconds);
      OLED_ShowString(84, 4, "s");
      break;

    case 1:
      OLED_ShowString(0, 0, "Stop Watch");
      Ui_ShowTimeLarge(16, 2, g_stopwatch_seconds);
      Ui_ShowStopwatchStatus();
      break;

    case 2:
      Ui_ShowClockTitle(edit_mode);
      Ui_ShowTimeLarge(16, 2, g_clock_seconds);
      Ui_ShowClockStatus(edit_mode, field);
      break;

    case 3:
      OLED_ShowString(0, 0, g_mpu_ready ? "MPU6050 OK" : "MPU6050 ERR");
      if (g_mpu_ready) {
        Ui_ShowSensorValues();
      } else {
        OLED_ShowString(0, 2, "WHO:");
        OLED_ShowU32(30, 2, g_mpu_whoami);
        OLED_ShowString(0, 4, "Check I2C2");
      }
      break;

    default:
      OLED_ShowString(0, 0, "Step Data");
      Ui_ShowStepValues();
      break;
  }
}

void StartClockTask(void const * argument)
{
  (void)argument;

  for (;;) {
    osDelay(1000);
    g_uptime_seconds++;
    if (g_stopwatch_running) {
      g_stopwatch_seconds++;
    }
    g_clock_seconds = (g_clock_seconds + 1U) % CLOCK_DAY_SECONDS;
  }
}

void StartBluetoothTask(void const * argument)
{
  (void)argument;
  uint8_t rx;
  uint32_t last_sent_step_count = g_step_count;

  for (;;) {
    while (HAL_UART_Receive(&huart1, &rx, 1, 0) == HAL_OK) {
      if (rx == 'C' || rx == 'c') {
        g_step_count = 0;
        last_sent_step_count = 0;
        Bluetooth_SendText("OK,CLEAR\r\n");
      } else if (rx == 'S' || rx == 's') {
        Bluetooth_SendStatus();
        last_sent_step_count = g_step_count;
      }
    }

    if (g_step_count > last_sent_step_count) {
      last_sent_step_count = g_step_count;
      Bluetooth_SendStepLine(last_sent_step_count);
    } else if (g_step_count < last_sent_step_count) {
      last_sent_step_count = g_step_count;
    }

    osDelay(100);
  }
}

void StartSensorTask(void const * argument)
{
  (void)argument;
  MPU6050_Data data;
  uint8_t id = 0;

  osDelay(300);
  g_mpu_ready = MPU6050_Init();
  if (MPU6050_ReadWhoAmI(&id)) {
    g_mpu_whoami = id;
  }

  for (;;) {
    if (g_mpu_ready && MPU6050_ReadData(&data)) {
      g_mpu_data = data;
      Step_Update(&data);
    } else {
      g_mpu_ready = 0;
      if (MPU6050_ReadWhoAmI(&id)) {
        g_mpu_whoami = id;
        if (id == 0x68U) {
          g_mpu_ready = MPU6050_Init();
        }
      }
    }

    osDelay(200);
  }
}

void StartUiTask(void const * argument)
{
  (void)argument;
  uint8_t current_page = 0;
  uint8_t menu_selection = 0;
  uint8_t next_was_pressed = 0;
  uint8_t up_was_pressed = 0;
  uint8_t ok_was_pressed = 0;
  uint8_t clock_edit_mode = 0;
  ClockField clock_field = CLOCK_FIELD_HOUR;
  uint32_t last_stopwatch_seconds = 0xFFFFFFFFU;
  uint32_t last_clock_seconds = 0xFFFFFFFFU;
  uint8_t last_mpu_ready = 0xFFU;
  int16_t last_ax = 0;
  int16_t last_ay = 0;
  int16_t last_az = 0;
  uint32_t last_step_count = 0xFFFFFFFFU;
  uint32_t last_step_motion = 0xFFFFFFFFU;
  uint8_t last_step_active = 0xFFU;

  Ui_DrawMenu(menu_selection);

  for (;;) {
    uint8_t next_is_pressed = Key_IsPressed(KEY_NEXT_GPIO_Port, KEY_NEXT_Pin);
    uint8_t up_is_pressed = Key_IsPressed(KEY_UP_GPIO_Port, KEY_UP_Pin);
    uint8_t ok_is_pressed = Key_IsPressed(ENC_KEY_GPIO_Port, ENC_KEY_Pin);
    int8_t encoder_step = Encoder_GetStep();

    if (next_is_pressed && !next_was_pressed) {
      if (current_page == 0U) {
        /* Keep PA1 as a no-op on the menu page. */
      } else if (current_page == 2U && clock_edit_mode) {
        clock_field = Clock_NextField(clock_field);
        Ui_ShowClockStatus(clock_edit_mode, clock_field);
      } else {
        current_page = 0U;
        clock_edit_mode = 0U;
        Ui_DrawMenu(menu_selection);
        last_stopwatch_seconds = 0xFFFFFFFFU;
        last_clock_seconds = 0xFFFFFFFFU;
        last_mpu_ready = 0xFFU;
        last_step_count = 0xFFFFFFFFU;
        last_step_motion = 0xFFFFFFFFU;
        last_step_active = 0xFFU;
      }
    }

    if (encoder_step != 0) {
      if (current_page == 0U) {
        uint8_t old_menu_selection = menu_selection;
        menu_selection = Ui_MenuMove(menu_selection, encoder_step);
        if (Ui_GetMenuTop(old_menu_selection) == Ui_GetMenuTop(menu_selection)) {
          Ui_UpdateMenuCursor(old_menu_selection, menu_selection);
        } else {
          Ui_DrawMenu(menu_selection);
        }
      } else if (current_page == 2U && clock_edit_mode) {
        if (encoder_step > 0) {
          Clock_AddSelectedField(clock_field);
          Ui_ShowTimeLarge(16, 2, g_clock_seconds);
          last_clock_seconds = g_clock_seconds;
        } else {
          clock_field = Clock_NextField(clock_field);
          Ui_ShowClockStatus(clock_edit_mode, clock_field);
        }
      }
    }

    if (up_is_pressed && !up_was_pressed) {
      if (current_page == 1U) {
        g_stopwatch_seconds = 0;
        Ui_ShowTimeLarge(16, 2, g_stopwatch_seconds);
        last_stopwatch_seconds = g_stopwatch_seconds;
      } else if (current_page == 4U) {
        g_step_count = 0;
        Ui_ShowStepValues();
        last_step_count = g_step_count;
        last_step_motion = g_step_motion;
        last_step_active = g_step_active;
      } else if (current_page == 2U && clock_edit_mode) {
        Clock_AddSelectedField(clock_field);
        Ui_ShowTimeLarge(16, 2, g_clock_seconds);
        last_clock_seconds = g_clock_seconds;
      }
    }

    if (ok_is_pressed && !ok_was_pressed) {
      if (current_page == 0U) {
        current_page = Ui_MenuSelectionToPage(menu_selection);
        Ui_DrawPage(current_page, g_uptime_seconds, clock_edit_mode, clock_field);
        last_stopwatch_seconds = 0xFFFFFFFFU;
        last_clock_seconds = 0xFFFFFFFFU;
        last_mpu_ready = 0xFFU;
        last_step_count = 0xFFFFFFFFU;
        last_step_motion = 0xFFFFFFFFU;
        last_step_active = 0xFFU;
      } else if (current_page == 1U) {
        g_stopwatch_running = !g_stopwatch_running;
        Ui_ShowStopwatchStatus();
        last_stopwatch_seconds = g_stopwatch_seconds;
      } else if (current_page == 2U) {
        clock_edit_mode = !clock_edit_mode;
        Ui_ShowClockTitle(clock_edit_mode);
        Ui_ShowClockStatus(clock_edit_mode, clock_field);
        last_clock_seconds = g_clock_seconds;
      }
    }

    next_was_pressed = next_is_pressed;
    up_was_pressed = up_is_pressed;
    ok_was_pressed = ok_is_pressed;

    if (current_page == 1U && last_stopwatch_seconds != g_stopwatch_seconds) {
      last_stopwatch_seconds = g_stopwatch_seconds;
      Ui_ShowTimeLarge(16, 2, last_stopwatch_seconds);
    } else if (current_page == 2U && last_clock_seconds != g_clock_seconds) {
      last_clock_seconds = g_clock_seconds;
      Ui_ShowTimeLarge(16, 2, last_clock_seconds);
    } else if (current_page == 3U &&
               (last_mpu_ready != g_mpu_ready ||
                last_ax != g_mpu_data.ax ||
                last_ay != g_mpu_data.ay ||
                last_az != g_mpu_data.az)) {
      if (last_mpu_ready != g_mpu_ready) {
        Ui_DrawPage(current_page, g_uptime_seconds, clock_edit_mode, clock_field);
      } else if (g_mpu_ready) {
        Ui_ShowSensorValues();
      }

      last_mpu_ready = g_mpu_ready;
      last_ax = g_mpu_data.ax;
      last_ay = g_mpu_data.ay;
      last_az = g_mpu_data.az;
    } else if (current_page == 4U &&
               (last_step_count != g_step_count ||
                last_step_motion != g_step_motion ||
                last_step_active != g_step_active)) {
      if (last_step_count != g_step_count) {
        Ui_ShowStepCount();
      }
      if (last_step_motion != g_step_motion) {
        Ui_ShowStepMotion();
      }
      if (last_step_active != g_step_active) {
        Ui_ShowStepStatus();
      }
      last_step_count = g_step_count;
      last_step_motion = g_step_motion;
      last_step_active = g_step_active;
    }

    osDelay(10);
  }
}

/* USER CODE END Application */

