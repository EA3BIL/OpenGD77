/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "menu/menuSystem.h"
#include "menu/menuUtilityQSOData.h"
#include "fw_settings.h"
#include "fw_HR-C6000.h"


static void updateScreen();
static void handleEvent(int buttons, int keys, int events);

static const int PIT_COUNTS_PER_SECOND = 10000;
static int timeInSeconds;
static uint32_t nextSecondPIT;

int menuTxScreen(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{

		settingsPrivateCallMuteMode = false;
		if ((currentChannelData->flag4 & 0x04) == 0x00 && (  trxCheckFrequencyInAmateurBand(currentChannelData->txFreq) || nonVolatileSettings.txFreqLimited == 0x00))
		{
			nextSecondPIT = PITCounter + PIT_COUNTS_PER_SECOND;
			timeInSeconds = currentChannelData->tot*15;

			GPIO_PinWrite(GPIO_LEDgreen, Pin_LEDgreen, 0);
			GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 1);

			txstopdelay=0;
			clearIsWaking();
			trx_setTX();
			updateScreen();
		}
		else
		{
			// Note.
			// Currently these messages are not being displayed, because this screen gets immediately after the display is update
			// However the melody will play
			//
			// We need to work out how to display this message for 1 or 2 seconds, even if the PTT is released.
			// But this would require some sort of timer callback system, which we don't currently have.
			//
			UC1701_clearBuf();
			UC1701_printCentered(4, "ERROR",UC1701_FONT_16x32);
			if ((currentChannelData->flag4 & 0x04) !=0x00)
			{
				UC1701_printCentered(40, "Rx only",UC1701_FONT_GD77_8x16);
			}
			else
			{
				UC1701_printCentered(40, "OUT OF BAND",UC1701_FONT_GD77_8x16);
			}
			UC1701_render();
			displayLightOverrideTimeout(-1);
			set_melody(melody_ERROR_beep);
		}
	}
	else
	{
		if (trxIsTransmitting)
		{
			if (PITCounter >= nextSecondPIT )
			{
				if (currentChannelData->tot==0)
				{
					timeInSeconds++;
				}
				else
				{
					timeInSeconds--;
					if (timeInSeconds <= (nonVolatileSettings.txTimeoutBeepX5Secs * 5))
					{
						if (timeInSeconds%5==0)
						{
							set_melody(melody_key_beep);
						}
					}
				}
				if (currentChannelData->tot!=0 && timeInSeconds == 0)
				{
					set_melody(melody_tx_timeout_beep);
					UC1701_clearBuf();
					UC1701_printCentered(20, "TIMEOUT",UC1701_FONT_16x32);
					UC1701_render();
				}
				else
				{
					updateScreen();
				}

				nextSecondPIT = PITCounter + PIT_COUNTS_PER_SECOND;
			}
		}
		handleEvent(buttons, keys, events);
	}
	return 0;
}

static void updateScreen()
{

	menuDisplayQSODataState = QSO_DISPLAY_DEFAULT_SCREEN;
	if (menuControlData.stack[0]==MENU_VFO_MODE)
	{
		menuVFOModeUpdateScreen(timeInSeconds);
		displayLightOverrideTimeout(-1);
	}
	else
	{
		menuChannelModeUpdateScreen(timeInSeconds);
		displayLightOverrideTimeout(-1);
	}
}

static void handleEvent(int buttons, int keys, int events)
{
	if ((buttons & BUTTON_PTT)==0 || (currentChannelData->tot!=0 && timeInSeconds == 0))
	{
		if (trxIsTransmitting)
		{
			trxIsTransmitting=false;
			if (trxGetMode() == RADIO_MODE_ANALOG)
			{
				// In analog mode. Stop transmitting immediately
				GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 0);

				// Need to wrap this in Task Critical to avoid bus contention on the I2C bus.
				taskENTER_CRITICAL();
				trx_activateRx();
				taskEXIT_CRITICAL();
				menuSystemPopPreviousMenu();
			}
			// When not in analogue mode, only the trxIsTransmitting flag is cleared
			// This screen keeps getting called via the handleEvent function and goes into the else clause - below.
		}
		else
		{
			// In DMR mode, wait for the DMR system to finish before exiting
			if (slot_state < DMR_STATE_TX_START_1)
			{
				GPIO_PinWrite(GPIO_LEDred, Pin_LEDred, 0);
				menuSystemPopPreviousMenu();
			}
		}
	}
}
