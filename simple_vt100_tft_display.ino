/*
 * Copyright (c) 2022 Marcel Licence
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * veröffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es nützlich sein wird, jedoch
 * OHNE JEDE GEWÄHR,; sogar ohne die implizite
 * Gewähr der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License für weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/**
 * @file simple_vt100_tft_display.ino
 * @author Marcel Licence
 * @date 16.12.2021
 *
 * @brief  This is the arduino project file to drive the IL9341 tft display
 * There is just support for some VT100 commands used in the synth / audio projects
 */


#include <TFT_eSPI.h>
#include <SPI.h>

#define TFT_LED 5

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

uint8_t fifo[256];
uint8_t fifo_in = 0;
uint8_t fifo_out = 0;

void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200); /* input for vt100 console data */

    Serial.println("ILI9341 Test!");

    tft.init();
    tft.setRotation(1);
    tft.setSwapBytes(true);

    tft.fillScreen(ILI9341_BLACK);

    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setTextSize(1);
    tft.setCursor(0, 0);
#if 0
    tft.printf("12345678901234567890123456789012345678901234567890123");
    delay(10000);
#endif
}

static uint8_t ins = 0;
static uint32_t inc = 0;

uint16_t getColorFromNum(uint8_t num)
{
    switch (num)
    {
    case 0:
        return ILI9341_BLACK;
    case 31:
        return ILI9341_RED;
    case 32:
        return ILI9341_GREEN;
    case 33:
        return ILI9341_YELLOW;
    case 34:
        return ILI9341_PINK;
    case 144:
    case 60:
    case 160:
        return ILI9341_BLUE;
    default:
        Serial.printf("undect: %d:%d:%d\n", ins, inc, num);
    }

    return 0;
}

#if 0
uint8_t lines[29][54] = {0};

void printAll(void)
{
    for (int i = 0; i < 29; i++)
    {
        tft.printf("%s", lines[i]);
    }
}
#endif

static uint8_t cntC = 0;
static uint8_t cntL = 0;

bool fill = false;

void enterChar(uint8_t cc)
{

    if (cntC > 53)
    {
        cntC = 0;
        cntL ++;
    }

    if (cntL > 32)
    {
#if 0
        for (int i = 0; i < 28; i++)
        {
            memcpy(lines[i], lines[i + ], 53);
        }
        cntL--;
#else
        cntL = 0;
        fill = true;
        tft.setCursor(0, 0);
#endif
    }

    if (cc == '\n')
    {
        if (fill)
        {
            tft.fillRect(0, cntL * 8, 320, 8, ILI9341_BLACK);
        }
        cntL++;
    }
    else
    {
#if 0
        lines[cntL][cntC] = cc;
#endif
        cntC ++;
    }


}

void loop(void)
{
    static uint8_t param1 = 0;
    static uint8_t param2 = 0;

    static char charBuff[1024];

    /* simple echo */
    while (Serial.available())
    {
        Serial.write(Serial.read());
    }

    /* move quickly all data from serial to free the buffer */
    while (Serial2.available())
    {
        fifo[fifo_in] = Serial2.read();
        fifo_in++;
    }

    /* process double buffered data */
    while (fifo_in != fifo_out)
    {
        const char in = fifo[fifo_out];
        fifo_out++;

        switch (ins)
        {
        case 0:
            if (in == '\033')
            {
                charBuff[inc] = 0;
                tft.printf("%s", charBuff);
                inc = 0;
                ins = 1;
            }
            else
            {
                charBuff[inc] = in;
                inc ++;

                enterChar(in);

                /* print directly */
                charBuff[inc] = 0;
                tft.printf("%s", charBuff);
                inc = 0;
            }
            break;
        case 1:
            if (in == '[')
            {
                ins = 2;
                param1 = 0;
                param2 = 0;
            }
            else
            {
                ins = 0;
            }
            break;
        case 2:
            if (in == ';')
            {
                ins = 3;
            }
            else if (in == '?')
            {
                ins = 4;
            }
            else if (in == 'm')
            {
                /* back to default */
                tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
                ins = 0;
            }
            else
            {
                param1 *= 10;
                param1 += (in - '0');
            }
            break;
        case 3:
            if (in == 'm')
            {
                ins = 0;

                uint16_t fColor;
                uint16_t bColor;

                fColor = getColorFromNum(param1);
                bColor = getColorFromNum(param2);

                tft.setTextColor(bColor, fColor);
            }
            else if (in == 'H')
            {
                tft.setCursor(0, 0);
                ins = 0;
            }
            else
            {
                param2 *= 10;
                param2 += (in - '0');
            }
            break;
        case 4:
            if (in == '2')
            {
                ins = 5;
            }
            else
            {
                ins = 0;
            }
            break;
        case 5:
            if (in == '5')
            {
                ins = 6;
            }
            else
            {
                ins = 0;
            }
            break;
        case 6:
            if (in == 'l')
            {
                tft.setCursor(0, 0);
                fill = false;
                ins = 0;

                cntC = 0;
                cntL = 0;
            }
            else
            {
                ins = 0;
            }
            break;
        }
    }
}

