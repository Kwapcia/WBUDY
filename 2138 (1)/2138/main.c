/*************************************************************************************
 *
 * @Description:
 *
 *
 * @Authors: Julia Pietrzykowska
 * Karolina Soltysiak
 * Aleksandra Kwapinska
 *
 * @Change log:
 *           2023.01.06: Wersja oryginalna.
 *
 ******************************************************************************/

// 200 lewo 400 gora 800 prawo 1000 dol 100 wglab

#include "general.h"
#include <lpc2xxx.h>
#include <printf_P.h>
#include <printf_init.h>
#include <consol.h>
#include <config.h>
#include "irq/irq_handler.h"
#include "timer.h"
#include "VIC.h"
#include "lcd.h"
#include "Common_Def.h"
#include <stdio.h>
#include "pca9532.h"
#include "i2c.h"
#include "eeprom.h"
#include "lcd_hw.h"
#include <string.h>

/************************************************************************
 * @Description: opóźnienie wyrażone w liczbie sekund
 * @Parameter:
 *    [in] seconds: liczba sekund opĂłĹşnienia
 * @Returns: Nic
 * @Side effects:
 *    przeprogramowany Timer #0
 *************************************************************************/
void sdelay(tU32 seconds)
{
    T0TCR = TIMER_RESET;         // Zatrzymaj i zresetuj
    T0PR = PERIPHERAL_CLOCK - 1; // jednostka w preskalerze
    T0MR0 = seconds;
    T0IR = TIMER_ALL_INT; // Resetowanie flag przerwaĹ„
    T0MCR = MR0_S;        // Licz do wartości w MR0 i zatrzymaj się
    T0TCR = TIMER_RUN;    // Uruchom timer

    // sprawdź czy timer działa
    // nie ma wpisanego ogranicznika liczby pętli, ze względu na charakter procedury
    while (T0TCR & TIMER_RUN)
    {
    }
}

/**
 * @brief: Opoznienie wyrazone w liczbie milisekund
 * @param seconds:
 *      liczba milisekund opoznienia
 * @Returns: Nic
 * @attention:
 *    przeprogramowany Timer #1
 */
void delayMs(tU16 delayInMs)
{
    T1TCR = 0x02;
    T1PR = 0x00;
    T1MR0 = delayInMs * (CORE_FREQ / PBSD / 1000);
    T1IR = 0xff;
    T1MCR = 0x04;
    T1TCR = 0x01;

    while ((T1TCR & 0x01) == 1)
    {
    }
}

int rand_X = 23;
int rand_m = 150;
int rand_c = 43;
int rand_a = 21;
int rand()
{
    rand_X = (rand_a * rand_X + rand_c) % rand_m;
    return rand_X;
}

struct gracz
{
    char name[12];
    int wynik;
};

void input_name(struct gracz *gracz)
{
    gracz->name[0] = '\0';
    char int_str[20];
    lcdClrscr();
    lcdGotoxy(10, 0);
    lcdPuts("  YOU WON! :)  ");
    sprintf(int_str, "%d", gracz->wynik);
    lcdGotoxy(10, 15);
    lcdPuts("score:        ");
    lcdGotoxy(10, 30);
    lcdPuts(int_str);
    delayMs(2500);
    lcdGotoxy(10, 65);
    lcdPuts("press joystick");
    while ((IOPIN & 0x00000100) != 0)
    {
    }
    lcdClrscr();
    lcdGotoxy(10, 0);
    lcdPuts("UP/DOWN-choice\n");
    lcdPuts("RIGHT-next\n");
    lcdPuts("PRESS=OK\n");
    lcdPuts("NAME:\n");

    int index = 0;
    char sign = 'A';
    int position = 10;

    lcdGotoxy(position, 60);
    lcdPutchar(sign);
    delayMs(200);

    while ((IOPIN & 0x00000100) != 0)
    {
        delayMs(200);
        if ((IOPIN & 0x00001000) == 0)
        {
            if (sign == 'A')
            {
                sign = 'Z';
            }
            else
            {
                sign--;
            }
            lcdGotoxy(position, 60);
            lcdPutchar(sign);
        }
        else if ((IOPIN & 0x00000400) == 0)
        {
            if (sign == 'Z')
            {
                sign = 'A';
            }
            else
            {
                sign++;
            }
            lcdGotoxy(position, 60);
            lcdPutchar(sign);
        }
        else if ((IOPIN & 0x00000200) == 0)
        {
            gracz->name[index] = '\0';
            if (index != 0)
            {
                index--;
            }
            if (position != 10)
            {
                lcdGotoxy(position, 60);
                lcdPutchar(' ');
                position -= 10;
                lcdGotoxy(position, 60);
                lcdPutchar('A');
                sign = 'A';
            }
        }
        else if ((IOPIN & 0x00000800) == 0)
        {
            if (position != 90)
            {
                gracz->name[index] = sign;
                index++;
                position += 10;
                lcdGotoxy(position, 60);
                lcdPutchar(sign);
            }
        }
    }
    gracz->name[index] = sign;
    index++;
    int name_idx = index;
    while (index < 12)
    {
        gracz->name[index] = '0';
        index++;
    }
    lcdGotoxy(10, 80);
    lcdPuts("YOUR NAME:\n");

    
    lcdGotoxy(10, 95);
    for (int i = 0; i < name_idx; i++)
    {
        lcdPutchar(gracz->name[i]);
    }

    delayMs(2000);
}

