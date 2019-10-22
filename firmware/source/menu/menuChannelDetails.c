/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 * 				and	 Colin Durbridge, G4EML
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
#include "fw_trx.h"
#include "fw_codeplug.h"
#include "fw_settings.h"

static void updateScreen();
static void handleEvent(int buttons, int keys, int events);
static const int CTCSS_TONE_NONE = 65535;
static const unsigned int CTCSSTones[]={65535,625,670,693,719,744,770,797,825,854,
										885,915,948,974,1000,1035,1072,1109,1148,
										1188,1230,1273,1318,1365,1413,1462,1514,
										1567,1598,1622,1655,1679,1713,1738,1773,
										1799,1835,1862,1899,1928,1966,1995,2035,
										2065,2107,2181,2257,2291,2336,2418,2503,2541};
static int NUM_CTCSS=52;
static int CTCSSRxIndex=0;
static int CTCSSTxIndex=0;
static int NUM_MENUS=7;
static struct_codeplugChannel_t tmpChannel;// update a temporary copy of the channel and only write back if green menu is pressed

enum CHANNEL_DETAILS_DISPLAY_LIST { CH_DETAILS_MODE, CH_DETAILS_DMR_CC,CH_DETAILS_DMR_TS,CH_DETAILS_RXCTCSS, CH_DETAILS_TXCTCSS , CH_DETAILS_BANDWIDTH,
									CH_DETAILS_FREQ_STEP, CH_DETAILS_TOT};

int menuChannelDetails(int buttons, int keys, int events, bool isFirstRun)
{
	if (isFirstRun)
	{
		memcpy(&tmpChannel,currentChannelData,sizeof(struct_codeplugChannel_t));
		gMenusCurrentItemIndex=0;
		for(int i=0;i<NUM_CTCSS;i++)
		{
			if (tmpChannel.txTone==CTCSSTones[i])
			{
				CTCSSTxIndex=i;
			}
			if (tmpChannel.rxTone==CTCSSTones[i])
			{
				CTCSSRxIndex=i;
			}
		}
		updateScreen();
	}
	else
	{
		if (events!=0 && keys!=0)
		{
			handleEvent(buttons, keys, events);
		}
	}
	return 0;
}

static void updateScreen()
{
	int mNum=0;
	char buf[17];
	int tmpVal;

	UC1701_clearBuf();
	UC1701_printCentered(0, "Channel info",UC1701_FONT_GD77_8x16);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i=-1;i<=1;i++)
	{
		mNum = gMenusCurrentItemIndex+i;
		if (mNum<0)
		{
			mNum = NUM_MENUS + mNum;
		}
		if (mNum >= NUM_MENUS)
		{
			mNum = mNum - NUM_MENUS;
		}

		switch(mNum)
		{
			case CH_DETAILS_MODE:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					strcpy(buf,"Mode:FM");
				}
				else
				{
					sprintf(buf,"Mode:DMR");
				}
				break;
			break;
			case CH_DETAILS_DMR_CC:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					strcpy(buf,"Color Code:N/A");
				}
				else
				{
					sprintf(buf,"Color Code:%d",tmpChannel.rxColor);
				}
				break;
			case CH_DETAILS_DMR_TS:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					strcpy(buf,"Timeslot:N/A");
				}
				else
				{
					sprintf(buf,"Timeslot:%d",((tmpChannel.flag2 & 0x40) >> 6) + 1);
				}
				break;
			case CH_DETAILS_RXCTCSS:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					if (tmpChannel.txTone==CTCSS_TONE_NONE)
					{
						strcpy(buf,"Tx CTCSS:None");
					}
					else
					{
						sprintf(buf,"Tx CTCSS:%d.%dHz",tmpChannel.txTone/10 ,tmpChannel.txTone%10 );
					}
				}
				else
				{
					strcpy(buf,"Tx CTCSS:N/A");
				}
				break;
			case CH_DETAILS_TXCTCSS:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					if (tmpChannel.rxTone==CTCSS_TONE_NONE)
					{
						strcpy(buf,"Rx CTCSS:None");
					}
					else
					{
						sprintf(buf,"Rx CTCSS:%d.%dHz",tmpChannel.rxTone/10 ,tmpChannel.rxTone%10 );
					}
				}
				else
				{
					strcpy(buf,"Rx CTCSS:N/A");
				}
				break;
			case CH_DETAILS_BANDWIDTH:
				// Bandwidth
				if (tmpChannel.chMode==RADIO_MODE_DIGITAL)
				{
					strcpy(buf,"Bandwidth:N/A");
				}
				else
				{
					sprintf(buf,"Bandwidth:%s",((tmpChannel.flag4 & 0x02) == 0x02)?"25kHz":"12.5kHz");
				}
				break;
			case CH_DETAILS_FREQ_STEP:
				tmpVal = VFO_FREQ_STEP_TABLE[(tmpChannel.VFOflag5 >> 4)]/10;
				sprintf(buf,"Step:%d.%dkHz",tmpVal, VFO_FREQ_STEP_TABLE[(tmpChannel.VFOflag5 >> 4)] - (tmpVal*10));
				break;
			case CH_DETAILS_TOT:// TOT
				if (tmpChannel.tot!=0)
				{
					sprintf(buf,"TOT:%d",tmpChannel.tot * 15);
				}
				else
				{
					strcpy(buf,"TOT:OFF");
				}
				break;

		}

		if (gMenusCurrentItemIndex==mNum)
		{
			UC1701_fillRect(0,(i+2)*16,128,16,false);
		}

		UC1701_printCore(0,(i+2)*16,buf,UC1701_FONT_GD77_8x16,0,(gMenusCurrentItemIndex==mNum));
	}

	UC1701_render();
	displayLightTrigger();
}

