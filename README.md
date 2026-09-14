# STM32F411 FreeRTOS Threshold Alarm

A three-task FreeRTOS application on an STM32F411: periodic sensor acquisition,
threshold evaluation with hysteresis and sample confirmation, and status
reporting over UART.

Written as a reference implementation of the task structure I use for
alarm-monitoring firmware. It contains no employer or client code.

## Hardware

Nucleo-F411RE. No external components required.

| Function | Pin | Notes |
|---|---|---|
| Alarm output | PA5 | Onboard LED, active high |
| Console | PA2 / PA3 | USART2, 115200 8N1, via ST-LINK virtual COM port |
| Sensor | ADC1, internal channel | On-die temperature sensor |

The on-die temperature sensor is used so the project runs on a bare board with
nothing wired. STM32F411 ships without factory calibration values for it, so
the datasheet typical figures are used (V25 = 0.76 V, slope = 2.5 mV/C) and
absolute accuracy is around +/-3 C. That is fine for exercising a threshold,
and it is not a measurement. Replacing `read_raw()` and `raw_to_celsius()` in
`sensor_task.c` is the only change needed to drive this from a real sensor.

For a WeAct Black Pill, change `ALARM_LED_PORT` to `GPIOC`, `ALARM_LED_PIN` to
`GPIO_PIN_13`, and `ALARM_LED_ACTIVE_HIGH` to `0` in `app_config.h`, and move
the console to USART1 on PA9 / PA10.

## Task structure

```
  sensor task (prio 3)  --- queue, depth 4 --->  alarm task (prio 2)
   every 100 ms                                   blocks on queue
   ADC read                                       hysteresis + confirm
   post sample                                    drives PA5
                                                  logs transitions
                                                       |
                                                  shared status
                                                       |
                                              report task (prio 1)
                                               every 2 s, UART
```

## Design decisions

**Sensor task has the highest priority.** It is the only task with a timing
requirement. Jitter in the sample interval shows up directly as jitter in
detection latency, so it preempts everything else and uses `vTaskDelayUntil`
rather than `vTaskDelay` so the period does not drift with execution time.

**The queue is four deep, not one and not thirty-two.** One would drop a
sample every time the alarm task was briefly preempted. Deep queues are worse
than useless here: a backlog of old readings means the alarm reacts to data
that is already seconds stale, which for an alarm is a fault, not a buffer.
Four absorbs normal scheduling jitter and nothing more.

**The sensor task never blocks on the queue.** `xQueueSend` is called with a
zero timeout. If the consumer is behind, the sample is dropped and counted.
Blocking the producer to wait for a slow consumer would corrupt the sampling
cadence, which is the one thing this task exists to protect. The drop counter
is printed in the status line so the condition is visible rather than silent.

**Polled ADC, not DMA.** One channel at 10 Hz costs about 20 us of CPU per
sample with a 480-cycle sampling time. DMA would add a completion path and
buffer management for no measurable gain. DMA earns its complexity at the
sampling rates where it matters, not here.

**Hysteresis and confirmation are separate mechanisms.** The rise and fall
thresholds are 3 C apart so the output does not chatter when the reading sits
on the boundary. Independently, three consecutive samples are required before
a transition is accepted, which rejects a single noisy conversion. Hysteresis
handles a signal sitting near the threshold; confirmation handles a spike.
Either alone leaves a hole.

**The output is driven before the log line is written.** A state change moves
the physical output first, then takes the UART mutex. The alarm output must
never wait on a peripheral another task might be holding.

**Shared status is copied inside a critical section.** The report task reads a
multi-field struct that the alarm task writes. Without the critical section it
could observe a state and a reading from different samples. It is a short
copy, so a critical section is cheaper and simpler than a mutex here.

**The report task is lowest priority.** UART at 115200 takes about 10 ms for a
long line. That must never delay acquisition or alarm evaluation, so it runs
only when nothing else is ready.

**No allocation after startup.** The queue, mutex and all three tasks are
created in `app_init()` before the scheduler starts. Failure at that point
calls `Error_Handler()` rather than running in a degraded state.

**Stack sizes are measured, not guessed.** The report task gets more because
`vsnprintf` is the deepest call in the system. `uxTaskGetStackHighWaterMark`
for all three tasks is printed in every status line so the headroom is visible
during a long run instead of being assumed.

## Build

Generate the peripheral init with STM32CubeMX, then add the files from
`Core/`.

CubeMX configuration:

- Board: NUCLEO-F411RE, or MCU STM32F411RET6
- RCC: HSE crystal/ceramic resonator, clock to 100 MHz
- SYS: Timebase source TIM1, not SysTick (FreeRTOS owns SysTick)
- ADC1: Temperature Sensor Channel enabled, sampling time 480 cycles,
  resolution 12 bit, continuous conversion disabled
- USART2: asynchronous, 115200 baud
- PA5: GPIO output
- Middleware: FreeRTOS, interface CMSIS_V1, no tasks or queues defined in
  CubeMX. This code creates its own.

In `FreeRTOSConfig.h`, confirm `configUSE_MUTEXES 1` and
`configCHECK_FOR_STACK_OVERFLOW 2`.

In `main.c`, after the peripheral init and before the scheduler starts:

```c
/* USER CODE BEGIN 2 */
app_init();
/* USER CODE END 2 */
```

Then call `vTaskStartScheduler()`. If CubeMX generated `MX_FREERTOS_Init()`
and `osKernelStart()`, put `app_init()` inside `MX_FREERTOS_Init()` instead and
leave the generated start call alone.

## Output

```
[2100] normal  temp=31.20 C  raw=946  n=21  dropped=0
      stack free: sensor=118 alarm=121 report=164
[4100] normal  temp=31.60 C  raw=947  n=41  dropped=0
      stack free: sensor=118 alarm=121 report=164
```

Pinching the chip package raises the die temperature enough to cross 40 C and
trip the alarm, which is the simplest way to test the transition path without
external hardware.

## Known limitations

- The temperature reading is uncalibrated and only suitable for demonstrating
  the threshold path.
- There is no persistence. Alarm state and counters reset on power loss.
- There is no watchdog. A production version of this would feed an IWDG from a
  task that verifies the other two are still running.
- `HAL_UART_Transmit` is blocking. At 115200 with short lines this is
  acceptable in the lowest priority task, but an interrupt or DMA driven
  transmit would be the right choice if the console load grew.