int game(struct gracz *gracz)
{

    tBool isGame = TRUE;
    tBool isStupid = FALSE;
    int round_number = 0;

    while (isGame && !isStupid && round_number < 10)
    {
        lcdGotoxy(10, 0);
        delayMs(1000);
        int random_number = rand() % 4;
        tBool roundInProgress = TRUE;
        while (roundInProgress)
        {

            if (random_number == 0)
            {
                lcdGotoxy(10, 0);
                lcdPuts("       *       \n"
                        "      **       \n"
                        "    ****       \n"
                        "  ***********  \n"
                        "*************  \n"
                        "  ***********  \n"
                        "    ****       \n"
                        "      **       \n"
                        "       *       \n");
                int i = 0;
                while (i < 1000)
                {
                    i++;
                    delayMs(1);

                    if ((IOPIN & 0x00000200) == 0)
                    {
                        round_number++;
                        roundInProgress = FALSE;
                        setPca9532Pin(round_number - 1, 0);
                        break;
                    }
                }
                if (i == 1000)
                {
                    isStupid = TRUE;
                    isGame = FALSE;
                    roundInProgress = FALSE;
                }
                gracz->wynik -= i;
            }

            else if (random_number == 1)
            {
                lcdGotoxy(10, 0);
                lcdPuts("      *       \n"
                        "      ***      \n"
                        "    *******    \n"
                        "  ***********  \n"
                        "      ***      \n"
                        "      ***      \n"
                        "      ***      \n"
                        "      ***      \n");
                int i = 0;
                while (i < 1000)
                {
                    i++;
                    delayMs(1);

                    if ((IOPIN & 0x00000400) == 0)
                    {
                        round_number++;
                        roundInProgress = FALSE;
                        setPca9532Pin(round_number - 1, 0);
                        break;
                    }
                }

                if (i == 1000)
                {
                    isStupid = TRUE;
                    isGame = FALSE;
                    roundInProgress = FALSE;
                }
                gracz->wynik -= i;
            }
            else if (random_number == 2)
            {
                lcdGotoxy(10, 0);
                lcdPuts("      *       \n"
                        "       **      \n"
                        "       ****    \n"
                        " ***********   \n"
                        " ************* \n"
                        " ***********   \n"
                        "       ****    \n"
                        "       **      \n"
                        "       *       \n");
                int i = 0;
                while (i < 1000)
                {
                    i++;
                    delayMs(1);
                    if ((IOPIN & 0x00000800) == 0)
                    {
                        round_number++;
                        roundInProgress = FALSE;
                        setPca9532Pin(round_number - 1, 0);
                        break;
                    }
                }
                if (i == 1000)
                {
                    isStupid = TRUE;
                    isGame = FALSE;
                    roundInProgress = FALSE;
                }
                gracz->wynik -= i;
            }
            else if (random_number == 3)
            {
                lcdGotoxy(10, 0);
                lcdPuts("    ***       \n"
                        "     ***       \n"
                        "     ***       \n"
                        "     ***       \n"
                        " ***********   \n"
                        "  *********    \n"
                        "   *******     \n"
                        "     ***       \n"
                        "      *        \n");
                int i = 0;
                while (i < 1000)
                {
                    i++;
                    delayMs(1);
                    if ((IOPIN & 0x00001000) == 0)
                    {
                        round_number++;
                        roundInProgress = FALSE;
                        setPca9532Pin(round_number - 1, 0);
                        break;
                    }
                }
                if (i == 1000)
                {
                    isStupid = TRUE;
                    isGame = FALSE;
                    roundInProgress = FALSE;
                }
                gracz->wynik -= i;
            }
        }
    }
    return round_number;
}