static void handleEvent(int buttons, int keys, int events)
{
	int tmpVal;

	if ((keys & KEY_DOWN)!=0)
	{
		gMenusCurrentItemIndex++;
		if (gMenusCurrentItemIndex>=NUM_MENUS)
		{
			gMenusCurrentItemIndex=0;
		}
	}
	else if ((keys & KEY_UP)!=0)
	{
		gMenusCurrentItemIndex--;
		if (gMenusCurrentItemIndex<0)
		{
			gMenusCurrentItemIndex=NUM_MENUS-1;
		}
	}
	else if ((keys & KEY_RIGHT)!=0)
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_DETAILS_MODE:
				if (tmpChannel.chMode ==RADIO_MODE_ANALOG)
				{
					tmpChannel.chMode = RADIO_MODE_DIGITAL;
				}
				else
				{
					tmpChannel.chMode = RADIO_MODE_ANALOG;
				}
				break;
			case CH_DETAILS_DMR_CC:
				if (tmpChannel.rxColor<15)
				{
					tmpChannel.rxColor++;
					trxSetDMRColourCode(tmpChannel.rxColor);
				}
				break;
			case CH_DETAILS_DMR_TS:
				tmpChannel.flag2 |= 0x40;// set TS 2 bit
				break;
			case CH_DETAILS_RXCTCSS:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					CTCSSTxIndex++;
					if (CTCSSTxIndex>=NUM_CTCSS)
					{
						CTCSSTxIndex=NUM_CTCSS-1;
					}
					tmpChannel.txTone=CTCSSTones[CTCSSTxIndex];
					trxSetTxCTCSS(tmpChannel.txTone);
				}
				break;
			case CH_DETAILS_TXCTCSS:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					CTCSSRxIndex++;
					if (CTCSSRxIndex>=NUM_CTCSS)
					{
						CTCSSRxIndex=NUM_CTCSS-1;
					}
					tmpChannel.rxTone=CTCSSTones[CTCSSRxIndex];
					trxSetRxCTCSS(tmpChannel.rxTone);
				}
				break;
			case CH_DETAILS_BANDWIDTH:
				tmpChannel.flag4 |= 0x02;// set 25kHz bit
				break;
			case CH_DETAILS_FREQ_STEP:
				tmpVal = (tmpChannel.VFOflag5>>4)+1;
				if (tmpVal>15)
				{
					tmpVal=15;
				}
				tmpChannel.VFOflag5 &= 0x0F;
				tmpChannel.VFOflag5 |= tmpVal<<4;
				break;
			case CH_DETAILS_TOT:
				if (tmpChannel.tot<255)
				{
					tmpChannel.tot++;
				}
				break;
		}
	}
	else if ((keys & KEY_LEFT)!=0)
	{
		switch(gMenusCurrentItemIndex)
		{
			case CH_DETAILS_MODE:
				if (tmpChannel.chMode==RADIO_MODE_ANALOG)
				{
					tmpChannel.chMode = RADIO_MODE_DIGITAL;
				}
				else
				{
					tmpChannel.chMode = RADIO_MODE_ANALOG;
				}
				break;
			case CH_DETAILS_DMR_CC:
				if (tmpChannel.rxColor>0)
				{
					tmpChannel.rxColor--;
					trxSetDMRColourCode(tmpChannel.rxColor);
				}
				break;
			case CH_DETAILS_DMR_TS:
				tmpChannel.flag2 &= 0xBF;// Clear TS 2 bit
				break;
			case CH_DETAILS_RXCTCSS:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					CTCSSTxIndex--;
					if (CTCSSTxIndex < 0)
					{
						CTCSSTxIndex=0;
					}
					tmpChannel.txTone=CTCSSTones[CTCSSTxIndex];
					trxSetTxCTCSS(tmpChannel.txTone);
				}
				break;
			case CH_DETAILS_TXCTCSS:
				if (tmpChannel.chMode == RADIO_MODE_ANALOG)
				{
					CTCSSRxIndex--;
					if (CTCSSRxIndex < 0)
					{
						CTCSSRxIndex=0;
					}
					tmpChannel.rxTone=CTCSSTones[CTCSSRxIndex];
					trxSetRxCTCSS(tmpChannel.rxTone);
				}
				break;
			case CH_DETAILS_BANDWIDTH:
				tmpChannel.flag4 &= ~0x02;// clear 25kHz bit
				break;
			case CH_DETAILS_FREQ_STEP:
				tmpVal = (tmpChannel.VFOflag5>>4)-1;
				if (tmpVal<0)
				{
					tmpVal=0;
				}
				tmpChannel.VFOflag5 &= 0x0F;
				tmpChannel.VFOflag5 |= tmpVal<<4;
				break;
			case CH_DETAILS_TOT:
				if (tmpChannel.tot>0)
				{
					tmpChannel.tot--;
				}
				break;
		}
	}
	else if ((keys & KEY_GREEN)!=0)
	{
		memcpy(currentChannelData,&tmpChannel,sizeof(struct_codeplugChannel_t));
		if (buttons & BUTTON_SK2 )
		{
			// Function button is also pressed, so these changes need to be saved back to the codeplug
			// settingsCurrentChannelNumber is -1 when in VFO mode
			if (settingsCurrentChannelNumber != -1 )
			{
				codeplugChannelSaveDataForIndex(settingsCurrentChannelNumber,currentChannelData);
			}
			else
			{
				// Its the VFO, which is in the nonVolatile settings and doesn't need to be updated unless it needs to be read back into the CPS
			}
		}

		menuSystemPopAllAndDisplayRootMenu();
		return;
	}
	else if ((keys & KEY_RED)!=0)
	{
		menuSystemPopPreviousMenu();
		return;
	}

	updateScreen();
}