int choice()
{
            lcdClrscr();
        while ((IOPIN & 0x00000100) != 0)
        {
            lcdGotoxy(0, 0);
            lcdPuts("UwUOwOUwUOwOUwU\n");
            lcdPuts("SUPER COOL GAME\n");
            lcdPuts("***************\n");
            lcdPuts("<-PLAY   TOP1->\n");
            if ((IOPIN & 0x00000200) == 0)
            {
                lcdPuts("\n      PLAY     \n");
                lcdPuts(" press joystick");
                while ((IOPIN & 0x00000100) != 0)
                                {

                                }
                return 2;
            }

            if ((IOPIN & 0x00000800) == 0)
            {
                lcdPuts("\n  HIGH  SCORE  \n");
                lcdPuts(" press joystick");
                while ((IOPIN & 0x00000100) != 0)
                {

                }
                return 1;
            }
        }
        return 0;
}

void load_highscore_info(struct gracz *highscore_gracz)
{


    tU8 testBuf[16];
    eepromPageRead(0x0000, testBuf, 16);
    char str_wynik[4];
    int wynik;
    for (int i = 0; i<4;i++)
    {
        str_wynik[i] = (char) testBuf[12+i];
    }
    sscanf(str_wynik, "%d", &wynik);
    highscore_gracz->wynik = wynik;
    


    tU8 dummy[12];
    for (int j = 0; j<12; j++)
    {
    	if (testBuf[j] == 48 || j == 12)
    	{
    		dummy[j] = '\0';
    		break;
    	}
    	else {
    	dummy[j] = testBuf[j];
    	}
    }
    sprintf(highscore_gracz->name, "%s", dummy);
    delayMs(2000);

}

void save_highscore(struct gracz *nowy_gracz, struct gracz *highscore_gracz)
{

    if (nowy_gracz->wynik > highscore_gracz->wynik)
    {
        char saveBuf[16];
        for (int i = 0; i < 12; i++)
        {
            saveBuf[i] = nowy_gracz->name[i];
        }
        char wynik[4];
        sprintf(wynik, "%d", nowy_gracz->wynik);
        for (int i = 12; i < 16; i++)
        {
            saveBuf[i] = wynik[i-12];
        }

        saveBuf[0] = nowy_gracz->name[0];

        tS8 errorCode;
        errorCode = eepromWrite(0x0000,(tU8*) saveBuf, 16);
        if (errorCode == I2C_CODE_OK) {
        	lcdGotoxy(50, 50);
            lcdPuts("OK");
            delayMs(1500);
        } else {
        	lcdGotoxy(50, 50);
            lcdPuts("Blad");
            delayMs(1500);
        }
    }

}

int main(void)
{
    i2cInit();
    printf_init();
    lcdInit();
    lcdColor(0xff, 0x00);
    pca9532Init();
    struct gracz gracz;
    struct gracz highscore_gracz;

    while (TRUE)
    {
    int wybor = choice();
        if (wybor == 1)
        {
            lcdClrscr();
            lcdGotoxy(0, 0);
            lcdPuts("Highest score:");
            load_highscore_info(&highscore_gracz);
            lcdGotoxy(10, 20);
            lcdPuts(highscore_gracz.name);
            char wynik[4];
            sprintf(wynik, "%d", highscore_gracz.wynik);
            lcdGotoxy(10, 40);
            lcdPuts(wynik);

            delayMs(5000);
            wybor = 0;
        }

        if (wybor == 2)
        {
            lcdClrscr();
            lcdGotoxy(10, 20);
            lcdPuts("   GET READY   ");
            delayMs(1000);


            gracz.wynik = 10000;
            int round_number = game(&gracz);
            for (int i = 0; i<round_number;i++){
            	setPca9532Pin(i, 1);
            	delayMs(200);
            }
            if (round_number == 10)
            {
                input_name(&gracz);
                save_highscore(&gracz, &highscore_gracz);
                wybor = 0;
            }
            else
            {
            	lcdClrscr();
                lcdGotoxy(10, 20);
                lcdPuts("  OH NO!   :(  ");
                lcdGotoxy(10, 40);
                lcdPuts("    YOU LOST   ");
                lcdGotoxy(10, 60);
                lcdPuts("   TRY AGAIN!  ");
                delayMs(2000);
                wybor = 0;
            }
        }
    }
}
